/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

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
        GRAPHICS::SCREEN_SPACE_DEFAULT,
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
    priCmd.reset({});

    // BEGIN BUFFERS
    vk::CommandBufferBeginInfo bufferInfo = {vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    // TODO: use vk::CommandBufferUsageFlagBits::eSimultaneousUse since this will only update after
    // swapchain recreation.
    priCmd.begin(bufferInfo);

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
                // priCmd.pushConstants(pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                //                   sizeof(::ScreenSpace::PushConstant), &pc);

                auto it = pPass->getPipelineBindDataList().getValues().begin();
                handler().getScreenQuad()->draw(TYPE, (*it), pPass->getDescSetBindDataMap((*it)->type).begin()->second,
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
                auto it = pPass->getPipelineBindDataList().getValues().begin();
                priCmd.pushConstants((*it)->layout, (*it)->pushConstantStages, 0,
                                     static_cast<uint32_t>(sizeof(::ScreenSpace::PushConstant)), &pushConstant);

                handler().getScreenQuad()->draw(TYPE, (*it), pPass->getDescSetBindDataMap((*it)->type).begin()->second,
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
                auto it = pPass->getPipelineBindDataList().getValues().begin();
                priCmd.pushConstants((*it)->layout, (*it)->pushConstantStages, 0,
                                     static_cast<uint32_t>(sizeof(::ScreenSpace::PushConstant)), &pushConstant);

                handler().getScreenQuad()->draw(TYPE, (*it), pPass->getDescSetBindDataMap((*it)->type).begin()->second,
                                                priCmd, frameIndex);
                pPass->endPass(priCmd);
            }
        }

        barrierResource_.reset();
        barrierResource_.glblBarriers.push_back({});
        barrierResource_.glblBarriers.back().srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        barrierResource_.glblBarriers.back().dstAccessMask = vk::AccessFlagBits::eShaderRead;
        helpers::recordBarriers(barrierResource_, priCmd, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                vk::PipelineStageFlagBits::eFragmentShader);

        // BLUR B
        {
            const auto& pPass = handler().getPass(dependentTypeOffsetPairs_[passIndex++].second);
            if (pPass->getStatus() != STATUS::READY) const_cast<RenderPass::Base*>(pPass.get())->update();
            if (pPass->getStatus() == STATUS::READY) {
                pPass->beginPass(priCmd, frameIndex);

                ::ScreenSpace::PushConstant pushConstant = {::ScreenSpace::BLOOM_BLUR_B};
                auto it = pPass->getPipelineBindDataList().getValues().begin();
                priCmd.pushConstants((*it)->layout, (*it)->pushConstantStages, 0,
                                     static_cast<uint32_t>(sizeof(::ScreenSpace::PushConstant)), &pushConstant);

                handler().getScreenQuad()->draw(TYPE, (*it), pPass->getDescSetBindDataMap((*it)->type).begin()->second,
                                                priCmd, frameIndex);
                pPass->endPass(priCmd);
            }
        }

        // SCREEN SPACE DEFAULT
        {
            beginPass(priCmd, frameIndex);

            auto it = pipelineBindDataList_.getValues().begin();
            ::ScreenSpace::PushConstant pushConstant = {::ScreenSpace::PASS_FLAG::BLOOM};
            priCmd.pushConstants((*it)->layout, (*it)->pushConstantStages, 0,
                                 static_cast<uint32_t>(sizeof(::ScreenSpace::PushConstant)), &pushConstant);

            handler().getScreenQuad()->draw(TYPE, (*it), pipelineDescSetBindDataMap_.at((*it)->type).begin()->second, priCmd,
                                            frameIndex);

            endPass(priCmd);
        }
    }

    // data.priCmds[frameIndex]).end();
}

