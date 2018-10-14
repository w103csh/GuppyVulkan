
#include <type_traits>

#include "CmdBufHandler.h"
#include "PipelineHandler.h"
#include "Scene.h"

Scene::Scene(const MyShell::Context& ctx, const Game::Settings& settings, const UniformBufferResources& ubo_res) : tex_count_(0) {
    // default descriptor resource values
    desc_resources_.pool = nullptr;
    desc_resources_.updatePool = true;
    desc_resources_.colorCount = ubo_res.count;  // shared description set type count!!!
    desc_resources_.texCount = 0;

    updateDescriptorResources(ctx, ubo_res.info);

    PipelineHandler::create_pipeline_resources(pipeline_resources_);

    create_draw_cmds(ctx);
}

void Scene::addMesh(const MyShell::Context& ctx, const VkDescriptorBufferInfo& ubo_info, std::unique_ptr<ColorMesh> pMesh) {
    pMesh->setSceneData(ctx, colorMeshes_.size());
    if (!pMesh->isReady()) {
        loading_futures_.emplace_back(pMesh->load(ctx));
    } else {
        pMesh->prepare(ctx, ubo_info, desc_resources_);
    }
    colorMeshes_.push_back(std::move(pMesh));
}

// void Scene::addMesh(const MyShell::Context& ctx, const VkDescriptorBufferInfo& ubo_info, std::unique_ptr<LineMesh> pMesh) {
//    pMesh->setOffset(lineMeshes_.size());
//    if (!pMesh->isReady()) {
//        loading_futures_.emplace_back(pMesh->load(ctx));
//    } else {
//        pMesh->prepare(ctx, ubo_info, desc_resources_);
//    }
//    lineMeshes_.push_back(std::move(pMesh));
//}

void Scene::addMesh(const MyShell::Context& ctx, const VkDescriptorBufferInfo& ubo_info, std::unique_ptr<TextureMesh> pMesh) {
    // TODO: this seems like overkill
    // Make space for the new sampler descriptor sets.
    desc_resources_.texCount++;
    updateDescriptorResources(ctx, ubo_info);

    pMesh->setSceneData(ctx, texMeshes_.size());
    if (!pMesh->isReady()) {
        loading_futures_.emplace_back(pMesh->load(ctx));
    } else {
        pMesh->prepare(ctx, ubo_info, desc_resources_);
    }
    texMeshes_.push_back(std::move(pMesh));
}

void Scene::removeMesh(Mesh* mesh) {
    // TODO...
    return;
}

bool Scene::update(const MyShell::Context& ctx, const VkDescriptorBufferInfo& ubo_info) {
    auto startCount = readyCount();

    // Check futures...
    auto it = loading_futures_.begin();
    while (it != loading_futures_.end()) {
        auto& fut = (*it);
        auto status = fut.wait_for(std::chrono::seconds(0));
        if (status == std::future_status::ready) {
            auto pMesh = fut.get();
            pMesh->prepare(ctx, ubo_info, desc_resources_);
            it = loading_futures_.erase(it);
        } else {
            ++it;
        }
    }

    //// Create or re-create descriptor layout/pool, and pipelines
    // if (tex_count != tex_count) {
    //    bool destroy = desc_set_layouts_.size() > 0;

    //    // Descriptors
    //    // Layouts
    //    if (destroy) destroy_descriptor_set_layouts(ctx.dev);
    //    create_descriptor_set_layouts(ctx);
    //    // Pool
    //    if (destroy) destroy_descriptor_pool(ctx.dev);
    //    create_descriptor_pool(ctx);

    //    // Pipelines (just recreate everything for now...)
    //    if (destroy) destroy_pipelines(ctx.dev);
    //    create_pipelines(ctx.dev, settings, vs, fs, cache);
    //}

    // TODO: This is bad. Should store descriptor sets on the scene and only record if a new
    // descriptor set is added... or something like that. Really this should be fast...
    return startCount != readyCount();
}

