/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Compute.h"

#include "Helpers.h"
#include "ScreenSpace.h"
// HANDLERS
#include "CommandHandler.h"
#include "ComputeHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"

Compute::Base::Base(Compute::Handler& handler, const Compute::CreateInfo* pCreateInfo)
    : Handlee(handler),
      HAS_FENCE(pCreateInfo->needsFence),
      PIPELINE_TYPE(pCreateInfo->pipelineType),
      PASS_TYPE(pCreateInfo->passType),
      POST_PASS_TYPE(pCreateInfo->postPassType),
      QUEUE_TYPE(pCreateInfo->queueType),
      groupCountX_(pCreateInfo->groupCountX),
      groupCountY_(pCreateInfo->groupCountY),
      groupCountZ_(pCreateInfo->groupCountZ),
      status_(STATUS::PENDING),
      isFramebufferCountDependent_(false),
      isFramebufferImageSizeDependent_(false),
      fence_() {
    // SYNC
    if (pCreateInfo->syncCount == -1) {
        isFramebufferCountDependent_ = true;
    } else {
        assert(pCreateInfo->syncCount > 0);
        cmds_.resize(pCreateInfo->syncCount);
        semaphores_.resize(pCreateInfo->syncCount);
    }
    // GROUP COUNT
    if (groupCountX_ == UINT32_MAX && groupCountY_ == UINT32_MAX && groupCountZ_ == UINT32_MAX) {
        isFramebufferImageSizeDependent_ = true;
    } else {
        assert(groupCountX_ != UINT32_MAX);
        assert(groupCountY_ != UINT32_MAX);
        assert(groupCountZ_ != UINT32_MAX);
    }
}

void Compute::Base::init() {
    // SYNC
    if (!isFramebufferCountDependent_) createSyncResources();

    status_ = STATUS::PENDING_PIPELINE;
}

void Compute::Base::prepare() {
    assert(status_ != STATUS::READY);

    if (status_ & STATUS::PENDING_PIPELINE) {
        auto& pPipeline = handler().pipelineHandler().getPipeline(PIPELINE_TYPE);
        if (pPipeline->getStatus() == STATUS::READY)
            status_ ^= STATUS::PENDING_PIPELINE;
        else
            pPipeline->updateStatus();
    } else {
        // Nothing else to wait on atm.
        assert(false);
    }
}

void Compute::Base::setBindData(const PIPELINE& pipelineType, const std::shared_ptr<Pipeline::BindData>& pBindData) {
    pipelineBindDataList_.insert(pipelineType, pBindData);
}

void Compute::Base::record(const uint8_t frameIndex, RenderPass::SubmitResource& submitResource) {
    // auto syncIndex = (std::min)(static_cast<uint8_t>(cmds_.size() - 1), frameIndex);

    // COMMAND
    auto& cmd = submitResource.commandBuffers[submitResource.commandBufferCount - 1];
    // auto& cmd = cmds_[syncIndex];
    // cmd.reset({});
    // handler().commandHandler().beginCmd(cmd);

    // PIPELINE
    assert(pipelineBindDataList_.size() == 1);  // TODO: This shouldn't be a map right? 1 to 1.
    const auto& pPipelineBindData = pipelineBindDataList_.getValue(PIPELINE_TYPE);

    // DESCRIPTOR
    assert(bindDataMap_.size() == 1 &&
           bindDataMap_.begin()->first.count(PASS_TYPE));  // TODO: This shouldn't be a map right? 1 to 1.
    const auto& descSetBindData = bindDataMap_.begin()->second;

    // PRE-DISPATCH
    preDispatch(cmd, pPipelineBindData, descSetBindData, frameIndex);

    // PUSH CONSTANT
    bindPushConstants(cmd);

    cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                           descSetBindData.descriptorSets[frameIndex], descSetBindData.dynamicOffsets);

    // DISPATCH
    cmd.dispatch(groupCountX_, groupCountY_, groupCountZ_);

    // POST-DISPATCH
    postDispatch(cmd, pPipelineBindData, descSetBindData, frameIndex);

    // cmd.end();

    //// SUBMIT INFO
    // submitResource.commandBufferCount = 1;
    // submitResource.commandBuffers[0] = cmd;
}

