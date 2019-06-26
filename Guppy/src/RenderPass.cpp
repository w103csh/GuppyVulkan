
#include "RenderPass.h"

#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "DescriptorHandler.h"
#include "MaterialHandler.h"
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
      // SETTINGS
      format_(VK_FORMAT_UNDEFINED),
      depthFormat_(VK_FORMAT_UNDEFINED),
      finalLayout_(VK_IMAGE_LAYOUT_UNDEFINED),
      commandCount_(0),
      semaphoreCount_(0),
      extent_(BAD_EXTENT_2D),
      depth_{},
      descPipelineOffsets_(pCreateInfo->descPipelineOffsets),
      textureIds_(pCreateInfo->textureIds),
      pTexture_(nullptr)  //
{
    // This is sloppy, but I don't really feel like changing a bunch of things.
    if (textureIds_.empty()) textureIds_.emplace_back(std::string(SWAPCHAIN_TARGET_ID));
    assert(textureIds_.size());

    for (const auto& pipelineType : pCreateInfo->pipelineTypes) {
        // If changed from set then a lot of work needs to be done, so I am
        // adding some validation here.
        assert(pipelineTypeBindDataMap_.count(pipelineType) == 0);
        pipelineTypeBindDataMap_.insert({pipelineType, {}});
    }
}