void Scene::updateDescriptorResources(const MyShell::Context& ctx, const VkDescriptorBufferInfo& ubo_info) {
    if (desc_resources_.pool != nullptr) {
        destroy_descriptor_resources(ctx.dev);
        desc_resources_.sharedSets.clear();
    }
    PipelineHandler::create_descriptor_pool(desc_resources_);
    createSharedDescriptorSets(ctx, ubo_info);
    desc_resources_.updatePool = false;
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
            // asnyc TODO: Can't update current draw until its done.. This isn't great. It can be revisited later.
            draw_resources_[i].thread = std::thread(&Scene::record, this, ctx, draw_resources_[i].cmd, framebuffers[i],
                                                    frame_data[i].fence, viewport, scissor, i, true);
        }
    }
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
    render_pass_info.renderPass = pipeline_resources_.renderPass;
    render_pass_info.framebuffer = framebuffer;
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = ctx.extent;

    // Inheritance info for secondary command buffers
    VkCommandBufferInheritanceInfo inherit_info = {};
    inherit_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inherit_info.renderPass = pipeline_resources_.renderPass;
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
    ext::DebugMarkerBegin(cmd, "Render x scene", glm::vec4(0.5f, 0.76f, 0.34f, 1.0f));

    vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdSetViewport(cmd, 0, 1, &viewport);
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    if (!colorMeshes_.empty() || !lineMeshes_.empty()) {
        // TODO: I don't like the search for the pipeline coming here.
        const auto& layout = PipelineHandler::getPipelineLayout(Vertex::TYPE::COLOR);
        const auto& descSet = getSharedDescriptorSet(frameIndex);

        if (!colorMeshes_.empty()) {
            // TODO: I don't like the search for the pipeline coming here.
            const auto& pipeline = getPipeline(PipelineHandler::TOPOLOGY::TRI_LIST_COLOR);

            for (auto& pMesh : colorMeshes_) {
                pMesh->draw(ctx.dev, layout, pipeline, descSet, frameIndex, inherit_info);
            }
        }

        vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        inherit_info.subpass++;

        if (!lineMeshes_.empty()) {
            // TODO: I don't like the search for the pipeline coming here.
            const auto& pipeline = getPipeline(PipelineHandler::TOPOLOGY::LINE);

            for (auto& pMesh : lineMeshes_) {
                pMesh->draw(ctx.dev, layout, pipeline, descSet, frameIndex, inherit_info);
            }
        }

    } else {
        // Nothing to draw ...
        vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        inherit_info.subpass++;
    }

    vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
    inherit_info.subpass++;

    if (!texMeshes_.empty()) {
        // TODO: I don't like the search for the pipeline coming here.
        const auto& layout = PipelineHandler::getPipelineLayout(Vertex::TYPE::TEXTURE);
        const auto& pipeline = getPipeline(PipelineHandler::TOPOLOGY::TRI_LIST_TEX);

        for (auto& pMesh : texMeshes_) {
            pMesh->draw2(ctx.dev, cmd, layout, pipeline, frameIndex, inherit_info, viewport, scissor);
        }
    }

    vkCmdEndRenderPass(cmd);

    // End current debug marker region
    ext::DebugMarkerEnd(cmd);
    //}

    vk::assert_success(vkEndCommandBuffer(cmd));
}

void Scene::createSharedDescriptorSets(const MyShell::Context& ctx, const VkDescriptorBufferInfo& ubo_info) {
    // DESCRIPTOR SETS
    std::vector<VkDescriptorSetLayout> layouts;
    // for (size_t i = 0; i < desc_resources_.sets.size(); i++) {
    //    auto& set = desc_resources_.sets[i];
    //    layouts.clear();
    PipelineHandler::getDescriptorLayouts(ctx.image_count, Vertex::TYPE::COLOR, layouts);
    auto setCount = static_cast<uint32_t>(layouts.size());

    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = desc_resources_.pool;
    alloc_info.descriptorSetCount = setCount;
    alloc_info.pSetLayouts = layouts.data();

    desc_resources_.sharedSets.resize(ctx.image_count);
    vk::assert_success(vkAllocateDescriptorSets(ctx.dev, &alloc_info, desc_resources_.sharedSets.data()));
    //}

    std::vector<VkWriteDescriptorSet> writes;
    VkWriteDescriptorSet write = {};
    // Default uniform buffer writes that are used for both shaders/vertex types
    for (size_t i = 0; i < ctx.image_count; i++) {
        write = {};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = desc_resources_.sharedSets[i];
        write.dstBinding = 0;
        write.dstArrayElement = 0;
        write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.descriptorCount = 1;
        write.pBufferInfo = &ubo_info;
        writes.push_back(write);
    }
    vkUpdateDescriptorSets(ctx.dev, 2, writes.data(), 0, nullptr);
}