void Compute::Base::createSyncResources() {
    const auto& ctx = handler().shell().context();
    // COMMAND
    if (cmds_.empty()) cmds_.resize(ctx.imageCount);
    handler().commandHandler().createCmdBuffers(QUEUE_TYPE, cmds_.data(), vk::CommandBufferLevel::ePrimary, cmds_.size());
    // SEMAPHORE
    vk::SemaphoreCreateInfo createInfo = {};
    if (semaphores_.empty()) semaphores_.resize(ctx.imageCount);
    for (auto& semaphore : semaphores_)
        semaphore = handler().shell().context().dev.createSemaphore(createInfo, ALLOC_PLACE_HOLDER);
    // FENCE
    if (HAS_FENCE && !fence_) {
        vk::FenceCreateInfo fenceInfo = {vk::FenceCreateFlagBits::eSignaled};
        fence_ = ctx.dev.createFence(fenceInfo, ALLOC_PLACE_HOLDER);
    }
}

void Compute::Base::attachSwapchain() {
    const auto& ctx = handler().shell().context();
    // SYNC
    if (isFramebufferCountDependent_) {
        assert(cmds_.empty());
        cmds_.resize(ctx.imageCount);
        createSyncResources();
    }
    // GROUP COUNT
    if (isFramebufferImageSizeDependent_) {
        groupCountX_ = ctx.extent.width;
        groupCountY_ = ctx.extent.height;
        groupCountZ_ = 1;
    }
}

void Compute::Base::destroySyncResources() {
    const auto& dev = handler().shell().context().dev;
    // COMMAND
    helpers::destroyCommandBuffers(dev, handler().commandHandler().getCmdPool(QUEUE_TYPE), cmds_);
    cmds_.clear();
    // SEMAPHORE
    for (auto& semaphore : semaphores_) dev.destroySemaphore(semaphore, ALLOC_PLACE_HOLDER);
    semaphores_.clear();
}

void Compute::Base::detachSwapchain() {
    // SYNC
    if (isFramebufferCountDependent_) {
        destroySyncResources();
    }
}

void Compute::Base::destroy() {
    const auto& dev = handler().shell().context().dev;
    // SYNC
    if (!isFramebufferCountDependent_) {
        destroySyncResources();
    }
    // FENCE
    dev.destroyFence(fence_, ALLOC_PLACE_HOLDER);
}

// POST-PROCESS

namespace Compute {

const CreateInfo PostProcess::DEFAULT_CREATE_INFO = {
    COMPUTE::SCREEN_SPACE_DEFAULT,
    PASS::COMPUTE_POST_PROCESS,
    QUEUE::GRAPHICS,
    PASS::SAMPLER_DEFAULT,
};

PostProcess::Default::Default(Handler& handler)  //
    : Compute::Base{handler, &DEFAULT_CREATE_INFO} {}

void PostProcess::Default::record(const uint8_t frameIndex, RenderPass::SubmitResource& submitResource) {
    // auto syncIndex = (std::min)(static_cast<uint8_t>(cmds_.size() - 1), frameIndex);

    // COMMAND
    auto& cmd = submitResource.commandBuffers[submitResource.commandBufferCount - 1];
    // auto& cmd = cmds_[syncIndex];
    // cmd.reset({});
    // cmd.begin();

    // PIPELINE
    assert(pipelineBindDataList_.size() == 1);  // TODO: This shouldn't be a map right? 1 to 1.
    const auto& pPipelineBindData = pipelineBindDataList_.getValue(PIPELINE_TYPE);

    // DESCRIPTOR
    assert(bindDataMap_.size() == 1 &&
           bindDataMap_.begin()->first.count(PASS_TYPE));  // TODO: This shouldn't be a map right? 1 to 1.
    const auto& descSetBindData = bindDataMap_.begin()->second;

    // PRE-DISPATCH
    preDispatch(cmd, pPipelineBindData, descSetBindData, frameIndex);

    // PUSH CONSTANT
    Compute::PostProcess::PushConstant pushConstant = {
        ScreenSpace::PASS_FLAG::EDGE,
        // ScreenSpace::PASS_FLAG::BLUR_1,
        // ScreenSpace::PASS_FLAG::HDR_1,
    };
    cmd.pushConstants(pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                      sizeof(Compute::PostProcess::PushConstant), &pushConstant);

    cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                           descSetBindData.descriptorSets[frameIndex], descSetBindData.dynamicOffsets);

