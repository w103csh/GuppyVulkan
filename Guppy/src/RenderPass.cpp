
#include "RenderPass.h"

#include "Helpers.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "DescriptorHandler.h"
#include "MaterialHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

RenderPass::Base::Base(RenderPass::Handler& handler, const uint32_t&& offset, const RenderPass::CreateInfo* pCreateInfo)
    : Handlee(handler),
      NAME(pCreateInfo->name),
      OFFSET(offset),
      FLAGS(pCreateInfo->flags),
      TYPE(pCreateInfo->type),
      pass(VK_NULL_HANDLE),
      data{},
      status_(STATUS::UNKNOWN),
      pipelineData_{
          usesDepth(),
          VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM,
      },
      scissor_{},
      viewport_{},
      beginInfo_{},
      // SECONDARY
      inheritInfo_{},
      secCmdBeginInfo_{},
      secCmdFlag_(false),
      depth_{},
      descPipelineOffsets_(pCreateInfo->descPipelineOffsets),
      textureIds_(pCreateInfo->textureIds),
      isInitialized_(false),
      // SETTINGS
      format_(VK_FORMAT_UNDEFINED),
      depthFormat_(VK_FORMAT_UNDEFINED),
      finalLayout_(pCreateInfo->finalLayout),
      initialLayout_(pCreateInfo->initialLayout),
      commandCount_(0),
      semaphoreCount_(0),
      extent_(BAD_EXTENT_2D) {
    // This is sloppy, but I don't really feel like changing a bunch of things.
    if (textureIds_.empty()) textureIds_.emplace_back(std::string(SWAPCHAIN_TARGET_ID));
    assert(textureIds_.size());

    // Create dependency list. Includes itself in order.
    for (const auto& passType : pCreateInfo->prePassTypes) dependentTypeOffsetPairs_.push_back({passType, BAD_OFFSET});
    dependentTypeOffsetPairs_.push_back({TYPE, BAD_OFFSET});
    for (const auto& passType : pCreateInfo->postPassTypes) dependentTypeOffsetPairs_.push_back({passType, BAD_OFFSET});

    for (const auto& pipelineType : pCreateInfo->pipelineTypes) {
        // If changed from set then a lot of work needs to be done, so I am
        // adding some validation here.
        assert(pipelineTypeBindDataMap_.count(pipelineType) == 0);
        pipelineTypeBindDataMap_.insert({pipelineType, {}});
    }
}

void RenderPass::Base::init() {
    assert(!isInitialized_);

    auto& ctx = handler().shell().context();

    // Find the texture targets if not the swapchain.
    if (getTargetId() != RenderPass::SWAPCHAIN_TARGET_ID) {
        for (const auto& textureId : textureIds_) {
            auto pTexture = handler().textureHandler().getTexture(textureId);
            if (pTexture != nullptr) {
                pTextures_.push_back(pTexture);
            } else {
                uint8_t frameIndex = 0;
                do {
                    pTexture = handler().textureHandler().getTexture(textureId, frameIndex++);
                    if (pTexture != nullptr) pTextures_.push_back(pTexture);
                } while (pTexture != nullptr);
            }
        }
        assert(pTextures_.size());
    }

    if (hasTargetSampler()) {
        assert(pTextures_.size());
        if (usesSwapchain()) {
            if (pTextures_[0]->samplers[0].imgCreateInfo.format == VK_FORMAT_UNDEFINED)
                format_ = ctx.surfaceFormat.format;
            else
                format_ = pTextures_[0]->samplers[0].imgCreateInfo.format;
        } else {
            // Force only one sampler, for now, and just assume its the target.
            for (auto i = 0; i < pTextures_.size(); i++) {
                assert(pTextures_[i]->samplers.size() == 1);
                if (i == 0) {
                    extent_.width = pTextures_[i]->samplers[0].imgCreateInfo.extent.width;
                    extent_.height = pTextures_[i]->samplers[0].imgCreateInfo.extent.height;
                    format_ = pTextures_[i]->samplers[0].imgCreateInfo.format;
                } else {
                    assert(helpers::compExtent2D(extent_, pTextures_[i]->samplers[0].imgCreateInfo.extent));
                    assert(format_ = pTextures_[i]->samplers[0].imgCreateInfo.format);
                }
            }
        }
    } else {
        format_ = ctx.surfaceFormat.format;
    }

    depthFormat_ = usesDepth() ? ctx.depthFormat : VK_FORMAT_UNDEFINED;
    pipelineData_.samples = usesMultiSample() ? ctx.samples : VK_SAMPLE_COUNT_1_BIT;

    // Validate frame settings.
    assert(initialLayout_ != VK_IMAGE_LAYOUT_UNDEFINED);  // I think this is the case.
    assert(finalLayout_ != VK_IMAGE_LAYOUT_UNDEFINED);
    if (pipelineData_.usesDepth)
        assert(depthFormat_ != VK_FORMAT_UNDEFINED);
    else
        assert(depthFormat_ == VK_FORMAT_UNDEFINED);

    // SYNC
    commandCount_ = ctx.imageCount;

    // CLEAR
    if (handler().clearTargetMap_.count(getTargetId()) == 0)  //
        handler().clearTargetMap_[getTargetId()] = TYPE;
    // FINAL
    handler().finalTargetMap_[getTargetId()] = TYPE;

    isInitialized_ = true;
}

