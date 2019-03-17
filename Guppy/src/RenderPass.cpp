
#include "RenderPass.h"

#include "CommandHandler.h"

namespace {

void addDefaultSubpasses(RenderPass::SubpassResources& resources, uint64_t count) {
    VkSubpassDependency dependency = {};
    for (uint32_t i = 0; i < count - 1; i++) {
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
        resources.dependencies.push_back(dependency);
    }
}

}  // namespace

// **********************
//      Base
// **********************

void RenderPass::Base::init(const Shell::Context& ctx, const Game::Settings& settings, RenderPass::InitInfo* pInfo,
                            SubpassResources* pSubpassResources) {
    // Set values from params.
    memcpy(&initInfo, pInfo, sizeof(RenderPass::InitInfo));

    createAttachmentsAndSubpasses(ctx, settings);
    createDependencies(ctx, settings);

    createPass(ctx.dev);
    assert(pass != VK_NULL_HANDLE);

    createBeginInfo(ctx, settings);

    if (settings.enable_debug_markers) {
        std::string markerName = NAME + " render pass";
        ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)pass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                                      markerName.c_str());
    }
}

void RenderPass::Base::createTarget(const Shell::Context& ctx, const Game::Settings& settings,
                                    Command::Handler& cmdBuffHandler, RenderPass::FrameInfo* pInfo) {
    // Set info values
    memcpy(&frameInfo, pInfo, sizeof(RenderPass::FrameInfo));

    // FRAME
    createColorResources(ctx, settings, cmdBuffHandler);
    createDepthResource(ctx, settings, cmdBuffHandler);
    createAttachmentDebugMarkers(ctx, settings);
    createClearValues(ctx, settings);
    createFramebuffers(ctx, settings);

    // RENDER PASS
    createCommandBuffers(ctx, cmdBuffHandler);
    createViewport();
    updateBeginInfo(ctx, settings);

    // SYNC
    createFences(ctx);
    createSemaphores(ctx);
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
}

void RenderPass::Base::createCommandBuffers(const Shell::Context& ctx, Command::Handler& cmdBuffHandler) {
    data.priCmds.resize(initInfo.commandCount);
    cmdBuffHandler.createCmdBuffers(cmdBuffHandler.graphicsCmdPool(), data.priCmds.data(), VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                    ctx.imageCount);
}

void RenderPass::Base::createFences(const Shell::Context& ctx, VkFenceCreateFlags flags) {
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = flags;
    data.fences.resize(initInfo.fenceCount);
    for (auto& fence : data.fences) vk::assert_success(vkCreateFence(ctx.dev, &fenceInfo, nullptr, &fence));
}

void RenderPass::Base::createViewport() {
    // VIEWPORT
    viewport_.x = 0.0f;
    viewport_.y = 0.0f;
    viewport_.width = static_cast<float>(frameInfo.extent.width);
    viewport_.height = static_cast<float>(frameInfo.extent.height);
    viewport_.minDepth = 0.0f;
    viewport_.maxDepth = 1.0f;
    // SCISSOR
    scissor_.offset = {0, 0};
    scissor_.extent = frameInfo.extent;
}

void RenderPass::Base::updateBeginInfo(const Shell::Context& ctx, const Game::Settings& settings) {
    beginInfo_.clearValueCount = static_cast<uint32_t>(clearValues_.size());
    beginInfo_.pClearValues = clearValues_.data();
    beginInfo_.renderArea.offset = {0, 0};
    beginInfo_.renderArea.extent = frameInfo.extent;
}

void RenderPass::Base::destroyTargetResources(const VkDevice& dev, Command::Handler& cmdBuffHandler) {
    // COLOR
    for (auto& color : colors_) helpers::destroyImageResource(dev, color);
    colors_.clear();
    // DEPTH
    helpers::destroyImageResource(dev, depth_);
    // FRAMEBUFFER
    for (auto& framebuffer : data.framebuffers) vkDestroyFramebuffer(dev, framebuffer, nullptr);
    data.framebuffers.clear();
    // COMMAND
    helpers::destroyCommandBuffers(dev, cmdBuffHandler.graphicsCmdPool(), data.priCmds);
    helpers::destroyCommandBuffers(dev, cmdBuffHandler.graphicsCmdPool(), data.secCmds);
    // FENCE
    for (auto& fence : data.fences) vkDestroyFence(dev, fence, nullptr);
    data.fences.clear();
    // SEMAPHORE
    for (auto& semaphore : data.semaphores) vkDestroySemaphore(dev, semaphore, nullptr);
    data.semaphores.clear();
}

