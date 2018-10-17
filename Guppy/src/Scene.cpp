
#include "CmdBufHandler.h"
#include "PipelineHandler.h"
#include "Scene.h"

Scene::Scene(const MyShell::Context& ctx, const UniformBufferResources& uboResource, size_t texCount) {
    pDescResources_ = PipelineHandler::createDescriptorResources({uboResource.info}, 1 /* !!! hardcode */, texCount);
    PipelineHandler::createPipelineResources(plResources_);
    create_draw_cmds(ctx);
}

void Scene::addMesh(const MyShell::Context& ctx, std::unique_ptr<ColorMesh> pMesh) {
    pMesh->setSceneData(ctx, colorMeshes_.size());
    if (pMesh->getStatus() != Mesh::STATUS::READY) {
        ldgFutures_.emplace_back(pMesh->load(ctx));
    } else {
        pMesh->prepare(ctx.dev, pDescResources_);
    }
    colorMeshes_.push_back(std::move(pMesh));
}

// void Scene::addMesh(const MyShell::Context& ctx, const VkDescriptorBufferInfo& ubo_info, std::unique_ptr<LineMesh> pMesh) {
//    pMesh->setOffset(lineMeshes_.size());
//    if (!pMesh->isReady()) {
//        loading_futures_.emplace_back(pMesh->load(ctx));
//    } else {
//        pMesh->prepare(ctx, ubo_info, pDescResouces);
//    }
//    lineMeshes_.push_back(std::move(pMesh));
//}

void Scene::addMesh(const MyShell::Context& ctx, std::unique_ptr<TextureMesh> pMesh) {
    // Set values based on scene, and move pointer onto scene
    // TODO: remove scene information from the mesh if mesh becomes shared
    pMesh->setSceneData(ctx, texMeshes_.size());
    texMeshes_.push_back(std::move(pMesh));

    // Update descriptors for all the meshes stored
    if (isNewTexture(texMeshes_.back())) {
        updateDescriptorResources(ctx, texMeshes_.back());
    }

    // Check mesh loading status
    if (texMeshes_.back()->getStatus() != Mesh::STATUS::READY) {
        ldgFutures_.emplace_back(texMeshes_.back()->load(ctx));
    } else {
        texMeshes_.back()->prepare(ctx.dev, pDescResources_);
    }
}

void Scene::removeMesh(Mesh* mesh) {
    // TODO...
    return;
}

bool Scene::update(const MyShell::Context& ctx, int frameIndex) {
    auto startCount = readyCount();

    // Check loading futures
    if (!ldgFutures_.empty()) {
        auto itFut = ldgFutures_.begin();

        while (itFut != ldgFutures_.end()) {
            auto& fut = (*itFut);

            // Check the status but don't wait...
            auto status = fut.wait_for(std::chrono::seconds(0));

            if (status == std::future_status::ready) {
                auto pMesh = fut.get();
                // Future is ready so prepare the mesh...
                pMesh->prepare(ctx.dev, pDescResources_);

                // Remove the future from the list if all goes well.
                itFut = ldgFutures_.erase(itFut);

            } else {
                ++itFut;
            }
        }
    }

    // TODO: fix all this ready stuff
    for (auto& pMesh : texMeshes_) {
        if (pMesh->getStatus() == Mesh::STATUS::PENDING_TEXTURE) {
            pMesh->tryCreateDescriptorSets(pDescResources_);
        }
    }

    // TODO: This is bad. Should store descriptor sets on the scene and only record if a new
    // descriptor set is added... or something like that. Really this should be fast...
    return startCount != readyCount();
}

void Scene::updateDescriptorResources(const MyShell::Context& ctx, std::unique_ptr<TextureMesh>& pMesh) {
    // There are not enough sets allocated for the descriptor pool so a new one needs to be created.
    auto newTexCount = pDescResources_->texCount + 1;
    auto pNewRes = PipelineHandler::createDescriptorResources(pDescResources_->uboInfos, 1, newTexCount);
    // Move the previous descriptor resources to the handler for later cleanup.
    PipelineHandler::takeOldResources(std::move(pDescResources_));
    // Create and init the new descriptor resources
    pDescResources_ = std::move(pNewRes);
    // Allocate descriptor sets for meshes that are ready ...
    for (auto& pMesh : texMeshes_) {
        if (pMesh->getStatus() == Mesh::STATUS::READY) {
            pMesh->tryCreateDescriptorSets(pDescResources_);
        }
    }
}