void Base::update(const std::vector<Descriptor::Base*> pDynamicItems) {
    // Check the mesh status.
    if (handler().getScreenQuad()->getStatus() == STATUS::READY) {
        status_ ^= STATUS::PENDING_MESH;
        RenderPass::Base::update(pDynamicItems);
    }
}

// BRIGHT
const CreateInfo BRIGHT_CREATE_INFO = {
    PASS::SCREEN_SPACE_BRIGHT,
    "Screen Space Bright Render Pass",
    {
        GRAPHICS::SCREEN_SPACE_BRIGHT,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::BLUR_A_2D_TEXTURE_ID)},
    {},
    {},
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageLayout::eShaderReadOnlyOptimal,
};
Bright::Bright(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::ScreenSpace::Base{handler, std::forward<const index>(offset), &BRIGHT_CREATE_INFO} {}

// BLUR A
const CreateInfo BLUR_A_CREATE_INFO = {
    PASS::SCREEN_SPACE_BLUR_A,
    "Screen Space Blur A Render Pass",
    {
        GRAPHICS::SCREEN_SPACE_BLUR_A,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::BLUR_B_2D_TEXTURE_ID)},
    {},
    {},
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageLayout::eShaderReadOnlyOptimal,
};
BlurA::BlurA(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::ScreenSpace::Base{handler, std::forward<const index>(offset), &BLUR_A_CREATE_INFO} {}

// BLUR B
const CreateInfo BLUR_B_CREATE_INFO = {
    PASS::SCREEN_SPACE_BLUR_B,
    "Screen Space Blur B Render Pass",
    {
        GRAPHICS::SCREEN_SPACE_BLUR_B,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::BLUR_A_2D_TEXTURE_ID)},
    {},
    {},
    vk::ImageLayout::eShaderReadOnlyOptimal,
    vk::ImageLayout::eShaderReadOnlyOptimal,
};
BlurB::BlurB(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::ScreenSpace::Base{handler, std::forward<const index>(offset), &BLUR_B_CREATE_INFO} {}

// HDR LOG

const CreateInfo HDR_LOG_CREATE_INFO = {
    PASS::SCREEN_SPACE_HDR_LOG,
    "Screen Space HDR Log Render Pass",
    {
        GRAPHICS::SCREEN_SPACE_HDR_LOG,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::HDR_LOG_2D_TEXTURE_ID)},
    {},
    {},
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageLayout::eTransferSrcOptimal,
};

HdrLog::HdrLog(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::ScreenSpace::Base{handler, std::forward<const index>(offset), &HDR_LOG_CREATE_INFO} {}

void transferAllToTransDst(const vk::CommandBuffer& cmd, const Sampler::Base& sampler, BarrierResource& res,
                           vk::ImageLayout oldLayout) {
    res.reset();

    res.imgBarriers.push_back({});
    res.imgBarriers.back().srcAccessMask = {};
    res.imgBarriers.back().dstAccessMask = vk::AccessFlagBits::eTransferWrite;
    res.imgBarriers.back().oldLayout = oldLayout;
    res.imgBarriers.back().newLayout = vk::ImageLayout::eTransferDstOptimal;
    res.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    res.imgBarriers.back().image = sampler.image;
    res.imgBarriers.back().subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, sampler.imgCreateInfo.mipLevels, 0,
                                               sampler.imgCreateInfo.arrayLayers};

    helpers::recordBarriers(res, cmd, vk::PipelineStageFlagBits::eFragmentShader, vk::PipelineStageFlagBits::eTransfer);
}