    // DISPATCH
    cmd.dispatch(groupCountX_, groupCountY_, groupCountZ_);

    barrierResource_.reset();
    auto& resource = barrierResource_;
    resource.imgBarriers.push_back({});
    resource.imgBarriers.back().srcAccessMask = vk::AccessFlagBits::eShaderWrite;
    resource.imgBarriers.back().dstAccessMask = vk::AccessFlagBits::eShaderWrite;
    resource.imgBarriers.back().oldLayout = vk::ImageLayout::eGeneral;
    resource.imgBarriers.back().newLayout = vk::ImageLayout::eColorAttachmentOptimal;
    resource.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    resource.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    resource.imgBarriers.back().image = handler().passHandler().getCurrentFramebufferImage();
    resource.imgBarriers.back().subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    helpers::recordBarriers(barrierResource_, cmd, vk::PipelineStageFlagBits::eComputeShader,
                            vk::PipelineStageFlagBits::eFragmentShader);

    // barrierResource_.reset();
    // helpers::attachementImageBarrierStorageWriteToColorRead(handler().passHandler().getCurrentFramebufferImage(),
    //                                                        barrierResource_);
    // helpers::recordBarriers(barrierResource_, cmd, vk::PipelineStageFlagBits::eComputeShader,
    //                        vk::PipelineStageFlagBits::eFragmentShader);

    // const auto& imageInfo =
    //    descSetBindData.setResourceInfoMap.at(DESCRIPTOR_SET::STORAGE_IMAGE_SCREEN_SPACE_COMPUTE_DEFAULT)
    //        ->at({0, STORAGE_IMAGE::PIPELINE})
    //        .imageInfos[frameIndex];
    // const auto& bufferInfo =
    //    descSetBindData.setResourceInfoMap.at(DESCRIPTOR_SET::STORAGE_SCREEN_SPACE_COMPUTE_POST_PROCESS)
    //        ->at({0, STORAGE_BUFFER::POST_PROCESS})
    //        .bufferInfos[frameIndex];

    // barrierResource_.reset();
    //// TODO: fetch this on attachSwapchain
    // helpers::storageImageBarrierWriteToRead(handler().passHandler().getCurrentFramebufferImage(), barrierResource_);
    //// TODO: fetch this on attachSwapchain
    // helpers::bufferBarrierWriteToRead(bufferInfo, barrierResource_);

    //// helpers::recordBarriers(barrierResource_, cmd, vk::PipelineStageFlagBits::eComputeShader,
    ////                        vk::PipelineStageFlagBits::eComputeShader);
    // helpers::recordBarriers(barrierResource_, cmd, vk::PipelineStageFlagBits::eComputeShader,
    //                        vk::PipelineStageFlagBits::eFragmentShader);

    // if (false) {
    //    // PUSH CONSTANT
    //    pushConstant = {
    //        // 0,
    //        ScreenSpace::PASS_FLAG::BLUR_2,
    //        // ScreenSpace::PASS_FLAG::HDR_2,
    //    };
    //    cmd.pushConstants(pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
    //                      static_cast<uint32_t>(sizeof(Compute::PostProcess::PushConstant)), &pushConstant);

    //    // DISPATCH
    //    cmd.dispatch(groupCountX_, groupCountY_, groupCountZ_);
    //    // cmd.dispatch(1, 1, 1);

