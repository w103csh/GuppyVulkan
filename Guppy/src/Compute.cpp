
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
      fence_(VK_NULL_HANDLE) {
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
    // vkResetCommandBuffer(cmd, 0);
    // handler().commandHandler().beginCmd(cmd);

    // PIPELINE
    assert(pipelineBindDataList_.size() == 1);  // TODO: This shouldn't be a map right? 1 to 1.
    const auto& pipelineBindData = pipelineBindDataList_.getValue(PIPELINE_TYPE);

    // DESCRIPTOR
    assert(bindDataMap_.size() == 1 &&
           bindDataMap_.begin()->first.count(PASS_TYPE));  // TODO: This shouldn't be a map right? 1 to 1.
    const auto& descSetBindData = bindDataMap_.begin()->second;

    // PRE-DISPATCH
    preDispatch(cmd, pipelineBindData, descSetBindData, frameIndex);

    // PUSH CONSTANT
    bindPushConstants(cmd);

    vkCmdBindPipeline(cmd, pipelineBindData->bindPoint, pipelineBindData->pipeline);

    vkCmdBindDescriptorSets(cmd, pipelineBindData->bindPoint, pipelineBindData->layout, descSetBindData.firstSet,
                            static_cast<uint32_t>(descSetBindData.descriptorSets[frameIndex].size()),
                            descSetBindData.descriptorSets[frameIndex].data(),
                            static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                            descSetBindData.dynamicOffsets.data());

    // DISPATCH
    vkCmdDispatch(cmd, groupCountX_, groupCountY_, groupCountZ_);

    // POST-DISPATCH
    postDispatch(cmd, pipelineBindData, descSetBindData, frameIndex);

    // handler().commandHandler().endCmd(cmd);

    //// SUBMIT INFO
    // submitResource.commandBufferCount = 1;
    // submitResource.commandBuffers[0] = cmd;
}

