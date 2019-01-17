
#include "RenderPass.h"

#include "CmdBufHandler.h"

// **********************
//      Base
// **********************

void RenderPass::Base::init(const Shell::Context& ctx, const Game::Settings& settings, RenderPass::InitInfo* pInfo) {
    // Set values from params
    memcpy(&initInfo, pInfo, sizeof(RenderPass::InitInfo));

    createSubpassResources(ctx, settings);

    createPass(ctx.dev);
    assert(pass != VK_NULL_HANDLE);

    createClearValues(ctx, settings);
    createBeginInfo(ctx, settings);

    if (settings.enable_debug_markers) {
        std::string markerName = name_ + " render pass";
        ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)pass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                                      markerName.c_str());
    }
}

void RenderPass::Base::createTarget(const Shell::Context& ctx, const Game::Settings& settings,
                                    RenderPass::FrameInfo* pInfo) {
    // Set info values
    memcpy(&frameInfo, pInfo, sizeof(RenderPass::FrameInfo));

    createFrameData(ctx, settings);
}

void RenderPass::Base::createPass(const VkDevice& dev) {
    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.pNext = nullptr;
    // ATTACHMENTS
    createInfo.attachmentCount = static_cast<uint32_t>(subpassResources_.attachments.size());
    createInfo.pAttachments = subpassResources_.attachments.data();
    // SUBPASSES
    createInfo.subpassCount = static_cast<uint32_t>(subpassResources_.subpasses.size());
    createInfo.pSubpasses = subpassResources_.subpasses.data();
    // DEPENDENCIES
    createInfo.dependencyCount = static_cast<uint32_t>(subpassResources_.dependencies.size());
    createInfo.pDependencies = subpassResources_.dependencies.data();

    vk::assert_success(vkCreateRenderPass(dev, &createInfo, nullptr, &pass));
}

void RenderPass::Base::createBeginInfo(const Shell::Context& ctx, const Game::Settings& settings) {
    beginInfo_ = {};
    beginInfo_.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo_.renderPass = pass;
    beginInfo_.clearValueCount = static_cast<uint32_t>(clearValues_.size());
    beginInfo_.pClearValues = clearValues_.data();
}

void RenderPass::Base::createSubpassResources(const Shell::Context& ctx, const Game::Settings& settings) {
    createSubpassesAndAttachments(ctx, settings);
    createDependencies(ctx, settings);
}

void RenderPass::Base::createFrameData(const Shell::Context& ctx, const Game::Settings& settings) {
    createCommandBuffers(ctx);
    createImages(ctx, settings);
    createImageViews(ctx);
    createViewport();
    createFramebuffers(ctx);
    updateBeginInfo(ctx, settings);
    createFences(ctx);
}

void RenderPass::Base::createCommandBuffers(const Shell::Context& ctx) {
    data.priCmds.resize(ctx.imageCount);
    CmdBufHandler::createCmdBuffers(CmdBufHandler::graphics_cmd_pool(), data.priCmds.data(), VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                    ctx.imageCount);
}

void RenderPass::Base::createImages(const Shell::Context& ctx, const Game::Settings& settings) {
    if (initInfo.hasColor) createColorResources(ctx);
    if (initInfo.hasDepth) createDepthResources(ctx);

    if (settings.enable_debug_markers) {
        std::string markerName;
        if (settings.include_color) {
            markerName = name_ + " color framebuffer image";
            ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)data.colorResource.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                          markerName.c_str());
        }
        if (settings.include_depth) {
            markerName = name_ + " depth framebuffer image";
            ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)data.depthResource.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                          markerName.c_str());
        }
    }
}