    //    // SECOND BARRIER
    //    barrierResource_.reset();
    //    // TODO: fetch this on attachSwapchain
    //    helpers::storageImageBarrierWriteToRead(imageInfo.image, barrierResource_);
    //    // TODO: fetch this on attachSwapchain
    //    helpers::bufferBarrierWriteToRead(bufferInfo, barrierResource_);

    //    helpers::recordBarriers(barrierResource_, cmd, vk::PipelineStageFlagBits::eComputeShader,
    //                            vk::PipelineStageFlagBits::eFragmentShader);
    //}

    // cmd.end();

    //// SUBMIT INFO
    // submitResource.signalSemaphores[submitResource.signalSemaphoreCount] = semaphores_[syncIndex];
    // submitResource.signalSrcStageMasks[submitResource.signalSemaphoreCount++] = vk::PipelineStageFlagBits::eBottomOfPipe;
    // submitResource.commandBufferCount = 1;
    // submitResource.commandBuffers[0] = cmd;
}

void PostProcess::Default::preDispatch(const vk::CommandBuffer& cmd,
                                       const std::shared_ptr<Pipeline::BindData>& pPplnBindData,
                                       const Descriptor::Set::BindData& descSetBindData, const uint8_t frameIndex) {
    // barrierResource_.reset();
    //// To me this makes no sense but oh well. It works atm.
    // helpers::attachementImageBarrierWriteToStorageRead(handler().passHandler().getCurrentFramebufferImage(),
    //                                                   barrierResource_);
    // helpers::recordBarriers(barrierResource_, cmd, vk::PipelineStageFlagBits::eColorAttachmentOutput,
    //                        vk::PipelineStageFlagBits::eComputeShader);

    barrierResource_.reset();
    auto& resource = barrierResource_;
    resource.imgBarriers.push_back({});
    resource.imgBarriers.back().srcAccessMask = {};
    resource.imgBarriers.back().dstAccessMask = vk::AccessFlagBits::eShaderWrite;
    resource.imgBarriers.back().oldLayout = vk::ImageLayout::eUndefined;
    resource.imgBarriers.back().newLayout = vk::ImageLayout::eGeneral;
    resource.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    resource.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    resource.imgBarriers.back().image = handler().passHandler().getCurrentFramebufferImage();
    resource.imgBarriers.back().subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

    helpers::recordBarriers(barrierResource_, cmd, vk::PipelineStageFlagBits::eTopOfPipe,
                            vk::PipelineStageFlagBits::eComputeShader);
}

void PostProcess::Default::postDispatch(const vk::CommandBuffer& cmd,
                                        const std::shared_ptr<Pipeline::BindData>& pPplnBindData,
                                        const Descriptor::Set::BindData& descSetBindData, const uint8_t frameIndex) {
    barrierResource_.reset();
    // TODO: fetch this on attachSwapchain
    const auto& imageInfo =
        descSetBindData.setResourceInfoMap.at(DESCRIPTOR_SET::STORAGE_IMAGE_SCREEN_SPACE_COMPUTE_DEFAULT)
            ->at({0, STORAGE_IMAGE::PIPELINE})
            .imageInfos[frameIndex];
    helpers::storageImageBarrierWriteToRead(imageInfo.image, barrierResource_);

    // TODO: fetch this on attachSwapchain
    const auto& bufferInfo =
        descSetBindData.setResourceInfoMap.at(DESCRIPTOR_SET::STORAGE_SCREEN_SPACE_COMPUTE_POST_PROCESS)
            ->at({0, STORAGE_BUFFER::POST_PROCESS})
            .bufferInfos[frameIndex];
    helpers::bufferBarrierWriteToRead(bufferInfo, barrierResource_);

    helpers::recordBarriers(barrierResource_, cmd, vk::PipelineStageFlagBits::eComputeShader,
                            vk::PipelineStageFlagBits::eFragmentShader);
}

}  // namespace Compute
