
#include "CmdBufHandler.h"
#include "PipelineHandler.h"
#include "Scene.h"
#include "SceneHandler.h"
#include "TextureHandler.h"
#include "UIHandler.h"

namespace {

struct UBOTag {
    const char name[17] = "ubo tag";
} uboTag;

}  // namespace

Scene::Scene(size_t offset)
    : offset_(offset),
      pDescResources_(nullptr),
      plResources_(),
      pDynUBOResource_(nullptr) {}

void Scene::createDynamicTexUniformBuffer(const Shell::Context& ctx, const Game::Settings& settings,
                                          std::string markerName) {
    const auto& limits = ctx.physical_dev_props[ctx.physical_dev_index].properties.limits;

    // this is just a single flag for now...
    VkDeviceSize dynBuffSize = sizeof(FlagBits);

    if (limits.minUniformBufferOffsetAlignment)
        dynBuffSize =
            (dynBuffSize + limits.minUniformBufferOffsetAlignment - 1) & ~(limits.minUniformBufferOffsetAlignment - 1);

    pDynUBOResource_ = std::make_shared<UniformBufferResources>();
    pDynUBOResource_->count = TextureHandler::getCount();
    pDynUBOResource_->info.offset = 0;
    pDynUBOResource_->info.range = dynBuffSize;

    pDynUBOResource_->size =
        helpers::createBuffer(ctx.dev, (dynBuffSize * pDynUBOResource_->count), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              pDynUBOResource_->buffer, pDynUBOResource_->memory);

    void* pData;
    size_t offset = 0;
    vk::assert_success(vkMapMemory(ctx.dev, pDynUBOResource_->memory, 0, pDynUBOResource_->size, 0, &pData));

    for (const auto& pTex : TextureHandler::getTextures()) {
        memcpy(static_cast<char*>(pData) + offset, &pTex->flags, dynBuffSize);
        offset += static_cast<size_t>(dynBuffSize);
    }

    vkUnmapMemory(ctx.dev, pDynUBOResource_->memory);

    pDynUBOResource_->info.buffer = pDynUBOResource_->buffer;

    if (settings.enable_debug_markers) {
        if (markerName.empty()) markerName += "Default";
        markerName += " dynamic texture uniform buffer block";
        ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)pDynUBOResource_->buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                      markerName.c_str());
        ext::DebugMarkerSetObjectTag(ctx.dev, (uint64_t)pDynUBOResource_->buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0,
                                     sizeof(uboTag), &uboTag);
    }
}

std::unique_ptr<ColorMesh>& Scene::moveMesh(const Game::Settings& settings, const Shell::Context& ctx,
                                            std::unique_ptr<ColorMesh> pMesh) {
    assert(pMesh->getStatus() == STATUS::PENDING_BUFFERS);

    auto offset = colorMeshes_.size();
    pMesh->setSceneData(offset);

    colorMeshes_.push_back(std::move(pMesh));
    colorMeshes_.back()->prepare(settings, ctx.dev, pDescResources_);

    return colorMeshes_.back();
}

std::unique_ptr<ColorMesh>& Scene::moveMesh(const Game::Settings& settings, const Shell::Context& ctx,
                                            std::unique_ptr<LineMesh> pMesh) {
    assert(pMesh->getStatus() == STATUS::PENDING_BUFFERS);

    auto offset = lineMeshes_.size();
    pMesh->setSceneData(offset);

    lineMeshes_.push_back(std::move(pMesh));
    lineMeshes_.back()->prepare(settings, ctx.dev, pDescResources_);

    return lineMeshes_.back();
}