void RenderPass::Base::createColorResources(const Shell::Context& ctx) {
    auto& colorResource = data.colorResource;

    helpers::createImage(ctx.dev, CmdBufHandler::getUniqueQueueFamilies(true, false, true), initInfo.samples,
                         initInfo.format, VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, frameInfo.extent.width, frameInfo.extent.height, 1, 1,
                         colorResource.image, colorResource.memory);

    helpers::createImageView(ctx.dev, colorResource.image, 1, initInfo.format, VK_IMAGE_ASPECT_COLOR_BIT,
                             VK_IMAGE_VIEW_TYPE_2D, 1, colorResource.view);

    helpers::transitionImageLayout(CmdBufHandler::graphics_cmd(), colorResource.image, initInfo.format,
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 1, 1);
}

void RenderPass::Base::createDepthResources(const Shell::Context& ctx) {
    VkImageTiling tiling;
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(ctx.physical_dev, initInfo.depthFormat, &props);

    if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        tiling = VK_IMAGE_TILING_LINEAR;
    } else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        tiling = VK_IMAGE_TILING_OPTIMAL;
    } else {
        // Try other depth formats
        throw std::runtime_error(("depth format unsupported"));
    }

    auto& depthResource = data.depthResource;

    helpers::createImage(ctx.dev, CmdBufHandler::getUniqueQueueFamilies(true, false, true), initInfo.samples,
                         initInfo.depthFormat, tiling, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, frameInfo.extent.width, frameInfo.extent.height, 1, 1,
                         depthResource.image, depthResource.memory);

    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (helpers::hasStencilComponent(initInfo.depthFormat)) {
        aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    helpers::createImageView(ctx.dev, depthResource.image, 1, initInfo.depthFormat, aspectFlags, VK_IMAGE_VIEW_TYPE_2D, 1,
                             depthResource.view);

    helpers::transitionImageLayout(CmdBufHandler::graphics_cmd(), depthResource.image, initInfo.depthFormat,
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 1, 1);
}

void RenderPass::Base::createViewport() {
    // VIEWPORT
    data.viewport.x = 0.0f;
    data.viewport.y = 0.0f;
    data.viewport.width = static_cast<float>(frameInfo.extent.width);
    data.viewport.height = static_cast<float>(frameInfo.extent.height);
    data.viewport.minDepth = 0.0f;
    data.viewport.maxDepth = 1.0f;
    // SCISSOR
    data.scissor.offset = {0, 0};
    data.scissor.extent = frameInfo.extent;
}

void RenderPass::Base::updateBeginInfo(const Shell::Context& ctx, const Game::Settings& settings) {
    beginInfo_.renderArea.offset = {0, 0};
    beginInfo_.renderArea.extent = frameInfo.extent;
}

void RenderPass::Base::destroyTarget(const VkDevice& dev) {
    // color
    if (data.colorResource.view != VK_NULL_HANDLE) vkDestroyImageView(dev, data.colorResource.view, nullptr);
    if (data.colorResource.image != VK_NULL_HANDLE) vkDestroyImage(dev, data.colorResource.image, nullptr);
    if (data.colorResource.memory != VK_NULL_HANDLE) vkFreeMemory(dev, data.colorResource.memory, nullptr);
    // depth
    if (data.depthResource.view != VK_NULL_HANDLE) vkDestroyImageView(dev, data.depthResource.view, nullptr);
    if (data.depthResource.image != VK_NULL_HANDLE) vkDestroyImage(dev, data.depthResource.image, nullptr);
    if (data.depthResource.memory != VK_NULL_HANDLE) vkFreeMemory(dev, data.depthResource.memory, nullptr);
    // framebuffer
    for (auto& framebuffer : data.framebuffers) vkDestroyFramebuffer(dev, framebuffer, nullptr);
    // fences
    for (auto& fence : data.fences) vkDestroyFence(dev, fence, nullptr);
    // command
    if (!data.priCmds.empty()) {
        vkFreeCommandBuffers(dev, CmdBufHandler::graphics_cmd_pool(), static_cast<uint32_t>(data.priCmds.size()),
                             data.priCmds.data());
    }
    if (!data.secCmds.empty()) {
        vkFreeCommandBuffers(dev, CmdBufHandler::graphics_cmd_pool(), static_cast<uint32_t>(data.secCmds.size()),
                             data.secCmds.data());
    }
}

