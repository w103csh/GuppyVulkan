
#include "RenderPassDefault.h"

#include <utility>

#include "Constants.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "RenderPassHandler.h"
#include "SceneHandler.h"

RenderPass::Default::Default(Handler& handler, const uint32_t&& offset, const RenderPass::CreateInfo* pCreateInfo)
    : Base{handler, std::forward<const uint32_t>(offset), pCreateInfo},  //
      includeMultiSampleAttachment_(true),
      inheritInfo_{},
      secCmdBeginInfo_{},
      secCmdFlag_(false) {}

void RenderPass::Default::init() {
    auto& ctx = handler().shell().context();

    // FRAME DATA
    format_ = ctx.surfaceFormat.format;
    depthFormat_ = ctx.depthFormat;
#ifdef USE_DEBUG_UI
    finalLayout_ = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#endif
    samples_ = ctx.samples;

    // SYNC
    commandCount_ = ctx.imageCount;
    if (finalLayout_ ^ VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
        semaphoreCount_ = ctx.imageCount;
        data.signalSrcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
}

void RenderPass::Default::setSwapchainInfo() {
    extent_ = handler().shell().context().extent;  //
}

void RenderPass::Default::createBeginInfo() {
    RenderPass::Base::createBeginInfo();

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

void RenderPass::Default::createCommandBuffers() {
    RenderPass::Base::createCommandBuffers();
    // SECONDARY
    data.secCmds.resize(handler().shell().context().imageCount);
    handler().commandHandler().createCmdBuffers(handler().commandHandler().graphicsCmdPool(), data.secCmds.data(),
                                                VK_COMMAND_BUFFER_LEVEL_SECONDARY, handler().shell().context().imageCount);
}

void RenderPass::Default::createColorResources() {
    auto& ctx = handler().shell().context();
    if (includeMultiSampleAttachment_) {
        assert(resources_.resolveAttachments.size() == 1 && "Make sure multi-sampling attachment reference was added");

        images_.push_back({});

        helpers::createImage(ctx.dev, ctx.mem_props, handler().commandHandler().getUniqueQueueFamilies(true, false, true),
                             samples_, format_, VK_IMAGE_TILING_OPTIMAL,
                             VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, extent_.width, extent_.height, 1, 1, images_.back().image,
                             images_.back().memory);

        helpers::createImageView(ctx.dev, images_.back().image, 1, format_, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D,
                                 1, images_.back().view);

        helpers::transitionImageLayout(handler().commandHandler().graphicsCmd(), images_.back().image, format_,
                                       VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 1,
                                       1);
    } else {
        assert(resources_.resolveAttachments.size() == 0 && "Make sure multi-sampling attachment reference was not added");
    }
}

void RenderPass::Default::createDepthResource() {
    auto& ctx = handler().shell().context();
    if (includeDepth_) {
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
                             samples_, depthFormat_, tiling, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                             VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, extent_.width, extent_.height, 1, 1, depth_.image,
                             depth_.memory);

        VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (helpers::hasStencilComponent(depthFormat_)) {
            aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        helpers::createImageView(ctx.dev, depth_.image, 1, depthFormat_, aspectFlags, VK_IMAGE_VIEW_TYPE_2D, 1, depth_.view);

        helpers::transitionImageLayout(handler().commandHandler().graphicsCmd(), depth_.image, depthFormat_,
                                       VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 1, 1);
    }
}

void RenderPass::Default::createAttachmentsAndSubpasses() {
    // DEPTH ATTACHMENT
    if (includeDepth_) {
        resources_.attachments.push_back({});
        resources_.attachments.back().format = getDepthFormat();
        resources_.attachments.back().samples = getSamples();
        resources_.attachments.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;  // TODO: clear used to be a param
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
    if (includeMultiSampleAttachment_) {
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

    bool clearAttachment = true;
    if (SWAPCHAIN_DEPENDENT) clearAttachment = !handler().getSwapchainCleared();

    // COLOR ATTACHMENT
    resources_.attachments.push_back({});
    resources_.attachments.back().format = getFormat();
    resources_.attachments.back().samples = VK_SAMPLE_COUNT_1_BIT;
    resources_.attachments.back().loadOp = clearAttachment ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    resources_.attachments.back().storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resources_.attachments.back().stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resources_.attachments.back().stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resources_.attachments.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resources_.attachments.back().finalLayout = getFinalLayout();
    resources_.attachments.back().flags = 0;

    // REFERENCE
    if (includeMultiSampleAttachment_) {
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
    subpass.pDepthStencilAttachment = &resources_.depthStencilAttachment;

    resources_.subpasses.assign(getPipelineCount(), subpass);
}

void RenderPass::Default::createDependencies() {
    AddDefaultSubpasses(resources_, pipelineTypeReferenceMap_.size());

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

void RenderPass::Default::updateClearValues() {
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
    if (includeDepth_) {
        clearValues_.push_back({});
        clearValues_.back().depthStencil = DEFAULT_CLEAR_DEPTH_STENCIL_VALUE;
    }
    // MULTI-SAMPLE ATTACHMENT
    if (includeMultiSampleAttachment_) {
        clearValues_.push_back({});
        clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
    }
    // SWAPCHAIN ATTACHMENT
    clearValues_.push_back({});
    clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
}

void RenderPass::Default::createFramebuffers() {
    /* These are the potential views for framebuffer.
     *  - depth
     *  - mulit-sample
     *  - swapchain
     */
    std::vector<VkImageView> attachmentViews;
    // DEPTH
    if (includeDepth_) {
        assert(depth_.view != VK_NULL_HANDLE);
        attachmentViews.push_back(depth_.view);
    }
    // MULTI-SAMPLE
    if (includeMultiSampleAttachment_) {
        assert(images_.size() == 1);
        assert(images_[0].view != VK_NULL_HANDLE);
        attachmentViews.push_back(images_[0].view);
    }
    // SWAPCHAIN
    if (SWAPCHAIN_DEPENDENT) {
        attachmentViews.push_back({});
    }

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = pass;
    createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
    createInfo.pAttachments = attachmentViews.data();
    createInfo.width = extent_.width;
    createInfo.height = extent_.height;
    createInfo.layers = 1;

    data.framebuffers.resize(handler().getSwapchainImageCount());
    for (auto i = 0; i < data.framebuffers.size(); i++) {
        // TODO: HOLY SHIT DOES THIS MEAN THAT THERE SHOULD BE VIEWS
        // FOR EVERY PASS FRAMEBUFFER???????????
        if (SWAPCHAIN_DEPENDENT) {
            attachmentViews.back() = handler().getSwapchainViews()[i];
        }
        // Create the framebuffer...
        vk::assert_success(
            vkCreateFramebuffer(handler().shell().context().dev, &createInfo, nullptr, &data.framebuffers[i]));
    }
}