std::unique_ptr<TextureMesh>& Scene::moveMesh(const Game::Settings& settings, const Shell::Context& ctx,
                                              std::unique_ptr<TextureMesh> pMesh) {
    assert(pMesh->getStatus() == STATUS::PENDING_BUFFERS);

    auto offset = texMeshes_.size();
    pMesh->setSceneData(ctx, offset);
    texMeshes_.push_back(std::move(pMesh));

    // Update descriptors for all the meshes stored
    if (isNewTexture(texMeshes_.back())) {
        SceneHandler::updateDescriptorResources(offset_);
    }

    texMeshes_.back()->prepare(settings, ctx.dev, pDescResources_);

    return texMeshes_.back();
}

size_t Scene::addMesh(const Game::Settings& settings, const Shell::Context& ctx, std::unique_ptr<ColorMesh> pMesh,
                      bool async, std::function<void(Mesh*)> callback) {
    auto offset = colorMeshes_.size();

    pMesh->setSceneData(offset);
    colorMeshes_.push_back(std::move(pMesh));

    if (colorMeshes_.back()->getStatus() == STATUS::READY) {
        colorMeshes_.back()->prepare(settings, ctx.dev, pDescResources_);
    } else {
        auto fut = colorMeshes_.back()->load(ctx, callback);

        // If async is specified then store the future, and get it later
        if (async) {
            ldgFutures_.emplace_back(std::move(fut));
        } else {
            fut.get()->prepare(settings, ctx.dev, pDescResources_);
        }
    }

    return offset;
}

size_t Scene::addMesh(const Game::Settings& settings, const Shell::Context& ctx, std::unique_ptr<LineMesh> pMesh, bool async,
                      std::function<void(Mesh*)> callback) {
    auto offset = lineMeshes_.size();

    pMesh->setSceneData(offset);
    lineMeshes_.push_back(std::move(pMesh));

    if (lineMeshes_.back()->getStatus() == STATUS::READY) {
        lineMeshes_.back()->prepare(settings, ctx.dev, pDescResources_);
    } else {
        auto fut = lineMeshes_.back()->load(ctx, callback);

        // If async is specified then store the future, and get it later
        if (async) {
            ldgFutures_.emplace_back(std::move(fut));
        } else {
            fut.get()->prepare(settings, ctx.dev, pDescResources_);
        }
    }

    return offset;
}

size_t Scene::addMesh(const Game::Settings& settings, const Shell::Context& ctx, std::unique_ptr<TextureMesh> pMesh,
                      bool async, std::function<void(Mesh*)> callback) {
    auto offset = texMeshes_.size();

    // Set values based on scene, and move pointer onto scene
    // TODO: remove scene information from the mesh if mesh becomes shared
    pMesh->setSceneData(ctx, offset);
    texMeshes_.push_back(std::move(pMesh));

    // Update descriptors for all the meshes stored
    if (isNewTexture(texMeshes_.back())) {
        SceneHandler::updateDescriptorResources(offset_);
    }

    // Check mesh loading status
    if (texMeshes_.back()->getStatus() == STATUS::READY) {
        texMeshes_.back()->prepare(settings, ctx.dev, pDescResources_);
    } else {
        auto fut = texMeshes_.back()->load(ctx, callback);

        // If async is specified then store the future, and get it later
        if (async) {
            ldgFutures_.emplace_back(std::move(fut));
        } else {
            fut.get()->prepare(settings, ctx.dev, pDescResources_);
        }
    }

    return offset;
}

void Scene::removeMesh(std::unique_ptr<Mesh>& pMesh) {
    // TODO...
    assert(false);
}

void Scene::update(const Game::Settings& settings, const Shell::Context& ctx) {
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
                pMesh->prepare(settings, ctx.dev, pDescResources_);

                // Remove the future from the list if all goes well.
                itFut = ldgFutures_.erase(itFut);

            } else {
                ++itFut;
            }
        }
    }

    // TODO: fix all this ready stuff
    for (auto& pMesh : texMeshes_) {
        if (pMesh->getStatus() == STATUS::PENDING_TEXTURE) {
            pMesh->tryCreateDescriptorSets(pDescResources_);
        }
    }
}