void Compute::Base::createSyncResources() {
    const auto& ctx = handler().shell().context();
    // COMMAND
    if (cmds_.empty()) cmds_.resize(ctx.imageCount);
    handler().commandHandler().createCmdBuffers(QUEUE_TYPE, cmds_.data(), VK_COMMAND_BUFFER_LEVEL_PRIMARY, cmds_.size());
    // SEMAPHORE
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    if (semaphores_.empty()) semaphores_.resize(ctx.imageCount);
    for (auto& semaphore : semaphores_)
        vk::assert_success(vkCreateSemaphore(handler().shell().context().dev, &createInfo, nullptr, &semaphore));
    // FENCE
    if (HAS_FENCE && fence_ == VK_NULL_HANDLE) {
        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vk::assert_success(vkCreateFence(ctx.dev, &fenceInfo, nullptr, &fence_));
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
    for (auto& semaphore : semaphores_) vkDestroySemaphore(dev, semaphore, nullptr);
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
    vkDestroyFence(dev, fence_, nullptr);
}

// POST-PROCESS

namespace Compute {

const CreateInfo PostProcess::DEFAULT_CREATE_INFO = {
    PIPELINE::SCREEN_SPACE_COMPUTE_DEFAULT,
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
    // vkResetCommandBuffer(cmd, 0);
    // handler().commandHandler().beginCmd(cmd);

    // PIPELINE
    assert(pipelineBindDataList_.size() == 1);  // TODO: This shouldn't be a map right? 1 to 1.
    const auto& pipelineBindData = pipelineBindDataList_.getValue(PIPELINE_TYPE);

    // DESCRIPTOR
    assert(bindDataMap_.size() == 1 &&
           bindDataMap_.begin()->first.count(PASS_TYPE));  // TODO: This shouldn't be a map right? 1 to 1.
    const auto& descSetBindData = bindDataMap_.begin()->second;

    // PRE-DISPATCH
    preDispatch(cmd, pipelineBindData, descSetBindData, frameIndex);

    // PUSH CONSTANT
    Compute::PostProcess::PushConstant pushConstant = {
        ScreenSpace::PASS_FLAG::EDGE,
        // ScreenSpace::PASS_FLAG::BLUR_1,
        // ScreenSpace::PASS_FLAG::HDR_1,
    };
    vkCmdPushConstants(cmd, pipelineBindData->layout, pipelineBindData->pushConstantStages, 0,
                       static_cast<uint32_t>(sizeof(PushConstant)), &pushConstant);

    vkCmdBindPipeline(cmd, pipelineBindData->bindPoint, pipelineBindData->pipeline);

    vkCmdBindDescriptorSets(cmd, pipelineBindData->bindPoint, pipelineBindData->layout, descSetBindData.firstSet,
                            static_cast<uint32_t>(descSetBindData.descriptorSets[frameIndex].size()),
                            descSetBindData.descriptorSets[frameIndex].data(),
                            static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                            descSetBindData.dynamicOffsets.data());

    // DISPATCH
    vkCmdDispatch(cmd, groupCountX_, groupCountY_, groupCountZ_);

    barrierResource_.reset();
    auto& resource = barrierResource_;
    resource.imgBarriers.push_back({});
    resource.imgBarriers.back().sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    resource.imgBarriers.back().pNext = nullptr;
    resource.imgBarriers.back().srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    resource.imgBarriers.back().dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    resource.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    resource.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resource.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    resource.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    resource.imgBarriers.back().image = handler().passHandler().getCurrentFramebufferImage();
    resource.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    helpers::recordBarriers(barrierResource_, cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    // barrierResource_.reset();
    // helpers::attachementImageBarrierStorageWriteToColorRead(handler().passHandler().getCurrentFramebufferImage(),
    //                                                        barrierResource_);
    // helpers::recordBarriers(barrierResource_, cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

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

    //// helpers::recordBarriers(barrierResource_, cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    ////                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
    // helpers::recordBarriers(barrierResource_, cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

    // if (false) {
    //    // PUSH CONSTANT
    //    pushConstant = {
    //        // 0,
    //        ScreenSpace::PASS_FLAG::BLUR_2,
    //        // ScreenSpace::PASS_FLAG::HDR_2,
    //    };
    //    vkCmdPushConstants(cmd, pipelineBindData->layout, pipelineBindData->pushConstantStages, 0,
    //                       static_cast<uint32_t>(sizeof(Compute::PostProcess::PushConstant)), &pushConstant);

    //    // DISPATCH
    //    vkCmdDispatch(cmd, groupCountX_, groupCountY_, groupCountZ_);
    //    // vkCmdDispatch(cmd, 1, 1, 1);

    //    // SECOND BARRIER
    //    barrierResource_.reset();
    //    // TODO: fetch this on attachSwapchain
    //    helpers::storageImageBarrierWriteToRead(imageInfo.image, barrierResource_);
    //    // TODO: fetch this on attachSwapchain
    //    helpers::bufferBarrierWriteToRead(bufferInfo, barrierResource_);

    //    helpers::recordBarriers(barrierResource_, cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
    //                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
    //}

    // handler().commandHandler().endCmd(cmd);

    // SUBMIT INFO
    // submitResource.signalSemaphores[submitResource.signalSemaphoreCount] = semaphores_[syncIndex];
    // submitResource.signalSrcStageMasks[submitResource.signalSemaphoreCount++] = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    // submitResource.commandBufferCount = 1;
    // submitResource.commandBuffers[0] = cmd;
}

void PostProcess::Default::preDispatch(const VkCommandBuffer& cmd, const std::shared_ptr<Pipeline::BindData>& pPplnBindData,
                                       const Descriptor::Set::BindData& descSetBindData, const uint8_t frameIndex) {
    // barrierResource_.reset();
    //// To me this makes no sense but oh well. It works atm.
    // helpers::attachementImageBarrierWriteToStorageRead(handler().passHandler().getCurrentFramebufferImage(),
    //                                                   barrierResource_);
    // helpers::recordBarriers(barrierResource_, cmd, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    //                        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

    barrierResource_.reset();
    auto& resource = barrierResource_;
    resource.imgBarriers.push_back({});
    resource.imgBarriers.back().sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    resource.imgBarriers.back().pNext = nullptr;
    resource.imgBarriers.back().srcAccessMask = 0;
    resource.imgBarriers.back().dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    resource.imgBarriers.back().oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resource.imgBarriers.back().newLayout = VK_IMAGE_LAYOUT_GENERAL;
    resource.imgBarriers.back().srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    resource.imgBarriers.back().dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    resource.imgBarriers.back().image = handler().passHandler().getCurrentFramebufferImage();
    resource.imgBarriers.back().subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

    helpers::recordBarriers(barrierResource_, cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
}

void PostProcess::Default::postDispatch(const VkCommandBuffer& cmd, const std::shared_ptr<Pipeline::BindData>& pPplnBindData,
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

    helpers::recordBarriers(barrierResource_, cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
}

}  // namespace Compute