//// Depends on:
////      ctx - surface_format.format
////      ctx - depth_format
////      settings - include_color
////      settings - include_depth
//// Can change:
////      Make subpasses dynamic
////      Make dependencies dynamic
////      Make attachments more dynamic
// void Scene::create_render_pass(const MyShell::Context& ctx, const Game::Settings& settings, bool clear, VkImageLayout
// finalLayout) {
//    // ATTACHMENTS
//    std::vector<VkAttachmentDescription> attachments;
//
//    // COLOR ATTACHMENT (SWAP-CHAIN)
//    VkAttachmentReference color_reference = {};
//    VkAttachmentDescription attachemnt = {};
//    attachemnt = {};
//    attachemnt.format = ctx.surface_format.format;
//    attachemnt.samples = VK_SAMPLE_COUNT_1_BIT;
//    attachemnt.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
//    attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//    attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//    attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//    attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//    attachemnt.finalLayout = finalLayout;
//    attachemnt.flags = 0;
//    attachments.push_back(attachemnt);
//    // REFERENCE
//    color_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);
//    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//    // COLOR ATTACHMENT RESOLVE (MULTI-SAMPLE)
//    VkAttachmentReference color_resolve_reference = {};
//    if (settings.include_color) {
//        attachemnt = {};
//        attachemnt.format = ctx.surface_format.format;
//        attachemnt.samples = ctx.num_samples;
//        attachemnt.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
//        attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//        attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//        attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//        attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//        attachemnt.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//        attachemnt.flags = 0;
//        // REFERENCE
//        color_resolve_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);  // point to swapchain attachment
//        color_resolve_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//        attachments.push_back(attachemnt);
//        color_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);  // point to multi-sample attachment
//    }
//
//    // DEPTH ATTACHMENT
//    VkAttachmentReference depth_reference = {};
//    if (settings.include_depth) {
//        attachemnt = {};
//        attachemnt.format = ctx.depth_format;
//        attachemnt.samples = ctx.num_samples;
//        attachemnt.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
//        // This was "don't care" in the sample and that makes more sense to
//        // me. This obvious is for some kind of stencil operation.
//        attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//        attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
//        attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
//        attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//        attachemnt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//        attachemnt.flags = 0;
//        attachments.push_back(attachemnt);
//        // REFERENCE
//        depth_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);
//        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//    }
//
//    // SUBPASSES
//    std::vector<VkSubpassDescription> subpasses;
//    VkSubpassDescription subpass = {};
//
//    subpass = {};
//    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//    subpass.inputAttachmentCount = 0;
//    subpass.pInputAttachments = nullptr;
//    subpass.colorAttachmentCount = 1;
//    subpass.pColorAttachments = &color_reference;
//    subpass.pResolveAttachments = settings.include_color ? &color_resolve_reference : nullptr;
//    subpass.pDepthStencilAttachment = settings.include_depth ? &depth_reference : nullptr;
//    subpass.preserveAttachmentCount = 0;
//    subpass.pPreserveAttachments = nullptr;
//    subpasses.push_back(subpass);
//
//    // Line drawing subpass (just need an identical subpass for now)
//    // subpasses.push_back(subpass);
//
//    // DEPENDENCIES
//    std::vector<VkSubpassDependency> dependencies;
//    VkSubpassDependency dependency = {};
//
//    //// TODO: used for waiting in draw (figure this out... what is the VK_SUBPASS_EXTERNAL one?)
//    // dependency = {};
//    // dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
//    // dependency.dstSubpass = 0;
//    // dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//    // dependency.srcAccessMask = 0;
//    // dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//    // dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//    // dependencies.push_back(dependency);
//
//    // TODO: fix this...
//    // dependency = {};
//    // dependency.srcSubpass = 0;
//    // dependency.dstSubpass = 1;
//    // dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
//    // dependency.srcAccessMask = 0;
//    // dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
//    // dependency.dstAccessMask = 0;
//    // dependencies.push_back(dependency);
//
//    VkRenderPassCreateInfo rp_info = {};
//    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//    rp_info.pNext = nullptr;
//    // Attachmments
//    uint32_t attachmentCount = static_cast<uint32_t>(attachments.size());
//    rp_info.attachmentCount = attachmentCount;
//    rp_info.pAttachments = attachmentCount > 0 ? attachments.data() : nullptr;
//    // Subpasses
//    uint32_t subpassCount = static_cast<uint32_t>(subpasses.size());
//    rp_info.subpassCount = subpassCount;
//    rp_info.pSubpasses = subpassCount > 0 ? subpasses.data() : nullptr;
//    // Dependencies
//    uint32_t dependencyCount = static_cast<uint32_t>(dependencies.size());
//    rp_info.dependencyCount = dependencyCount;
//    rp_info.pDependencies = dependencyCount > 0 ? dependencies.data() : nullptr;
//
//    VkRenderPass render_pass;
//    vk::assert_success(vkCreateRenderPass(ctx.dev, &rp_info, nullptr, &render_pass));
//    render_passes_.emplace_back(render_pass);
//}