void RenderPass::Base::init(bool isFinal) {
    auto& ctx = handler().shell().context();

    // SAMPLER
    if (getTargetId().compare(RenderPass::SWAPCHAIN_TARGET_ID) != 0) {
        // Force only one texture, for now.
        assert(textureIds_.size() == 1);
        pTexture_ = RenderPass::Base::handler().textureHandler().getTextureByName(getTargetId());
        // Force only one sampler, for now, and just assume its the target.
        assert(pTexture_->samplers.size() == 1);
        extent_ = pTexture_->samplers[0].extent;
        assert(pTexture_ != nullptr && pTexture_->samplers.size());
    }

    // FRAME
    format_ = ctx.surfaceFormat.format;
    depthFormat_ = usesDepth() ? ctx.depthFormat : VK_FORMAT_UNDEFINED;
    pipelineData_.samples = usesMultiSample() ? ctx.samples : VK_SAMPLE_COUNT_1_BIT;
    if (hasTargetSampler()) {
        finalLayout_ = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    } else if (hasTargetSwapchain()) {
        // Check if this is the last swapchain dependent pass.
        finalLayout_ = isFinal ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    // Validate frame settings.
    assert(finalLayout_ != VK_IMAGE_LAYOUT_UNDEFINED);
    if (pipelineData_.usesDepth)
        assert(depthFormat_ != VK_FORMAT_UNDEFINED);
    else
        assert(depthFormat_ == VK_FORMAT_UNDEFINED);

    // SYNC
    commandCount_ = ctx.imageCount;
    if (finalLayout_ ^ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        semaphoreCount_ = ctx.imageCount;
        data.signalSrcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
}

void RenderPass::Base::create() {
    createAttachmentsAndSubpasses();
    createDependencies();

    createPass();
    assert(pass != VK_NULL_HANDLE);

    createBeginInfo();

    if (handler().settings().enable_debug_markers) {
        std::string markerName = NAME + " render pass";
        ext::DebugMarkerSetObjectName(handler().shell().context().dev, (uint64_t)pass,
                                      VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, markerName.c_str());
    }
}

void RenderPass::Base::setSwapchainInfo() {
    assert(extent_.height == BAD_EXTENT_2D.height && extent_.width == BAD_EXTENT_2D.width);
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

void RenderPass::Base::overridePipelineCreateInfo(const PIPELINE& type, Pipeline::CreateInfoVkResources& createInfoRes) {
    assert(pipelineData_.samples != VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM);
    createInfoRes.depthStencilStateInfo.depthTestEnable = pipelineData_.usesDepth;
    createInfoRes.depthStencilStateInfo.depthWriteEnable = pipelineData_.usesDepth;
    createInfoRes.multisampleStateInfo.rasterizationSamples = pipelineData_.samples;
}

void RenderPass::Base::record(const uint32_t& frameIndex) {
    beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    // pPass->beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    auto& priCmd = data.priCmds[frameIndex];
    auto& secCmd = data.secCmds[frameIndex];
    auto& pScene = handler().sceneHandler().getActiveScene();

    auto it = pipelineTypeBindDataMap_.begin();
    while (it != pipelineTypeBindDataMap_.end()) {
        // Record the scene
        pScene->record(TYPE, it->first, it->second, priCmd, secCmd, frameIndex);

        std::advance(it, 1);
        if (it != pipelineTypeBindDataMap_.end()) {
            vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
            // vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        }
    }

    endPass(frameIndex);
}

void RenderPass::Base::beginPass(const uint8_t& frameIndex, VkCommandBufferUsageFlags&& primaryCommandUsage,
                                 VkSubpassContents&& subpassContents) {
    // FRAME UPDATE
    beginInfo_.framebuffer = data.framebuffers[frameIndex];
    auto& priCmd = data.priCmds[frameIndex];

    // RESET BUFFERS
    vkResetCommandBuffer(priCmd, 0);

    // BEGIN BUFFERS
    VkCommandBufferBeginInfo bufferInfo;
    // PRIMARY
    bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferInfo.flags = primaryCommandUsage;
    vk::assert_success(vkBeginCommandBuffer(priCmd, &bufferInfo));

    // FRAME COMMANDS
    vkCmdSetScissor(priCmd, 0, 1, &scissor_);
    vkCmdSetViewport(priCmd, 0, 1, &viewport_);

    //// Start a new debug marker region
    // ext::DebugMarkerBegin(primaryCmd, "Render x scene", glm::vec4(0.2f, 0.3f, 0.4f, 1.0f));
    vkCmdBeginRenderPass(priCmd, &beginInfo_, subpassContents);
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
    beginInfo_.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo_.renderPass = pass;

    if (usesSecondaryCommands()) {
        // INHERITANCE
        inheritInfo_ = {};
        inheritInfo_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritInfo_.renderPass = pass;
        inheritInfo_.subpass = getSubpassId(PIPELINE::TRI_LIST_TEX);
        // Validation layer: Cannot set inherited occlusionQueryEnable in vkBeginCommandBuffer() when device does not support
        // inheritedQueries.
        inheritInfo_.occlusionQueryEnable = VK_FALSE;  // TODO: not sure
        inheritInfo_.queryFlags = 0;                   // TODO: not sure
        inheritInfo_.pipelineStatistics = 0;           // TODO: not sure

        // BUFFER
        secCmdBeginInfo_ = {};
        secCmdBeginInfo_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        secCmdBeginInfo_.pNext = nullptr;
        secCmdBeginInfo_.flags =
            VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        secCmdBeginInfo_.pInheritanceInfo = &inheritInfo_;
    }
}

void RenderPass::Base::createAttachmentsAndSubpasses() {
    // DEPTH ATTACHMENT
    if (pipelineData_.usesDepth) {
        resources_.attachments.push_back({});
        resources_.attachments.back().format = getDepthFormat();
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
        resources_.attachments.back().format = getFormat();
        resources_.attachments.back().samples = getSamples();
        resources_.attachments.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        resources_.attachments.back().storeOp = VK_ATTACHMENT_STORE_OP_STORE;
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

    bool clearTarget = handler().shouldClearTarget(getTargetId());

    // COLOR ATTACHMENT
    resources_.attachments.push_back({});
    resources_.attachments.back().format = getFormat();
    resources_.attachments.back().samples = usesMultiSample() ? VK_SAMPLE_COUNT_1_BIT : getSamples();
    resources_.attachments.back().loadOp = clearTarget ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    resources_.attachments.back().storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resources_.attachments.back().stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resources_.attachments.back().stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resources_.attachments.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resources_.attachments.back().finalLayout = getFinalLayout();
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

    // TODO: below comes from pipeline ???
    // SETUP DESCRIPTION
    VkSubpassDescription subpass = {};
    subpass.colorAttachmentCount = static_cast<uint32_t>(resources_.colorAttachments.size());
    subpass.pColorAttachments = resources_.colorAttachments.data();
    subpass.pResolveAttachments = resources_.resolveAttachments.data();
    subpass.pDepthStencilAttachment = pipelineData_.usesDepth ? &resources_.depthStencilAttachment : nullptr;

    resources_.subpasses.assign(getPipelineCount(), subpass);
}

void RenderPass::Base::createDependencies() {
    VkSubpassDependency dependency = {};
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
    data.priCmds.resize(commandCount_);
    handler().commandHandler().createCmdBuffers(handler().commandHandler().graphicsCmdPool(), data.priCmds.data(),
                                                VK_COMMAND_BUFFER_LEVEL_PRIMARY, handler().shell().context().imageCount);
    if (usesSecondaryCommands() || true) {  // TODO: these things are never used
        // SECONDARY
        data.secCmds.resize(handler().shell().context().imageCount);
        handler().commandHandler().createCmdBuffers(handler().commandHandler().graphicsCmdPool(), data.secCmds.data(),
                                                    VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                                                    handler().shell().context().imageCount);
    }
}

void RenderPass::Base::createImageResources() {
    auto& ctx = handler().shell().context();

    if (hasTargetSampler()) {
        auto& sampler = pTexture_->samplers.back();

        helpers::createImage(ctx.dev, ctx.mem_props, handler().commandHandler().getUniqueQueueFamilies(true, false, true),
                             (usesMultiSample() ? VK_SAMPLE_COUNT_1_BIT : getSamples()), format_, sampler.TILING,
                             VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, extent_.width, extent_.height, 1, 1, sampler.image,
                             sampler.memory);

        helpers::createImageView(ctx.dev, sampler.image, 1, format_, VK_IMAGE_ASPECT_COLOR_BIT, sampler.imageViewType, 1,
                                 pTexture_->samplers.back().view);

        handler().textureHandler().createSampler(ctx.dev, sampler);

        sampler.imgDescInfo.imageLayout = finalLayout_;
        sampler.imgDescInfo.imageView = sampler.view;
        sampler.imgDescInfo.sampler = sampler.sampler;

        // TODO: This is getting a little too similar here. Maybe find a way to
        // move this stuff to the texture handler.
        bool wasRemade = pTexture_->status == STATUS::DESTROYED;
        pTexture_->status = STATUS::READY;
        handler().materialHandler().updateTexture(pTexture_);
        if (wasRemade) {
            handler().descriptorHandler().updateBindData(pTexture_);
        }
    } else {
        assert(pTexture_ == nullptr);
    }

    if (usesMultiSample()) {
        assert(resources_.resolveAttachments.size() == 1 && "Make sure multi-sampling attachment reference was added");

        images_.push_back({});

        helpers::createImage(ctx.dev, ctx.mem_props, handler().commandHandler().getUniqueQueueFamilies(true, false, true),
                             pipelineData_.samples, format_, VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, extent_.width, extent_.height, 1, 1, images_.back().image,
                             images_.back().memory);

        helpers::createImageView(ctx.dev, images_.back().image, 1, format_, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D,
                                 1, images_.back().view);
    } else {
        assert(resources_.resolveAttachments.size() == 0 && "Make sure multi-sampling attachment reference was not added");
    }
}

void RenderPass::Base::createDepthResource() {
    auto& ctx = handler().shell().context();
    if (pipelineData_.usesDepth) {
        VkImageTiling tiling;
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(ctx.physical_dev, depthFormat_, &props);

        if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            tiling = VK_IMAGE_TILING_LINEAR;
        } else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            tiling = VK_IMAGE_TILING_OPTIMAL;
        } else {
            // Try other depth formats
            throw std::runtime_error(("depth format unsupported"));
        }

        helpers::createImage(ctx.dev, ctx.mem_props, handler().commandHandler().getUniqueQueueFamilies(true, false, true),
                             pipelineData_.samples, depthFormat_, tiling, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, extent_.width, extent_.height, 1, 1, depth_.image,
                             depth_.memory);

        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (helpers::hasStencilComponent(depthFormat_)) {
            aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        helpers::createImageView(ctx.dev, depth_.image, 1, depthFormat_, aspectFlags, VK_IMAGE_VIEW_TYPE_2D, 1, depth_.view);
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
    // SWAPCHAIN ATTACHMENT
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
     */
    std::vector<VkImageView> attachmentViews;
    // DEPTH
    if (pipelineData_.usesDepth) {
        assert(depth_.view != VK_NULL_HANDLE);
        attachmentViews.push_back(depth_.view);
    }
    // MULTI-SAMPLE
    if (usesMultiSample()) {
        assert(images_.size() == 1);
        assert(images_[0].view != VK_NULL_HANDLE);
        attachmentViews.push_back(images_[0].view);
    }
    // TARGET
    if (hasTargetSwapchain()) {
        // SWAPCHAIN
        attachmentViews.push_back({});
    } else {
        // SAMPLER
        assert(pTexture_->samplers.size() == 1 && pTexture_->samplers[0].view != VK_NULL_HANDLE);
        attachmentViews.push_back(pTexture_->samplers[0].view);
    }

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = pass;
    createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
    createInfo.pAttachments = attachmentViews.data();
    createInfo.width = extent_.width;
    createInfo.height = extent_.height;
    createInfo.layers = 1;

    data.framebuffers.resize(handler().shell().context().imageCount);  // Should this count always be this?
    for (auto i = 0; i < data.framebuffers.size(); i++) {
        // TODO: HOLY SHIT DOES THIS MEAN THAT THERE SHOULD BE VIEWS
        // FOR EVERY PASS FRAMEBUFFER???????????
        if (hasTargetSwapchain()) {
            attachmentViews.back() = handler().getSwapchainViews()[i];
        }
        // Create the framebuffer...
        vk::assert_success(
            vkCreateFramebuffer(handler().shell().context().dev, &createInfo, nullptr, &data.framebuffers[i]));
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

    // SAMPLER
    if (usesSwapchain() && hasTargetSampler()) pTexture_->destroy(dev);
    // COLOR
    for (auto& color : images_) helpers::destroyImageResource(dev, color);
    images_.clear();
    // DEPTH
    helpers::destroyImageResource(dev, depth_);
    // FRAMEBUFFER
    for (auto& framebuffer : data.framebuffers) vkDestroyFramebuffer(dev, framebuffer, nullptr);
    data.framebuffers.clear();

    extent_ = BAD_EXTENT_2D;

    // COMMAND
    helpers::destroyCommandBuffers(dev, handler().commandHandler().graphicsCmdPool(), data.priCmds);
    helpers::destroyCommandBuffers(dev, handler().commandHandler().graphicsCmdPool(), data.secCmds);
    // SEMAPHORE
    for (auto& semaphore : data.semaphores) vkDestroySemaphore(dev, semaphore, nullptr);

    data.semaphores.clear();
}

void RenderPass::Base::beginSecondary(const uint8_t& frameIndex) {
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

void RenderPass::Base::endSecondary(const uint8_t& frameIndex, const VkCommandBuffer& priCmd) {
    if (!secCmdFlag_) return;
    // EXECUTE SECONDARY
    auto& secCmd = data.secCmds[frameIndex];
    vk::assert_success(vkEndCommandBuffer(secCmd));
    vkCmdExecuteCommands(priCmd, 1, &secCmd);

    secCmdFlag_ = false;
}

void RenderPass::Base::updateSubmitResource(SubmitResource& resource, const uint32_t& frameIndex) const {
    if (data.signalSrcStageMask != VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM) {
        std::memcpy(                                                    //
            &resource.signalSemaphores[resource.signalSemaphoreCount],  //
            &data.semaphores[frameIndex], sizeof(VkSemaphore)           //
        );
        std::memcpy(                                                       //
            &resource.signalSrcStageMasks[resource.signalSemaphoreCount],  //
            &data.signalSrcStageMask, sizeof(VkPipelineStageFlags)         //
        );
        resource.signalSemaphoreCount++;
    }
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