// TODO: make cmd amount dynamic
const VkCommandBuffer& Scene::getDrawCmds(const uint32_t& frame_data_index) {
    auto& res = draw_resources_[frame_data_index];
    if (res.thread.joinable()) res.thread.join();
    return res.cmd;
}

void Scene::create_draw_cmds(const MyShell::Context& ctx) {
    draw_resources_.resize(ctx.image_count);

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = CmdBufHandler::graphics_cmd_pool();
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    for (auto& res : draw_resources_) {
        vk::assert_success(vkAllocateCommandBuffers(ctx.dev, &alloc_info, &res.cmd));
    }
}

void Scene::recordDrawCmds(const MyShell::Context& ctx, const std::vector<FrameData>& frame_data,
                           const std::vector<VkFramebuffer>& framebuffers, const VkViewport& viewport, const VkRect2D& scissor) {
    // TODO: this is in the wrong order. YOu should loop through the meshes, and then update each draw resource...
    // MAYBE USE PRIMARY AND SECONDARY COMMAND BUFFERS!!!
    for (size_t i = 0; i < draw_resources_.size(); i++) {
        if (vkGetFenceStatus(ctx.dev, frame_data[i].fence) == VK_SUCCESS) {
            // sync
            record(ctx, draw_resources_[i].cmd, framebuffers[i], frame_data[i].fence, viewport, scissor, i);
        } else {
            // If this fails, then some things need to be rethought.
            assert(false);
            // asnyc TODO: Can't update current draw until its done.. This isn't great. It can be revisited later.
            draw_resources_[i].thread = std::thread(&Scene::record, this, ctx, draw_resources_[i].cmd, framebuffers[i],
                                                    frame_data[i].fence, viewport, scissor, i, true);
        }
    }
    PipelineHandler::cleanupOldResources();
}