void RenderPass::Base::destroy(const VkDevice& dev) {
    // render pass
    if (pass != VK_NULL_HANDLE) vkDestroyRenderPass(dev, pass, nullptr);
}

// **********************
//      Default
// **********************

void RenderPass::Default::createClearValues(const Shell::Context& ctx, const Game::Settings& settings) {
    /*
        These indices need to be the same for the attachment
        references and descriptions for the render pass!
    */
    attachmentCount_ = 1;  // TODO: this logic needs a home with the other attachment stuff
    if (initInfo.hasColor) attachmentCount_++;
    if (initInfo.hasDepth) attachmentCount_++;

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
    clearValues_.resize(attachmentCount_);
    // clearValuues[0] = final layout is in this spot; // pad this spot
    clearValues_[1].color = initInfo.colorClearColorValue;
    clearValues_[2].depthStencil = initInfo.depthClearValue;
}

void RenderPass::Default::createBeginInfo(const Shell::Context& ctx, const Game::Settings& settings) {
    RenderPass::Base::createBeginInfo(ctx, settings);

    // INHERITANCE
    inheritInfo_ = {};
    inheritInfo_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritInfo_.renderPass = pass;
    inheritInfo_.subpass = getSubpassId(PIPELINE_TYPE::TRI_LIST_TEX);
    // Validation layer: Cannot set inherited occlusionQueryEnable in vkBeginCommandBuffer() when device does not support
    // inheritedQueries.
    inheritInfo_.occlusionQueryEnable = VK_FALSE;  // TODO: not sure
    inheritInfo_.queryFlags = 0;                   // TODO: not sure
    inheritInfo_.pipelineStatistics = 0;           // TODO: not sure

    // BUFFER
    secCmdBeginInfo_ = {};
    secCmdBeginInfo_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    secCmdBeginInfo_.pNext = nullptr;
    secCmdBeginInfo_.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    secCmdBeginInfo_.pInheritanceInfo = &inheritInfo_;
}

void RenderPass::Default::createCommandBuffers(const Shell::Context& ctx) {
    RenderPass::Base::createCommandBuffers(ctx);
    // SECONDARY
    data.secCmds.resize(ctx.imageCount);
    CmdBufHandler::createCmdBuffers(CmdBufHandler::graphics_cmd_pool(), data.secCmds.data(),
                                    VK_COMMAND_BUFFER_LEVEL_SECONDARY, ctx.imageCount);
}

void RenderPass::Default::createSubpassesAndAttachments(const Shell::Context& ctx, const Game::Settings& settings) {
    auto& resources = subpassResources_;
    VkAttachmentDescription attachemnt = {};

    // COLOR ATTACHMENT (SWAP-CHAIN)
    attachemnt = {};
    attachemnt.format = initInfo.format;
    attachemnt.samples = VK_SAMPLE_COUNT_1_BIT;
    attachemnt.loadOp =
        VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachemnt.finalLayout = initInfo.finalLayout;
    attachemnt.flags = 0;
    resources.attachments.push_back(attachemnt);
    // REFERENCE
    resources.colorAttachments.resize(1);
    resources.colorAttachments.back().attachment = static_cast<uint32_t>(resources.attachments.size() - 1);
    resources.colorAttachments.back().layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // COLOR ATTACHMENT RESOLVE (MULTI-SAMPLE)
    if (initInfo.hasColor) {
        attachemnt = {};
        attachemnt.format = initInfo.format;
        attachemnt.samples = initInfo.samples;
        attachemnt.loadOp = initInfo.hasDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachemnt.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachemnt.flags = 0;
        // REFERENCE
        // point to swapchain attachment
        resources.resolveAttachments.resize(1);
        resources.resolveAttachments.back().attachment = static_cast<uint32_t>(resources.attachments.size() - 1);
        resources.resolveAttachments.back().layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        resources.attachments.push_back(attachemnt);
        // point to multi-sample attachment
        resources.colorAttachments.back().attachment = static_cast<uint32_t>(resources.attachments.size() - 1);
    }

    // TODO: this depth attachment shouldn't be added if there is no depth

    // DEPTH ATTACHMENT
    attachemnt = {};
    attachemnt.format = initInfo.depthFormat;
    attachemnt.samples = initInfo.samples;
    attachemnt.loadOp = initInfo.hasDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachemnt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachemnt.flags = 0;
    resources.attachments.push_back(attachemnt);
    // REFERENCE
    resources.depthStencilAttachment = {};
    resources.depthStencilAttachment.attachment = static_cast<uint32_t>(resources.attachments.size() - 1);
    resources.depthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // TODO: below comes from pipeline ???
    // SETUP DESCRIPTION
    VkSubpassDescription subpass = {};
    subpass.colorAttachmentCount = static_cast<uint32_t>(resources.colorAttachments.size());
    subpass.pColorAttachments = resources.colorAttachments.data();
    subpass.pResolveAttachments = resources.resolveAttachments.data();
    subpass.pDepthStencilAttachment = &resources.depthStencilAttachment;

    resources.subpasses.assign(PIPELINE_TYPES.size(), subpass);
}