void RenderPass::Base::create() {
    assert(isInitialized_);

    // Update some settings based on the clear map.
    if (hasTargetSwapchain() && handler().isFinalTargetPass(getTargetId(), TYPE)) {
        finalLayout_ = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    }
    // Not sure if this makes sense anymore
    if (finalLayout_ != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        semaphoreCount_ = handler().shell().context().imageCount;
        data.signalSrcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }

    createAttachments();
    createSubpassDescriptions();
    createDependencies();

    createPass();
    assert(pass != VK_NULL_HANDLE);

    createBeginInfo();

    if (handler().settings().enable_debug_markers) {
        std::string markerName = NAME + " render pass";
        ext::DebugMarkerSetObjectName(handler().shell().context().dev, (uint64_t)pass, VK_DEBUG_REPORT_OBJECT_TYPE_PASS_EXT,
                                      markerName.c_str());
    }
}

void RenderPass::Base::setSwapchainInfo() {
    assert(helpers::compExtent2D(extent_, BAD_EXTENT_2D));
    extent_ = handler().shell().context().extent;
}

void RenderPass::Base::createTarget() {
    if (usesSwapchain()) setSwapchainInfo();

    // FRAME
    createImageResources();
    if (usesDepth()) createDepthResource();
    createAttachmentDebugMarkers();
    updateClearValues();
    createFramebuffers();

    // RENDER PASS
    createCommandBuffers();
    createViewport();
    updateBeginInfo();

    // SYNC
    createSemaphores();

    // Validate signal semaphore creation. If semaphores are created
    // the mask should be set.
    if (data.semaphores.empty())
        assert(data.signalSrcStageMask == VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM);
    else
        assert(data.signalSrcStageMask != VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM);

    assert(data.priCmds.size() + data.secCmds.size() <= RESOURCE_SIZE);
    assert(data.semaphores.size() <= RESOURCE_SIZE);
}

void RenderPass::Base::overridePipelineCreateInfo(const PIPELINE& type, Pipeline::CreateInfoResources& createInfoRes) {
    assert(pipelineData_.samples != VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM);
    createInfoRes.depthStencilStateInfo.depthTestEnable = pipelineData_.usesDepth;
    createInfoRes.depthStencilStateInfo.depthWriteEnable = pipelineData_.usesDepth;
    createInfoRes.multisampleStateInfo.rasterizationSamples = pipelineData_.samples;
}