// void Scene::destroy_pipeline_layouts(const VkDevice& dev) { vkDestroyPipelineLayout(dev, layout_, nullptr); }

// void Scene::create_pipelines(const VkDevice& dev, const Game::Settings& settings, const ShaderResources& vs,
//                             const ShaderResources& fs, const VkPipelineCache& cache) {
//    pipeline_info_.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
//    pipeline_info_.layout = layout_;
//    pipeline_info_.basePipelineHandle = base_pipeline();
//    pipeline_info_.basePipelineIndex = -1;
//
//    VkPipelineInputAssemblyStateCreateInfo input_info = {};
//    input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//    input_info.primitiveRestartEnable = VK_FALSE;
//
//    // Start from BASE + 1
//    for (size_t i = 1; i < pipelines_.size(); i++) {
//        // Do whatever is different...
//
//        switch (i) {
//            case static_cast<size_t>(PIPELINE_TYPE::POLY): {
//                input_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//                pipeline_info_.pInputAssemblyState = &input_info;
//                pipeline_info_.layout = layout_;
//
//                pipeline_info_.subpass = 0;
//            } break;
//            case static_cast<size_t>(PIPELINE_TYPE::LINE): {
//                input_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
//                pipeline_info_.pInputAssemblyState = &input_info;
//                pipeline_info_.layout = layout_;
//
//                pipeline_info_.subpass = 1;
//            } break;
//        }
//
//        vk::assert_success(vkCreateGraphicsPipelines(dev, cache, 1, &pipeline_info_, nullptr, &pipelines_[i]));
//    }
//}

void Scene::destroy_descriptor_resources(const VkDevice& dev) { vkDestroyDescriptorPool(dev, desc_resources_.pool, nullptr); }

//// TODO: fix this
// void Scene::destroy_pipelines(const VkDevice& dev, bool destroy_base) {
//    // for (size_t i = 0; i < pipelines_.size(); i++) {
//    // for (size_t i = 0; i < 1; i++) {
//    //    if (i == static_cast<size_t>(PIPELINE_TYPE::BASE) && !destroy_base) {
//    //        continue;  // really...
//    //    } else {
//    //        vkDestroyPipeline(dev, pipelines_[i], nullptr);
//    //    }
//    //}
//}
//
// void Scene::destroy_render_passes(const VkDevice& dev) {
//    for (auto& pass : render_passes_) vkDestroyRenderPass(dev, pass, nullptr);
//}

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
    destroy_descriptor_resources(dev);
    // pipeline
    PipelineHandler::destroy_pipeline_resources(pipeline_resources_);
    // commands
    destroy_cmds(dev);
}