void Scene::updatePipeline(PIPELINE_TYPE type) { PipelineHandler::createPipeline(type, plResources_); }

// TODO: make cmd amount dynamic
const VkCommandBuffer& Scene::getDrawCmds(const uint8_t& frame_data_index, uint32_t& commandBufferCount) {
    auto& res = drawResources_[frame_data_index];
    if (res.thread.joinable()) res.thread.join();
    commandBufferCount = 1;  // !hardcode
    return res.cmd;
}

void Scene::createDrawCmds(const Shell::Context& ctx) {
    drawResources_.resize(ctx.image_count);

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = CmdBufHandler::graphics_cmd_pool();
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    for (auto& res : drawResources_) {
        vk::assert_success(vkAllocateCommandBuffers(ctx.dev, &alloc_info, &res.cmd));
    }
}

void Scene::record(const Shell::Context& ctx, const VkFramebuffer& framebuffer, const VkFence& fence,
                   const VkViewport& viewport, const VkRect2D& scissor, uint8_t frameIndex, bool wait) {
    auto& cmd = drawResources_[frameIndex].cmd;

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
            const auto& pipeline = getPipeline(PIPELINE_TYPE::TRI_LIST_COLOR);

            for (auto& pMesh : colorMeshes_) {
                if (pMesh->getStatus() == STATUS::READY) {
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
            const auto& pipeline = getPipeline(PIPELINE_TYPE::LINE);

            for (auto& pMesh : lineMeshes_) {
                if (pMesh->getStatus() == STATUS::READY) {
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
        const auto& pipeline = getPipeline(PIPELINE_TYPE::TRI_LIST_TEX);

        for (auto& pMesh : texMeshes_) {
            if (pMesh->getStatus() == STATUS::READY) {
                // descriptor set
                const auto& descSet = getTexDescSet(pMesh->getTextureOffset(), frameIndex);
                // dynamic uniform buffer offset
                uint32_t offset = static_cast<uint32_t>(pDescResources_->dynUboInfos[0].range) * pMesh->getTextureOffset();
                std::array<uint32_t, 1> dynUboOffsets = {offset};

                pMesh->drawSecondary(cmd, layout, pipeline, descSet, dynUboOffsets, frameIndex, inherit_info, viewport,
                                     scissor);
            }
        }
    }

    // **********************
    //  UI PIPELINE
    // **********************

    vkCmdNextSubpass(cmd, VK_SUBPASS_CONTENTS_INLINE);
    UIHandler::draw(cmd, frameIndex);

    vkCmdEndRenderPass(cmd);

    // End current debug marker region
    ext::DebugMarkerEnd(cmd);

    vk::assert_success(vkEndCommandBuffer(cmd));
}

void Scene::destroyCmds(const VkDevice& dev) {
    for (auto& res : drawResources_) {
        if (res.thread.joinable()) res.thread.join();
        vkFreeCommandBuffers(dev, CmdBufHandler::graphics_cmd_pool(), 1, &res.cmd);
    }
    drawResources_.clear();
}

void Scene::destroyUniforms(const VkDevice& dev) {
    vkDestroyBuffer(dev, pDynUBOResource_->buffer, nullptr);
    vkFreeMemory(dev, pDynUBOResource_->memory, nullptr);
}

void Scene::destroy(const VkDevice& dev) {
    // mesh
    for (auto& pMesh : colorMeshes_) pMesh->destroy(dev);
    colorMeshes_.clear();
    for (auto& pMesh : lineMeshes_) pMesh->destroy(dev);
    lineMeshes_.clear();
    for (auto& pMesh : texMeshes_) pMesh->destroy(dev);
    texMeshes_.clear();
    // uniforms
    destroyUniforms(dev);
    // descriptor
    SceneHandler::destroyDescriptorResources(pDescResources_);
    // pipeline
    PipelineHandler::destroyPipelineResources(plResources_);
    // commands
    destroyCmds(dev);
}