void Scene::record(const MyShell::Context& ctx, const VkCommandBuffer& cmd, const VkFramebuffer& framebuffer, const VkFence& fence,
                   const VkViewport& viewport, const VkRect2D& scissor, size_t frameIndex, bool wait) {
    if (wait) {
        // Wait for fences
        vk::assert_success(vkWaitForFences(ctx.dev, 1, &fence, VK_TRUE, UINT64_MAX));
    }

    // Reset buffer
    vkResetCommandBuffer(cmd, 0);

    // Record
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin_info.pInheritanceInfo = nullptr;  // Optional

    vk::assert_success(vkBeginCommandBuffer(cmd, &begin_info));

    // for (auto& render_pass : render_passes_) {
    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    // render_pass_info.renderPass = render_pass;
    render_pass_info.renderPass = plResources_.renderPass;
    render_pass_info.framebuffer = framebuffer;
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = ctx.extent;

    // Inheritance info for secondary command buffers
    VkCommandBufferInheritanceInfo inherit_info = {};
    inherit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inherit_info.renderPass = plResources_.renderPass;
    inherit_info.subpass = 0;
    inherit_info.framebuffer = framebuffer;
    // Validation layer: Cannot set inherited occlusionQueryEnable in vkBeginCommandBuffer() when device does not support
    // inheritedQueries.
    inherit_info.occlusionQueryEnable = VK_FALSE;  // TODO: not sure
    inherit_info.queryFlags = 0;                   // TODO: not sure
    inherit_info.pipelineStatistics = 0;           // TODO: not sure

    /*  Need to pad this here because:

        In vkCmdBeginRenderPass() the VkRenderPassBeginInfo struct has a clearValueCount
        of 2 but there must be at least 3 entries in pClearValues array to account for the
        highest index attachment in renderPass 0x2f that uses VK_ATTACHMENT_LOAD_OP_CLEAR
        is 3. Note that the pClearValues array is indexed by attachment number so even if
        some pClearValues entries between 0 and 2 correspond to attachments that aren't
        cleared they will be ignored. The spec valid usage text states 'clearValueCount
        must be greater than the largest attachment index in renderPass that specifies a
        loadOp (or stencilLoadOp, if the attachment has a depth/stencil format) of
        VK_ATTACHMENT_LOAD_OP_CLEAR'
        (https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#VUID-VkRenderPassBeginInfo-clearValueCount-00902)
    */
    std::array<VkClearValue, 3> clearValues = {};
    // clearValuues[0] = final layout is in this spot; // pad this spot
    clearValues[1].color = CLEAR_VALUE;
    clearValues[2].depthStencil = {1.0f, 0};

    render_pass_info.clearValueCount = static_cast<uint32_t>(clearValues.size());
    render_pass_info.pClearValues = clearValues.data();

    // Start a new debug marker region
    ext::DebugMarkerBegin(cmd, "Render x scene", glm::vec4(0.2f, 0.3f, 0.4f, 1.0f));

    vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    if (!colorMeshes_.empty() || !lineMeshes_.empty()) {
        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        // **********************
        //  TRI_LIST_COLOR PIPELINE
        // **********************

        const auto& layout = PipelineHandler::getPipelineLayout(Vertex::TYPE::COLOR);
        const auto& descSet = getColorDescSet(frameIndex);

        if (!colorMeshes_.empty()) {
            const auto& pipeline = getPipeline(PipelineHandler::TOPOLOGY::TRI_LIST_COLOR);

            for (auto& pMesh : colorMeshes_) {
                if (pMesh->getStatus() == Mesh::STATUS::READY) {
                    pMesh->drawInline(cmd, layout, pipeline, descSet);
                }
            }
        }

        vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
        inherit_info.subpass++;

        // **********************
        //  LINE PIPELINE
        // **********************

        if (!lineMeshes_.empty()) {
            const auto& pipeline = getPipeline(PipelineHandler::TOPOLOGY::LINE);

            for (auto& pMesh : lineMeshes_) {
                if (pMesh->getStatus() == Mesh::STATUS::READY) {
                    pMesh->drawInline(cmd, layout, pipeline, descSet);
                }
            }
        }

    } else {
        // Nothing in the color pipeline ...
        vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
        inherit_info.subpass++;
    }

    vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    inherit_info.subpass++;

    // **********************
    //  TRI_LIST_TEX PIPELINE
    // **********************

    if (!texMeshes_.empty()) {
        // TODO: I don't like the search for the pipeline coming here.
        const auto& layout = PipelineHandler::getPipelineLayout(Vertex::TYPE::TEXTURE);
        const auto& pipeline = getPipeline(PipelineHandler::TOPOLOGY::TRI_LIST_TEX);

        for (auto& pMesh : texMeshes_) {
            if (pMesh->getStatus() == Mesh::STATUS::READY) {
                const auto& descSet = getTexDescSet(pMesh->getTextureOffset(), frameIndex);
                pMesh->drawSecondary(cmd, layout, pipeline, descSet, frameIndex, inherit_info, viewport, scissor);
            }
        }
    }

    vkCmdEndRenderPass(cmd);

    // End current debug marker region
    ext::DebugMarkerEnd(cmd);

    vk::assert_success(vkEndCommandBuffer(cmd));
}

void Scene::destroy_cmds(const VkDevice& dev) {
    for (auto& res : draw_resources_) {
        if (res.thread.joinable()) res.thread.join();
        vkFreeCommandBuffers(dev, CmdBufHandler::graphics_cmd_pool(), 1, &res.cmd);
    }
    draw_resources_.clear();
}

void Scene::destroy(const VkDevice& dev) {
    // mesh
    for (auto& pMesh : colorMeshes_) pMesh->destroy(dev);
    colorMeshes_.clear();
    // for (auto& pMesh : lineMeshes_) pMesh->destroy(dev);
    // lineMeshes_.clear();
    for (auto& pMesh : texMeshes_) pMesh->destroy(dev);
    texMeshes_.clear();
    // descriptor
    PipelineHandler::destroy_descriptor_resources(pDescResources_);
    // pipeline
    PipelineHandler::destroy_pipeline_resources(plResources_);
    // commands
    destroy_cmds(dev);
}