void RenderPass::Base::destroy(const VkDevice& dev) {
    // render pass
    if (pass != VK_NULL_HANDLE) vkDestroyRenderPass(dev, pass, nullptr);
}

void RenderPass::Base::createSemaphores(const Shell::Context& ctx) {
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    data.semaphores.resize(initInfo.semaphoreCount);
    for (uint32_t i = 0; i < initInfo.semaphoreCount; i++) {
        vk::assert_success(vkCreateSemaphore(ctx.dev, &createInfo, nullptr, &data.semaphores[i]));
    }
}

void RenderPass::Base::createAttachmentDebugMarkers(const Shell::Context& ctx, const Game::Settings& settings) {
    if (settings.enable_debug_markers) {
        std::string markerName;
        uint32_t count = 0;
        for (auto& color : colors_) {
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

// **********************
//      Default
// **********************

RenderPass::Default::Default()
    : Base{"Default",
           {
               // Order of the subpasses
               PIPELINE::TRI_LIST_COLOR,  //
               PIPELINE::PBR_COLOR,
               PIPELINE::LINE,
               PIPELINE::TRI_LIST_TEX,
               PIPELINE::PBR_TEX,
           }},
      secCmdFlag_(false) {}

void RenderPass::Default::getSubmitResource(const uint8_t& frameIndex, SubmitResource& resource) const {
    if (initInfo.finalLayout ^ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
        resource.signalSemaphores.push_back(data.semaphores[frameIndex]);
    resource.commandBuffers.push_back(data.priCmds[frameIndex]);
}

void RenderPass::Default::createClearValues(const Shell::Context& ctx, const Game::Settings& settings) {
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
    clearValues_.resize(1);  // pad for framebuffer view
    // clearValuues[0] = final layout is in this spot; // pad this spot

    for (auto& color : colors_) {
        VkClearValue value = {CLEAR_VALUE};
        clearValues_.push_back(value);
    }
    if (depth_.view != VK_NULL_HANDLE) {
        VkClearValue value = {1.0f, 0};
        clearValues_.push_back(value);
    }
}

void RenderPass::Default::createBeginInfo(const Shell::Context& ctx, const Game::Settings& settings) {
    RenderPass::Base::createBeginInfo(ctx, settings);

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
    secCmdBeginInfo_.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    secCmdBeginInfo_.pInheritanceInfo = &inheritInfo_;
}

void RenderPass::Default::createCommandBuffers(const Shell::Context& ctx, Command::Handler& cmdBuffHandler) {
    RenderPass::Base::createCommandBuffers(ctx, cmdBuffHandler);
    // SECONDARY
    data.secCmds.resize(ctx.imageCount);
    cmdBuffHandler.createCmdBuffers(cmdBuffHandler.graphicsCmdPool(), data.secCmds.data(), VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                                    ctx.imageCount);
}

void RenderPass::Default::createColorResources(const Shell::Context& ctx, const Game::Settings& settings,
                                               Command::Handler& cmdBuffHandler) {
    if (settings.include_color) {  // TODO: is this still something?
        colors_.resize(1);
        auto& res = colors_.back();

        helpers::createImage(ctx.dev, ctx.mem_props, cmdBuffHandler.getUniqueQueueFamilies(true, false, true),
                             initInfo.samples, initInfo.format, VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, frameInfo.extent.width, frameInfo.extent.height, 1, 1,
                             colors_.back().image, colors_.back().memory);

        helpers::createImageView(ctx.dev, colors_.back().image, 1, initInfo.format, VK_IMAGE_ASPECT_COLOR_BIT,
                                 VK_IMAGE_VIEW_TYPE_2D, 1, colors_.back().view);

        helpers::transitionImageLayout(cmdBuffHandler.graphicsCmd(), colors_.back().image, initInfo.format,
                                       VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 1,
                                       1);
    }
}

void RenderPass::Default::createDepthResource(const Shell::Context& ctx, const Game::Settings& settings,
                                              Command::Handler& cmdBuffHandler) {
    if (settings.include_depth) {  // TODO: is this still something?
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

        helpers::createImage(ctx.dev, ctx.mem_props, cmdBuffHandler.getUniqueQueueFamilies(true, false, true),
                             initInfo.samples, initInfo.depthFormat, tiling, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, frameInfo.extent.width, frameInfo.extent.height, 1, 1,
                             depth_.image, depth_.memory);

        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (helpers::hasStencilComponent(initInfo.depthFormat)) {
            aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        helpers::createImageView(ctx.dev, depth_.image, 1, initInfo.depthFormat, aspectFlags, VK_IMAGE_VIEW_TYPE_2D, 1,
                                 depth_.view);

        helpers::transitionImageLayout(cmdBuffHandler.graphicsCmd(), depth_.image, initInfo.depthFormat,
                                       VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 1, 1);
    }
}

void RenderPass::Default::createAttachmentsAndSubpasses(const Shell::Context& ctx, const Game::Settings& settings) {
    auto& resources = subpassResources_;
    VkAttachmentDescription attachemnt = {};

    // COLOR ATTACHMENT (SWAP-CHAIN)
    attachemnt = {};
    attachemnt.format = initInfo.format;
    attachemnt.samples = VK_SAMPLE_COUNT_1_BIT;
    attachemnt.loadOp = initInfo.clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
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
    if (settings.include_color) {
        attachemnt = {};
        attachemnt.format = initInfo.format;
        attachemnt.samples = initInfo.samples;
        attachemnt.loadOp = initInfo.clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachemnt.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachemnt.flags = 0;

        // REFERENCE (point to swapchain attachment)
        resources.resolveAttachments.resize(1);
        resources.resolveAttachments.back().attachment = static_cast<uint32_t>(resources.attachments.size() - 1);
        resources.resolveAttachments.back().layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        resources.attachments.push_back(attachemnt);

        // point to multi-sample attachment
        resources.colorAttachments.back().attachment = static_cast<uint32_t>(resources.attachments.size() - 1);
    }

    // TODO: this depth attachment shouldn't be added if there is no depth

    // DEPTH ATTACHMENT
    if (settings.include_depth) {
        attachemnt = {};
        attachemnt.format = initInfo.depthFormat;
        attachemnt.samples = initInfo.samples;
        attachemnt.loadOp = initInfo.clearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachemnt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachemnt.flags = 0;
    }

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

    addDefaultSubpasses(resources, PIPELINE_TYPES.size());

    //// Desparate attempt to understand the pipline ordering issue below...
    // VkSubpassDependency dependency = {};

    // dependency = {};
    // dependency.srcSubpass = 0;
    // dependency.dstSubpass = 2;
    // dependency.dependencyFlags = 0;
    // dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    // dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    // dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    // dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    // resources.dependencies.push_back(dependency);

    // dependency = {};
    // dependency.srcSubpass = 1;
    // dependency.dstSubpass = 2;
    // dependency.dependencyFlags = 0;
    // dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    // dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    // dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    // dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    // resources.dependencies.push_back(dependency);
}

void RenderPass::Default::createFramebuffers(const Shell::Context& ctx, const Game::Settings& settings) {
    /*  These are the views for framebuffer.

        view[0] is swapchain
        view[1] is mulit-sample (resolve)
        view[2] is depth

        The incoming "views" param are the swapchain views.
    */
    std::vector<VkImageView> attachmentViews(1);

    // colors
    for (const auto& color : colors_)
        if (color.view != VK_NULL_HANDLE) attachmentViews.push_back(color.view);

    // depth
    if (depth_.view != VK_NULL_HANDLE) attachmentViews.push_back(depth_.view);

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
        data.tests.push_back(1);
    }
}
