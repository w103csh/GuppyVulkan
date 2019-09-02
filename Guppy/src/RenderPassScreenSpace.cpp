
#include "RenderPassScreenSpace.h"

#include "Sampler.h"
#include "ScreenSpace.h"
// HANDLERS
#include "CommandHandler.h"
#include "DescriptorHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"
#include "TextureHandler.h"

namespace RenderPass {
namespace ScreenSpace {

// CREATE INFO

const CreateInfo CREATE_INFO = {
    PASS::SCREEN_SPACE,
    "Screen Space Swapchain Render Pass",
    {
        PIPELINE::SCREEN_SPACE_DEFAULT,
    },
    FLAG::SWAPCHAIN,
    {},
    {
        PASS::SCREEN_SPACE_HDR_LOG,
        PASS::SCREEN_SPACE_BRIGHT,
        PASS::SCREEN_SPACE_BLUR_A,
        PASS::SCREEN_SPACE_BLUR_B,
    },
};

// BASE

Base::Base(RenderPass::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo)
    : RenderPass::Base{handler, std::forward<const index>(offset), pCreateInfo} {
    status_ = STATUS::PENDING_MESH | STATUS::PENDING_PIPELINE;
    // Validate the pipelines used are in the right spot in the vertex map.
    // for (const auto& [pipelineType, value] : pipelineTypeBindDataMap_)
    //    assert(Pipeline::VERTEX_MAP.at(VERTEX::SCREEN_QUAD).count(pipelineType) != 0);
}

// TODO: should something like this be on the base class????
void Base::init() {
    // Initialize the dependent pass types. Currently they all will come prior to the
    // main loop pass.
    for (const auto& [passType, offset] : dependentTypeOffsetPairs_) {
        // Boy is this going to be confusing if it ever doesn't work right.
        if (passType == TYPE) {
            RenderPass::Base::init();
        } else {
            const auto& pPass = handler().getPass(offset);
            if (!pPass->isIntialized()) const_cast<RenderPass::Base*>(pPass.get())->init();
        }
    }
}

void Base::record(const uint8_t frameIndex) {
    beginInfo_.framebuffer = data.framebuffers[frameIndex];
    auto& priCmd = data.priCmds[frameIndex];

    // RESET BUFFERS
    vkResetCommandBuffer(priCmd, 0);

    // BEGIN BUFFERS
    VkCommandBufferBeginInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    // TODO: use VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT since this will only update after
    // swapchain recreation.
    vk::assert_success(vkBeginCommandBuffer(priCmd, &bufferInfo));

    if (getStatus() != STATUS::READY) update();
    if (getStatus() == STATUS::READY) {
        assert(dependentTypeOffsetPairs_.size() == 5);
        uint32_t passIndex = 0;

        // HDR LOG BLIT
        {
            const auto& pPass = handler().getPass(dependentTypeOffsetPairs_[passIndex++].second);
            if (pPass->getStatus() != STATUS::READY) const_cast<RenderPass::Base*>(pPass.get())->update();
            if (pPass->getStatus() == STATUS::READY) {
                pPass->beginPass(priCmd, frameIndex);

                //::ScreenSpace::PushConstant pc = {::ScreenSpace::BLOOM_BRIGHT};
                // const auto& pPipelineBindData = pPass->getPipelineBindDataMap().begin()->second;
                // vkCmdPushConstants(priCmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                //                   sizeof(::ScreenSpace::PushConstant), &pc);

                auto it = pPass->getPipelineBindDataMap().begin();
                handler().getScreenQuad()->draw(TYPE, it->second, pPass->getDescSetBindDataMap(it->first).begin()->second,
                                                priCmd, frameIndex);
                pPass->endPass(priCmd);

                assert(pPass->TYPE == PASS::SCREEN_SPACE_HDR_LOG);
                ((HdrLog*)pPass.get())->downSample(priCmd, frameIndex);
            }
        }

        // BRIGHT
        {
            const auto& pPass = handler().getPass(dependentTypeOffsetPairs_[passIndex++].second);
            if (pPass->getStatus() != STATUS::READY) const_cast<RenderPass::Base*>(pPass.get())->update();
            if (pPass->getStatus() == STATUS::READY) {
                pPass->beginPass(priCmd, frameIndex);

                ::ScreenSpace::PushConstant pushConstant = {::ScreenSpace::BLOOM_BRIGHT};
                auto it = pPass->getPipelineBindDataMap().begin();
                vkCmdPushConstants(priCmd, it->second->layout, it->second->pushConstantStages, 0,
                                   static_cast<uint32_t>(sizeof(::ScreenSpace::PushConstant)), &pushConstant);

                handler().getScreenQuad()->draw(TYPE, it->second, pPass->getDescSetBindDataMap(it->first).begin()->second,
                                                priCmd, frameIndex);
                pPass->endPass(priCmd);
            }
        }

        // BLUR A
        {
            const auto& pPass = handler().getPass(dependentTypeOffsetPairs_[passIndex++].second);
            if (pPass->getStatus() != STATUS::READY) const_cast<RenderPass::Base*>(pPass.get())->update();
            if (pPass->getStatus() == STATUS::READY) {
                pPass->beginPass(priCmd, frameIndex);

                ::ScreenSpace::PushConstant pushConstant = {::ScreenSpace::BLOOM_BLUR_A};
                auto it = pPass->getPipelineBindDataMap().begin();
                vkCmdPushConstants(priCmd, it->second->layout, it->second->pushConstantStages, 0,
                                   static_cast<uint32_t>(sizeof(::ScreenSpace::PushConstant)), &pushConstant);

                handler().getScreenQuad()->draw(TYPE, it->second, pPass->getDescSetBindDataMap(it->first).begin()->second,
                                                priCmd, frameIndex);
                pPass->endPass(priCmd);
            }
        }

        barrierResource_.reset();
        barrierResource_.glblBarriers.push_back({VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr});
        barrierResource_.glblBarriers.back().srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        barrierResource_.glblBarriers.back().dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        helpers::recordBarriers(barrierResource_, priCmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

        // BLUR B
        {
            const auto& pPass = handler().getPass(dependentTypeOffsetPairs_[passIndex++].second);
            if (pPass->getStatus() != STATUS::READY) const_cast<RenderPass::Base*>(pPass.get())->update();
            if (pPass->getStatus() == STATUS::READY) {
                pPass->beginPass(priCmd, frameIndex);

                ::ScreenSpace::PushConstant pushConstant = {::ScreenSpace::BLOOM_BLUR_B};
                auto it = pPass->getPipelineBindDataMap().begin();
                vkCmdPushConstants(priCmd, it->second->layout, it->second->pushConstantStages, 0,
                                   static_cast<uint32_t>(sizeof(::ScreenSpace::PushConstant)), &pushConstant);

                handler().getScreenQuad()->draw(TYPE, it->second, pPass->getDescSetBindDataMap(it->first).begin()->second,
                                                priCmd, frameIndex);
                pPass->endPass(priCmd);
            }
        }

        // SCREEN SPACE DEFAULT
        {
            beginPass(priCmd, frameIndex);

            auto it = pipelineTypeBindDataMap_.begin();
            ::ScreenSpace::PushConstant pushConstant = {::ScreenSpace::PASS_FLAG::BLOOM};
            vkCmdPushConstants(priCmd, it->second->layout, it->second->pushConstantStages, 0,
                               static_cast<uint32_t>(sizeof(::ScreenSpace::PushConstant)), &pushConstant);

            handler().getScreenQuad()->draw(TYPE, it->second, getDescSetBindDataMap(it->first).begin()->second, priCmd,
                                            frameIndex);

            endPass(priCmd);
        }
    }

    // vk::assert_success(vkEndCommandBuffer(data.priCmds[frameIndex]));
}

void Base::update() {
    // Check the mesh status.
    if (handler().getScreenQuad()->getStatus() == STATUS::READY) {
        status_ ^= STATUS::PENDING_MESH;
        RenderPass::Base::update();
    }
}

// BRIGHT
const CreateInfo BRIGHT_CREATE_INFO = {
    PASS::SCREEN_SPACE_BRIGHT,
    "Screen Space Bright Render Pass",
    {
        PIPELINE::SCREEN_SPACE_BRIGHT,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::BLUR_A_2D_TEXTURE_ID)},
    {},
    {},
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
};
Bright::Bright(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::ScreenSpace::Base{handler, std::forward<const index>(offset), &BRIGHT_CREATE_INFO} {}

// BLUR A
const CreateInfo BLUR_A_CREATE_INFO = {
    PASS::SCREEN_SPACE_BLUR_A,
    "Screen Space Blur A Render Pass",
    {
        PIPELINE::SCREEN_SPACE_BLUR_A,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::BLUR_B_2D_TEXTURE_ID)},
    {},
    {},
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
};
BlurA::BlurA(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::ScreenSpace::Base{handler, std::forward<const index>(offset), &BLUR_A_CREATE_INFO} {}

// BLUR B
const CreateInfo BLUR_B_CREATE_INFO = {
    PASS::SCREEN_SPACE_BLUR_B,
    "Screen Space Blur B Render Pass",
    {
        PIPELINE::SCREEN_SPACE_BLUR_B,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::BLUR_A_2D_TEXTURE_ID)},
    {},
    {},
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
};
BlurB::BlurB(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::ScreenSpace::Base{handler, std::forward<const index>(offset), &BLUR_B_CREATE_INFO} {}

// HDR LOG

const CreateInfo HDR_LOG_CREATE_INFO = {
    PASS::SCREEN_SPACE_HDR_LOG,
    "Screen Space HDR Log Render Pass",
    {
        PIPELINE::SCREEN_SPACE_HDR_LOG,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::HDR_LOG_2D_TEXTURE_ID)},
    {},
    {},
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
};

HdrLog::HdrLog(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::ScreenSpace::Base{handler, std::forward<const index>(offset), &HDR_LOG_CREATE_INFO} {}

void transferAllToTransDst(const VkCommandBuffer& cmd, const Sampler::Base& sampler, BarrierResource& res,
                           VkImageLayout oldLayout) {
    res.reset();

    res.imgBarriers.push_back({});
    res.imgBarriers.back().sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    res.imgBarriers.back().pNext = nullptr;
    res.imgBarriers.back().srcAccessMask = 0;
    res.imgBarriers.back().dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    res.imgBarriers.back().oldLayout = oldLayout;
    res.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    res.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().image = sampler.image;
    res.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, sampler.imgCreateInfo.mipLevels, 0,
                                               sampler.imgCreateInfo.arrayLayers};

    helpers::recordBarriers(res, cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
}

void HdrLog::downSample(const VkCommandBuffer& priCmd, const uint8_t frameIndex) {
    if (transfCmds_.empty()) {
        transfCmds_.resize(handler().shell().context().imageCount);
        handler().commandHandler().createCmdBuffers(QUEUE::GRAPHICS, transfCmds_.data(), VK_COMMAND_BUFFER_LEVEL_SECONDARY,
                                                    handler().shell().context().imageCount);
        for (uint8_t i = 0; static_cast<uint32_t>(i) < handler().shell().context().imageCount; i++) passFlags_[i] = true;
    }
    const auto& cmd = transfCmds_[frameIndex];

    // RESET BUFFERS
    vkResetCommandBuffer(cmd, 0);

    // BEGIN BUFFERS
    VkCommandBufferBeginInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VkCommandBufferInheritanceInfo bufferInheritanceInfo = {};
    bufferInheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    bufferInfo.pInheritanceInfo = &bufferInheritanceInfo;
    // TODO: use VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT since this will only update after
    // swapchain recreation.
    vk::assert_success(vkBeginCommandBuffer(cmd, &bufferInfo));

    // Wait on log average luminance render target
    barrierResource_.reset();
    barrierResource_.glblBarriers.push_back({});
    barrierResource_.glblBarriers.back().sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
    barrierResource_.glblBarriers.back().pNext = nullptr;
    barrierResource_.glblBarriers.back().srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrierResource_.glblBarriers.back().dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    helpers::recordBarriers(barrierResource_, cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT);

    // Source of the blit (render targets can have multiple mip levels)
    const auto& hdrLogSampler = pTextures_[frameIndex]->samplers[0];
    // Sampler with enough mip levels to downsample to a single pixel.
    const auto& blitSamplerA =
        handler().textureHandler().getTexture(Texture::ScreenSpace::HDR_LOG_BLIT_A_2D_TEXTURE_ID, frameIndex)->samplers[0];

    // Transfer all mip levels
    transferAllToTransDst(cmd, blitSamplerA, barrierResource_,
                          (passFlags_[frameIndex] ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL));
    passFlags_[frameIndex] = false;

    int32_t mipWidth = hdrLogSampler.imgCreateInfo.extent.width;
    int32_t mipHeight = hdrLogSampler.imgCreateInfo.extent.height;

    VkImageBlit blit = {};
    // source
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = blitSamplerA.imgCreateInfo.arrayLayers;

    mipWidth /= 2;
    mipHeight /= 2;

    // destination
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {mipWidth, mipHeight, 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = blitSamplerA.imgCreateInfo.arrayLayers;

    vkCmdBlitImage(cmd, hdrLogSampler.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, blitSamplerA.image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = blitSamplerA.image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = blitSamplerA.imgCreateInfo.arrayLayers;
    barrier.subresourceRange.levelCount = 1;

    for (uint32_t i = 1; i < blitSamplerA.imgCreateInfo.mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr,
                             1, &barrier);

        blit = {};
        // source
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = blitSamplerA.imgCreateInfo.arrayLayers;

        mipWidth /= 2;
        mipHeight /= 2;

        // destination
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth, mipHeight, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = blitSamplerA.imgCreateInfo.arrayLayers;

        vkCmdBlitImage(cmd, blitSamplerA.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, blitSamplerA.image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        // Validation layers complain about every mip level that is not shader ready only optimal.
        // TODO: make the image view of the blit sampler only refer to the single pixel mip level so
        // these transition are not needed.
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                             nullptr, 1, &barrier);
    }

    // Single pixel mip needs to be shader read only optimal.
    barrier.subresourceRange.baseMipLevel = blitSamplerA.imgCreateInfo.mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    vk::assert_success(vkEndCommandBuffer(cmd));
    vkCmdExecuteCommands(priCmd, 1, &cmd);
}

}  // namespace ScreenSpace

}  // namespace RenderPass