void RenderPass::Base::record(const uint8_t frameIndex) {
    // FRAME UPDATE
    beginInfo_.framebuffer = data.framebuffers[frameIndex];
    auto& priCmd = data.priCmds[frameIndex];

    // RESET BUFFERS
    vkResetCommandBuffer(priCmd, 0);

    // BEGIN BUFFERS
    VkCommandBufferBeginInfo bufferInfo = {};
    // PRIMARY
    bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vk::assert_success(vkBeginCommandBuffer(priCmd, &bufferInfo));

    beginPass(priCmd, frameIndex, VK_SUBPASS_CONTENTS_INLINE);
    // pPass->beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    auto& secCmd = data.secCmds[frameIndex];
    auto& pScene = handler().sceneHandler().getActiveScene();

    auto it = pipelineTypeBindDataMap_.begin();
    while (it != pipelineTypeBindDataMap_.end()) {
        pScene->record(TYPE, it->first, it->second, priCmd, secCmd, frameIndex);

        ++it;

        if (it != pipelineTypeBindDataMap_.end()) {
            vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
            // vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        }
    }

    endPass(priCmd);

    // vk::assert_success(vkEndCommandBuffer(priCmd));
}

void RenderPass::Base::update() {
    assert(status_ & STATUS::PENDING_PIPELINE);

    bool isReady = true;
    for (const auto& [pipelineType, value] : pipelineTypeBindDataMap_) {
        if (Pipeline::MESHLESS.count(pipelineType) == 0) continue;

        // Get pipeline bind data, and check the status
        auto& pPipeline = handler().pipelineHandler().getPipeline(pipelineType);

        // If the pipeline is not ready try to update once.
        if (pPipeline->getStatus() != STATUS::READY) pPipeline->updateStatus();

        if (pPipeline->getStatus() == STATUS::READY) {
            if (pipelineDescSetBindDataMap_.count(pipelineType) == 0) {
                auto it = pipelineDescSetBindDataMap_.insert({pipelineType, {}});
                assert(it.second);

                // Get or make descriptor bind data.
                handler().descriptorHandler().getBindData(pipelineType, it.first->second, nullptr, nullptr);
                assert(it.first->second.size());
            }
        }

        isReady &= pPipeline->getStatus() == STATUS::READY;
    }

    if (isReady) {
        status_ ^= STATUS::PENDING_PIPELINE;
        assert(status_ == STATUS::READY);
    }
}

void RenderPass::Base::beginPass(const VkCommandBuffer& cmd, const uint8_t frameIndex,
                                 VkSubpassContents&& subpassContents) const {
    // TODO: remove the data member.
    const_cast<VkRenderPassBeginInfo*>(&beginInfo_)->framebuffer = data.framebuffers[frameIndex];
    //// Start a new debug marker region
    // ext::DebugMarkerBegin(primaryCmd, "Render x scene", glm::vec4(0.2f, 0.3f, 0.4f, 1.0f));
    vkCmdBeginRenderPass(cmd, &beginInfo_, subpassContents);
    // Frame commands
    vkCmdSetScissor(cmd, 0, 1, &scissor_);
    vkCmdSetViewport(cmd, 0, 1, &viewport_);
}

void RenderPass::Base::endPass(const VkCommandBuffer& cmd) const {
    vkCmdEndRenderPass(cmd);
    //// End current debug marker region
    // ext::DebugMarkerEnd(cmd);
    // vk::assert_success(vkEndCommandBuffer(cmd));
}

void RenderPass::Base::createPass() {
    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.pNext = nullptr;
    // ATTACHMENTS
    createInfo.attachmentCount = static_cast<uint32_t>(resources_.attachments.size());
    createInfo.pAttachments = resources_.attachments.data();
    // SUBPASSES
    createInfo.subpassCount = static_cast<uint32_t>(resources_.subpasses.size());
    createInfo.pSubpasses = resources_.subpasses.data();
    // DEPENDENCIES
    createInfo.dependencyCount = static_cast<uint32_t>(resources_.dependencies.size());
    createInfo.pDependencies = resources_.dependencies.data();

    vk::assert_success(vkCreateRenderPass(handler().shell().context().dev, &createInfo, nullptr, &pass));
}

void RenderPass::Base::createBeginInfo() {
    beginInfo_ = {};
    beginInfo_.sType = VK_STRUCTURE_TYPE_PASS_BEGIN_INFO;
    beginInfo_.renderPass = pass;

    if (usesSecondaryCommands()) {
        // INHERITANCE
        inheritInfo_ = {};
        inheritInfo_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritInfo_.renderPass = pass;
        inheritInfo_.subpass = getSubpassId(PIPELINE::TRI_LIST_TEX);
        // Validation layer: Cannot set inherited occlusionQueryEnable in vkBeginCommandBuffer() when device does not
        // support inheritedQueries.
        inheritInfo_.occlusionQueryEnable = VK_FALSE;  // TODO: not sure
        inheritInfo_.queryFlags = 0;                   // TODO: not sure
        inheritInfo_.pipelineStatistics = 0;           // TODO: not sure

        // BUFFER
        secCmdBeginInfo_ = {};
        secCmdBeginInfo_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        secCmdBeginInfo_.pNext = nullptr;
        secCmdBeginInfo_.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_PASS_CONTINUE_BIT;
        secCmdBeginInfo_.pInheritanceInfo = &inheritInfo_;
    }
}

void RenderPass::Base::createAttachments() {
    // DEPTH ATTACHMENT
    if (pipelineData_.usesDepth) {
        resources_.attachments.push_back({});
        resources_.attachments.back().format = depthFormat_;
        resources_.attachments.back().samples = getSamples();
        resources_.attachments.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        resources_.attachments.back().storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        resources_.attachments.back().stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        resources_.attachments.back().stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        resources_.attachments.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        resources_.attachments.back().finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        resources_.attachments.back().flags = 0;

        // REFERENCE
        resources_.depthStencilAttachment = {
            static_cast<uint32_t>(resources_.attachments.size() - 1),
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
    }

    // COLOR ATTACHMENT (MULTI-SAMPLE)
    if (usesMultiSample()) {
        resources_.attachments.push_back({});
        resources_.attachments.back().format = format_;
        resources_.attachments.back().samples = getSamples();
        resources_.attachments.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        resources_.attachments.back().storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        // TODO: Are these stencil ops right?
        resources_.attachments.back().stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resources_.attachments.back().stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        resources_.attachments.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        resources_.attachments.back().finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        resources_.attachments.back().flags = 0;

        // REFERENCE
        resources_.colorAttachments.push_back({
            static_cast<uint32_t>(resources_.attachments.size() - 1),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });
    }

    bool isClear = handler().isClearTargetPass(getTargetId(), TYPE);

    // COLOR ATTACHMENT
    resources_.attachments.push_back({});
    resources_.attachments.back().format = format_;
    resources_.attachments.back().samples = usesMultiSample() ? VK_SAMPLE_COUNT_1_BIT : getSamples();
    resources_.attachments.back().loadOp = isClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    resources_.attachments.back().storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resources_.attachments.back().stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resources_.attachments.back().stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resources_.attachments.back().initialLayout = isClear ? VK_IMAGE_LAYOUT_UNDEFINED : initialLayout_;
    resources_.attachments.back().finalLayout = finalLayout_;
    resources_.attachments.back().flags = 0;

    // REFERENCE
    if (usesMultiSample()) {
        // Set resolve to swapchain attachment
        resources_.resolveAttachments.push_back({
            static_cast<uint32_t>(resources_.attachments.size() - 1),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });
    } else {
        resources_.colorAttachments.push_back({
            static_cast<uint32_t>(resources_.attachments.size() - 1),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });
    }
}

void RenderPass::Base::createSubpassDescriptions() {
    // TODO: below comes from pipeline ???

    // SUBPASS DESCRIPTION
    VkSubpassDescription subpass = {};
    subpass.colorAttachmentCount = static_cast<uint32_t>(resources_.colorAttachments.size());
    subpass.pColorAttachments = resources_.colorAttachments.data();
    subpass.pResolveAttachments = resources_.resolveAttachments.data();
    subpass.pDepthStencilAttachment = pipelineData_.usesDepth ? &resources_.depthStencilAttachment : nullptr;

    resources_.subpasses.assign(getPipelineCount(), subpass);
}

void RenderPass::Base::createDependencies() {
    // 7.1. Render Pass Creation has examples of what this should be.
    VkSubpassDependency dependency;
    for (uint32_t i = 0; i < pipelineTypeBindDataMap_.size() - 1; i++) {
        dependency = {};
        dependency.srcSubpass = i;
        dependency.dstSubpass = i + 1;
        dependency.dependencyFlags = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        dependency.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dependency.srcAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        resources_.dependencies.push_back(dependency);
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        resources_.dependencies.push_back(dependency);
    }

    //// Desparate attempt to understand the pipline ordering issue below...
    // VkSubpassDependency dependency = {};

    // dependency = {};
    // dependency.srcSubpass = 0;
    // dependency.dstSubpass = 2;
    // dependency.dependencyFlags = 0;
    // dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    // dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    // dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
    // VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT; dependency.srcAccessMask =
    // VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    // resources.dependencies.push_back(dependency);

    // dependency = {};
    // dependency.srcSubpass = 1;
    // dependency.dstSubpass = 2;
    // dependency.dependencyFlags = 0;
    // dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    // dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    // dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
    // VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT; dependency.srcAccessMask =
    // VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    // resources.dependencies.push_back(dependency);
}

void RenderPass::Base::createCommandBuffers() {
    // PRIMARY
    data.priCmds.resize(commandCount_);
    handler().commandHandler().createCmdBuffers(QUEUE::GRAPHICS, data.priCmds.data(), VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                                commandCount_);
    if (usesSecondaryCommands() || true) {  // TODO: these things are never used
        // SECONDARY
        data.secCmds.resize(commandCount_);
        handler().commandHandler().createCmdBuffers(QUEUE::GRAPHICS, data.secCmds.data(), VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                                                    commandCount_);
    }
}

void RenderPass::Base::createImageResources() {
    auto& ctx = handler().shell().context();

    if (hasTargetSampler()) {
        for (auto& pTexture : pTextures_) {
            // Force only one sampler, for now, and just assume its the target.
            assert(pTexture->samplers.size() == 1);
            auto& sampler = pTexture->samplers.back();
            if (!usesSwapchain()) {
                // I did not test or think this through. This might require postponing
                // texture creation to here like the swapchain dependent samplers above.
                assert(!usesMultiSample());
            }
            assert(pTexture->status == STATUS::READY);
            assert((sampler.imgCreateInfo.usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) > 0);
        }
    } else {
        assert(pTextures_.empty());
    }

    if (usesMultiSample()) {
        assert(resources_.resolveAttachments.size() == 1 && "Make sure multi-sampling attachment reference was added");

        images_.push_back({});

        helpers::createImage(ctx.dev, ctx.memProps,
                             handler().commandHandler().getUniqueQueueFamilies(true, false, true, false),
                             pipelineData_.samples, format_, VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, extent_.width, extent_.height, 1, 1, images_.back().image,
                             images_.back().memory);

        VkImageSubresourceRange range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        helpers::createImageView(ctx.dev, images_.back().image, format_, VK_IMAGE_VIEW_TYPE_2D, range, images_.back().view);
    } else {
        assert(resources_.resolveAttachments.size() == 0 && "Make sure multi-sampling attachment reference was not added");
    }
}

void RenderPass::Base::createDepthResource() {
    auto& ctx = handler().shell().context();
    if (pipelineData_.usesDepth) {
        VkImageTiling tiling;
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(ctx.physicalDev, depthFormat_, &props);

        if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            tiling = VK_IMAGE_TILING_LINEAR;
        } else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            tiling = VK_IMAGE_TILING_OPTIMAL;
        } else {
            // Try other depth formats
            throw std::runtime_error(("depth format unsupported"));
        }

        helpers::createImage(
            ctx.dev, ctx.memProps, handler().commandHandler().getUniqueQueueFamilies(true, false, true, false),
            pipelineData_.samples, depthFormat_, tiling, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, extent_.width, extent_.height, 1, 1, depth_.image, depth_.memory);

        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (helpers::hasStencilComponent(depthFormat_)) {
            aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        VkImageSubresourceRange range = {aspectFlags, 0, 1, 0, 1};
        helpers::createImageView(ctx.dev, depth_.image, depthFormat_, VK_IMAGE_VIEW_TYPE_2D, range, depth_.view);
    }
}

void RenderPass::Base::updateClearValues() {
    /* NO LONGER RELEVANT! (keeping because this was confusing when first encountered)
     *  Need to pad this here because:
     *      In vkCmdBeginRenderPass() the VkRenderPassBeginInfo struct has a clearValueCount
     *      of 2 but there must be at least 3 entries in pClearValues array to account for the
     *      highest index attachment in renderPass 0x2f that uses VK_ATTACHMENT_LOAD_OP_CLEAR
     *      is 3. Note that the pClearValues array is indexed by attachment number so even if
     *      some pClearValues entries between 0 and 2 correspond to attachments that aren't
     *      cleared they will be ignored. The spec valid usage text states 'clearValueCount
     *      must be greater than the largest attachment index in renderPass that specifies a
     *      loadOp (or stencilLoadOp, if the attachment has a depth/stencil format) of
     *      VK_ATTACHMENT_LOAD_OP_CLEAR'
     *      (https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#VUID-VkRenderPassBeginInfo-clearValueCount-00902)
     */
    clearValues_.clear();
    // DEPTH ATTACHMENT
    if (pipelineData_.usesDepth) {
        clearValues_.push_back({});
        clearValues_.back().depthStencil = DEFAULT_CLEAR_DEPTH_STENCIL_VALUE;
    }
    // MULTI-SAMPLE ATTACHMENT
    if (usesMultiSample()) {
        clearValues_.push_back({});
        clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
    }
    // TARGET ATTACHMENT
    clearValues_.push_back({});
    clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
}

void RenderPass::Base::createFramebuffers() {
    /* These are the potential views for framebuffer.
     *  - depth
     *  - mulit-sample
     *  - target
     *      - swapchain
     *      - sampler
     *
     * TODO: Should it always be imageCount based?
     */
    std::vector<std::vector<VkImageView>> attachmentViewsList(handler().shell().context().imageCount);
    data.framebuffers.resize(attachmentViewsList.size());

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = pass;
    // createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
    // createInfo.pAttachments = attachmentViews.data();
    createInfo.width = extent_.width;
    createInfo.height = extent_.height;
    createInfo.layers = 1;

    for (uint8_t frameIndex = 0; frameIndex < attachmentViewsList.size(); frameIndex++) {
        auto& attachmentViews = attachmentViewsList[frameIndex];
        // DEPTH
        if (pipelineData_.usesDepth) {
            assert(depth_.view != VK_NULL_HANDLE);
            attachmentViews.push_back(depth_.view);  // TODO: should there be one per swapchain image????????????????
        }
        // MULTI-SAMPLE
        if (usesMultiSample()) {
            assert(images_.size() == 1);
            assert(images_[0].view != VK_NULL_HANDLE);
            attachmentViews.push_back(images_[0].view);  // TODO: should there be one per swapchain image????????????????
        }
        // TARGET
        if (hasTargetSwapchain()) {
            // SWAPCHAIN
            attachmentViews.push_back(handler().getSwapchainViews()[frameIndex]);
        } else {
            // SAMPLER
            auto texIndex = (std::min)(static_cast<uint8_t>(pTextures_.size() - 1), frameIndex);
            const auto& imageView =
                pTextures_[texIndex]->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view;
            assert(imageView != VK_NULL_HANDLE);
            attachmentViews.push_back(imageView);
        }

        createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        createInfo.pAttachments = attachmentViews.data();

        vk::assert_success(
            vkCreateFramebuffer(handler().shell().context().dev, &createInfo, nullptr, &data.framebuffers[frameIndex]));
    }
}

void RenderPass::Base::createViewport() {
    // VIEWPORT
    viewport_.x = 0.0f;
    viewport_.y = 0.0f;
    viewport_.width = static_cast<float>(extent_.width);
    viewport_.height = static_cast<float>(extent_.height);
    viewport_.minDepth = 0.0f;
    viewport_.maxDepth = 1.0f;
    // SCISSOR
    scissor_.offset = {0, 0};
    scissor_.extent = extent_;
}

void RenderPass::Base::updateBeginInfo() {
    beginInfo_.clearValueCount = static_cast<uint32_t>(clearValues_.size());
    beginInfo_.pClearValues = clearValues_.data();
    beginInfo_.renderArea.offset = {0, 0};
    beginInfo_.renderArea.extent = extent_;
}

void RenderPass::Base::destroyTargetResources() {
    auto& dev = handler().shell().context().dev;

    // COLOR
    for (auto& color : images_) helpers::destroyImageResource(dev, color);
    images_.clear();

    // DEPTH
    helpers::destroyImageResource(dev, depth_);

    // FRAMEBUFFER
    for (auto& framebuffer : data.framebuffers) vkDestroyFramebuffer(dev, framebuffer, nullptr);
    data.framebuffers.clear();

    // EXTENT
    extent_ = BAD_EXTENT_2D;

    // COMMAND
    helpers::destroyCommandBuffers(dev, handler().commandHandler().graphicsCmdPool(), data.priCmds);
    helpers::destroyCommandBuffers(dev, handler().commandHandler().graphicsCmdPool(), data.secCmds);

    // SEMAPHORE
    for (auto& semaphore : data.semaphores) vkDestroySemaphore(dev, semaphore, nullptr);
    data.semaphores.clear();
}

void RenderPass::Base::beginSecondary(const uint8_t frameIndex) {
    if (secCmdFlag_) return;
    // FRAME UPDATE
    auto& secCmd = data.secCmds[frameIndex];
    inheritInfo_.framebuffer = data.framebuffers[frameIndex];
    // COMMAND
    vkResetCommandBuffer(secCmd, 0);
    vk::assert_success(vkBeginCommandBuffer(secCmd, &secCmdBeginInfo_));
    // FRAME COMMANDS
    vkCmdSetScissor(secCmd, 0, 1, &scissor_);
    vkCmdSetViewport(secCmd, 0, 1, &viewport_);

    secCmdFlag_ = true;
}

void RenderPass::Base::endSecondary(const uint8_t frameIndex, const VkCommandBuffer& priCmd) {
    if (!secCmdFlag_) return;
    // EXECUTE SECONDARY
    auto& secCmd = data.secCmds[frameIndex];
    vk::assert_success(vkEndCommandBuffer(secCmd));
    vkCmdExecuteCommands(priCmd, 1, &secCmd);

    secCmdFlag_ = false;
}

void RenderPass::Base::updateSubmitResource(SubmitResource& resource, const uint8_t frameIndex) const {
    if (TYPE == PASS::SCREEN_SPACE_HDR_LOG) {
        resource.signalSemaphores[resource.signalSemaphoreCount] = data.semaphores[frameIndex];
        resource.signalSrcStageMasks[resource.signalSemaphoreCount] = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        resource.signalSemaphoreCount++;
    }
    // if (data.signalSrcStageMask != VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM) {
    //    std::memcpy(                                                    //
    //        &resource.signalSemaphores[resource.signalSemaphoreCount],  //
    //        &data.semaphores[frameIndex], sizeof(VkSemaphore)           //
    //    );
    //    std::memcpy(                                                       //
    //        &resource.signalSrcStageMasks[resource.signalSemaphoreCount],  //
    //        &data.signalSrcStageMask, sizeof(VkPipelineStageFlags)         //
    //    );
    //    resource.signalSemaphoreCount++;
    //}
    std::memcpy(                                                //
        &resource.commandBuffers[resource.commandBufferCount],  //
        &data.priCmds[frameIndex], sizeof(VkCommandBuffer)      //
    );
    resource.commandBufferCount++;
}

uint32_t RenderPass::Base::getSubpassId(const PIPELINE& type) const {
    uint32_t id = 0;
    for (const auto keyValue : pipelineTypeBindDataMap_) {
        if (keyValue.first == type) break;
        id++;
    }
    return id;  // TODO: is returning 0 for no types okay? Try using std::optional maybe?
}

void RenderPass::Base::setSubpassOffsets(const std::vector<std::unique_ptr<Base>>& pPasses) {
    for (auto& [passType, offset] : dependentTypeOffsetPairs_) {
        bool found = false;
        for (const auto& pPass : pPasses) {
            if (pPass->TYPE == passType) {
                offset = pPass->OFFSET;
                found = true;
            }
        }
        assert(found);
    }
}

void RenderPass::Base::setBindData(const PIPELINE& pipelineType, const std::shared_ptr<Pipeline::BindData>& pBindData) {
    pipelineTypeBindDataMap_[pipelineType] = pBindData;
}

void RenderPass::Base::destroy() {
    // render pass
    if (pass != VK_NULL_HANDLE) vkDestroyRenderPass(handler().shell().context().dev, pass, nullptr);
}

void RenderPass::Base::createSemaphores() {
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    data.semaphores.resize(semaphoreCount_);
    for (uint32_t i = 0; i < semaphoreCount_; i++) {
        vk::assert_success(vkCreateSemaphore(handler().shell().context().dev, &createInfo, nullptr, &data.semaphores[i]));
    }
}

void RenderPass::Base::createAttachmentDebugMarkers() {
    auto& ctx = handler().shell().context();
    if (handler().settings().enable_debug_markers) {
        std::string markerName;
        uint32_t count = 0;
        for (auto& color : images_) {
            if (color.image != VK_NULL_HANDLE) {
                markerName = NAME + " color framebuffer image (" + std::to_string(count++) + ")";
                ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)color.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                              markerName.c_str());
            }
        }
        if (depth_.image != VK_NULL_HANDLE) {
            markerName = NAME + " depth framebuffer image";
            ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)depth_.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                          markerName.c_str());
        }
    }
}
