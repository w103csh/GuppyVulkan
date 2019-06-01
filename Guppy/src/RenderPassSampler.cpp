
#include "RenderPassSampler.h"

#include <utility>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Texture.h"
#include "Helpers.h"
#include "RenderPassDefault.h"
// HANDLERS
#include "CommandHandler.h"
#include "MaterialHandler.h"
#include "RenderPassHandler.h"
#include "TextureHandler.h"

RenderPass::Sampler::Sampler(RenderPass::Handler& handler, const uint32_t&& offset)
    : RenderPass::Default{handler, std::forward<const uint32_t>(offset), &SAMPLER_CREATE_INFO}, pTexture_(nullptr) {}

void RenderPass::Sampler::init() {
    auto& ctx = handler().shell().context();

    // FRAME DATA
    format_ = ctx.surfaceFormat.format;
    depthFormat_ = ctx.depthFormat;
    finalLayout_ = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    samples_ = VK_SAMPLE_COUNT_1_BIT;
    extent_ = {1024, 768};
    includeMultiSampleAttachment_ = false;
    // SYNC
    commandCount_ = ctx.imageCount;
    semaphoreCount_ = ctx.imageCount;
    data.signalSrcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    createSampler();
}

void RenderPass::Sampler::overridePipelineCreateInfo(const PIPELINE& type, Pipeline::CreateInfoResources& createInfoRes) {
    createInfoRes.multisampleStateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
}

void RenderPass::Sampler::createSampler() {
    auto& ctx = handler().shell().context();

    // pTexture_ = handler().textureHandler().getTextureByName(TEXTURE_2D_CREATE_INFO.name);
    pTexture_ = handler().textureHandler().getTextureByName(TEXTURE_2D_ARRAY_CREATE_INFO.name);
    assert(pTexture_ != nullptr && pTexture_->samplers.size());

    auto& sampler = pTexture_->samplers.back();

    helpers::createImage(ctx.dev, ctx.mem_props, handler().commandHandler().getUniqueQueueFamilies(true, false, true),
                         samples_, format_, sampler.TILING, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, extent_.width, extent_.height, 1, 1, sampler.image,
                         sampler.memory);

    helpers::createImageView(ctx.dev, sampler.image, 1, format_, VK_IMAGE_ASPECT_COLOR_BIT, sampler.imageViewType, 1,
                             pTexture_->samplers.back().view);

    helpers::transitionImageLayout(handler().commandHandler().graphicsCmd(), sampler.image, format_,
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 1, 1);

    handler().textureHandler().createSampler(ctx.dev, sampler);

    sampler.imgDescInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    sampler.imgDescInfo.imageView = sampler.view;
    sampler.imgDescInfo.sampler = sampler.sampler;

    // TODO: This is getting a little too similar here. Maybe find a way to
    // move this stuff to the texture handler.
    pTexture_->status = STATUS::READY;
    handler().materialHandler().updateTexture(pTexture_);
}

void RenderPass::Sampler::createFramebuffers() {
    /* These are the potential views for framebuffer.
     *  - depth
     *  - sampler
     */
    std::vector<VkImageView> attachmentViews;
    // DEPTH
    if (includeDepth_) {
        assert(depth_.view != VK_NULL_HANDLE);
        attachmentViews.push_back(depth_.view);
    }
    // SAMPLER
    assert(pTexture_->samplers.size() == 1 && pTexture_->samplers[0].view != VK_NULL_HANDLE);
    attachmentViews.push_back(pTexture_->samplers[0].view);

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
        // Create the framebuffer...
        vk::assert_success(
            vkCreateFramebuffer(handler().shell().context().dev, &createInfo, nullptr, &data.framebuffers[i]));
    }
}