void HdrLog::downSample(const vk::CommandBuffer& priCmd, const uint8_t frameIndex) {
    if (transfCmds_.empty()) {
        transfCmds_.resize(handler().shell().context().imageCount);
        handler().commandHandler().createCmdBuffers(QUEUE::GRAPHICS, transfCmds_.data(), vk::CommandBufferLevel::eSecondary,
                                                    handler().shell().context().imageCount);
        for (uint8_t i = 0; static_cast<uint32_t>(i) < handler().shell().context().imageCount; i++) passFlags_[i] = true;
    }
    const auto& cmd = transfCmds_[frameIndex];

    // RESET BUFFERS
    cmd.reset({});

    // BEGIN BUFFERS
    vk::CommandBufferBeginInfo bufferInfo = {vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    vk::CommandBufferInheritanceInfo bufferInheritanceInfo = {};
    bufferInfo.pInheritanceInfo = &bufferInheritanceInfo;
    // TODO: use vk::CommandBufferUsageFlagBits::eSimultaneousUse since this will only update after
    // swapchain recreation.
    cmd.begin(bufferInfo);

    // Wait on log average luminance render target
    barrierResource_.reset();
    barrierResource_.glblBarriers.push_back({});
    barrierResource_.glblBarriers.back().srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    barrierResource_.glblBarriers.back().dstAccessMask = vk::AccessFlagBits::eTransferRead;
    helpers::recordBarriers(barrierResource_, cmd, vk::PipelineStageFlagBits::eColorAttachmentOutput,
                            vk::PipelineStageFlagBits::eTransfer);

    // Source of the blit (render targets can have multiple mip levels)
    const auto& hdrLogSampler = pTextures_[frameIndex]->samplers[0];
    // Sampler with enough mip levels to downsample to a single pixel.
    const auto& blitSamplerA =
        handler().textureHandler().getTexture(Texture::ScreenSpace::HDR_LOG_BLIT_A_2D_TEXTURE_ID, frameIndex)->samplers[0];

    // Transfer all mip levels
    transferAllToTransDst(cmd, blitSamplerA, barrierResource_,
                          (passFlags_[frameIndex] ? vk::ImageLayout::eUndefined : vk::ImageLayout::eShaderReadOnlyOptimal));
    passFlags_[frameIndex] = false;

    int32_t mipWidth = hdrLogSampler.imgCreateInfo.extent.width;
    int32_t mipHeight = hdrLogSampler.imgCreateInfo.extent.height;

    vk::ImageBlit blit = vk::ImageBlit{};
    // source
    blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
    blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
    blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.srcSubresource.mipLevel = 0;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = blitSamplerA.imgCreateInfo.arrayLayers;

    mipWidth /= 2;
    mipHeight /= 2;

    // destination
    blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
    blit.dstOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
    blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    blit.dstSubresource.mipLevel = 0;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = blitSamplerA.imgCreateInfo.arrayLayers;

    cmd.blitImage(hdrLogSampler.image, vk::ImageLayout::eTransferSrcOptimal, blitSamplerA.image,
                  vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

    vk::ImageMemoryBarrier barrier = {};
    barrier.image = blitSamplerA.image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = blitSamplerA.imgCreateInfo.arrayLayers;
    barrier.subresourceRange.levelCount = 1;

    for (uint32_t i = 1; i < blitSamplerA.imgCreateInfo.mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {}, {}, {},
                            {barrier});

        blit = vk::ImageBlit{};
        // source
        blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = blitSamplerA.imgCreateInfo.arrayLayers;

        mipWidth /= 2;
        mipHeight /= 2;

        // destination
        blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.dstOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = blitSamplerA.imgCreateInfo.arrayLayers;

        cmd.blitImage(blitSamplerA.image, vk::ImageLayout::eTransferSrcOptimal, blitSamplerA.image,
                      vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

        // Validation layers complain about every mip level that is not shader ready only optimal.
        // TODO: make the image view of the blit sampler only refer to the single pixel mip level so
        // these transition are not needed.
        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
                            {barrier});
    }

    // Single pixel mip needs to be shader read only optimal.
    barrier.subresourceRange.baseMipLevel = blitSamplerA.imgCreateInfo.mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {},
                        {barrier});

    cmd.end();
    priCmd.executeCommands({cmd});
}

}  // namespace ScreenSpace

}  // namespace RenderPass