void RenderPass::Default::createDependencies(const Shell::Context& ctx, const Game::Settings& settings) {
    auto& resources = subpassResources_;
    VkSubpassDependency dependency = {};

    // TODO: fix this...
    dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    resources.dependencies.push_back(dependency);

    // TODO: fix this...
    dependency = {};
    dependency.srcSubpass = 0;
    dependency.dstSubpass = 1;
    dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependency.dstAccessMask = 0;
    resources.dependencies.push_back(dependency);

    // TODO: fix this...
    dependency = {};
    dependency.srcSubpass = 1;
    dependency.dstSubpass = 2;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    resources.dependencies.push_back(dependency);

    //// TODO: fix this...
    // dependency = {};
    // dependency.srcSubpass = static_cast<uint32_t>(dependencyTuples_.size()) - 1;
    // dependency.dstSubpass = static_cast<uint32_t>(dependencyTuples_.size());
    // dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // dependency.srcAccessMask = 0;
    // dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    // dependencyTuples_.push_back({PIPELINE_TYPE::TRI_LIST_COLOR, SUBPASS_TYPE::UI, dependency});
}

void RenderPass::Default::createFences(const Shell::Context& ctx, VkFenceCreateFlags flags) {
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = flags;
    data.fences.resize(ctx.imageCount);
    for (auto& fence : data.fences) vk::assert_success(vkCreateFence(ctx.dev, &fenceInfo, nullptr, &fence));
}

void RenderPass::Default::createFramebuffers(const Shell::Context& ctx) {
    /*  These are the views for framebuffer.

        view[0] is swapchain
        view[1] is mulit-sample (resolve)
        view[2] is depth

        The incoming "views" param are the swapchain views.
    */
    std::vector<VkImageView> attachmentViews(attachmentCount_);
    if (initInfo.hasColor) attachmentViews[1] = data.colorResource.view;
    if (initInfo.hasDepth) attachmentViews[2] = data.depthResource.view;

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = pass;
    createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
    createInfo.pAttachments = attachmentViews.data();
    createInfo.width = frameInfo.extent.width;
    createInfo.height = frameInfo.extent.height;
    createInfo.layers = 1;

    data.framebuffers.resize(frameInfo.viewCount);
    for (uint8_t i = 0; i < frameInfo.viewCount; i++) {
        // Set slot [0] for the framebuffer[i] view attachments
        // to the appropriate swapchain image view.
        attachmentViews[0] = frameInfo.pViews[i];
        // Create the framebuffer...
        vk::assert_success(vkCreateFramebuffer(ctx.dev, &createInfo, nullptr, &data.framebuffers[i]));
    }
}
