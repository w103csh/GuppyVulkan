
#include "ComputeHandler.h"

#include <algorithm>

#include "ConstantsAll.h"
#include "ScreenSpace.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "DescriptorHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"

namespace {

template <typename T>
void getTypeFromMap(const T& map, const PIPELINE& pipelineTypeIn, std::set<PASS>& types) {
    for (const auto& [type, pCompute] : map) {
        if (pipelineTypeIn == PIPELINE::ALL_ENUM || pCompute->PIPELINE_TYPE == pipelineTypeIn) {
            types.insert(pCompute->PASS_TYPE);
        }
    }
}

}  // namespace

// clang-format off

const std::set<PASS> Compute::ALL = {
    PASS::COMPUTE_POST_PROCESS,
};

const std::vector<PASS> Compute::ACTIVE = {
    PASS::COMPUTE_POST_PROCESS,
};

// clang-format on

Compute::Handler::Handler(Game* pGame) : Game::Handler(pGame) {
    std::unique_ptr<Compute::Base> pCompute = nullptr;
    for (const auto& type : Compute::ACTIVE) {  // TODO: Should this be ALL?
        // clang-format off
        switch (type) {
            case PASS::COMPUTE_POST_PROCESS: pCompute = std::unique_ptr<Compute::Base>(new Compute::PostProcess::Default(std::ref(*this))); break;
            default: assert(false && "Unhandled pass type"); exit(EXIT_FAILURE);
        }
        // clang-format on

        assert(pCompute != nullptr);
        assert(pCompute->PASS_TYPE == type);

        if (pCompute->getStatus() == STATUS::READY) {
            assert(pComputeMap_.count(pCompute->PASS_TYPE) == 0);
            pComputeMap_[pCompute->PASS_TYPE] = std::move(pCompute);
        } else {
            assert(pComputePendingMap_.count(pCompute->PASS_TYPE) == 0);
            pComputePendingMap_[pCompute->PASS_TYPE] = std::move(pCompute);
        }
    }

    assert(pComputePendingMap_.size() + pComputeMap_.size() == Compute::ACTIVE.size());  // TODO: Should this be ALL?
}

void Compute::Handler::init() {
    // reset();
    for (auto& [passType, pCompute] : pComputePendingMap_) pCompute->init();
    update();
}

void Compute::Handler::update() {
    for (auto it = pComputePendingMap_.begin(); it != pComputePendingMap_.end();) {
        it->second->prepare();
        if (it->second->getStatus() == STATUS::READY) {
            descriptorHandler().getBindData(it->second->PIPELINE_TYPE, it->second->bindDataMap_, nullptr, nullptr);
            pComputeMap_.insert(pComputeMap_.end(), std::move(*it));
            it = pComputePendingMap_.erase(it);
        } else {
            ++it;
        }
    }
}

void Compute::Handler::getActivePassTypes(std::set<PASS>& types, const PIPELINE& pipelineTypeIn) {
    // This is obviously too slow if ever used for anything meaningful.
    getTypeFromMap(pComputePendingMap_, pipelineTypeIn, types);
    getTypeFromMap(pComputeMap_, pipelineTypeIn, types);
}

void Compute::Handler::attachSwapchain() {
    for (auto& [key, pCompute] : pComputePendingMap_) pCompute->attachSwapchain();
    for (auto& [key, pCompute] : pComputeMap_) pCompute->attachSwapchain();
    // FENCE
    assert(passFences_.empty());
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    passFences_.resize(shell().context().imageCount);
    for (auto& fence : passFences_) vk::assert_success(vkCreateFence(shell().context().dev, &fenceInfo, nullptr, &fence));
}

void Compute::Handler::detachSwapchain() {
    for (auto& [key, pCompute] : pComputePendingMap_) pCompute->detachSwapchain();
    for (auto& [key, pCompute] : pComputeMap_) pCompute->detachSwapchain();
    // FENCE
    for (auto& fence : passFences_) {
        vkWaitForFences(shell().context().dev, 1, &fence, VK_TRUE, UINT64_MAX);
        vkDestroyFence(shell().context().dev, fence, nullptr);
    }
    passFences_.clear();
}

void Compute::Handler::addPipelinePassPairs(pipelinePassSet& set) {
    for (const auto& [passType, pCompute] : pComputeMap_) {
        set.insert({pCompute->PIPELINE_TYPE, pCompute->PASS_TYPE});
    }
    for (const auto& [passType, pCompute] : pComputePendingMap_) {
        set.insert({pCompute->PIPELINE_TYPE, pCompute->PASS_TYPE});
    }
}

void Compute::Handler::updateBindData(const pipelinePassSet& set) {
    const auto& bindDataMap = pipelineHandler().getPipelineBindDataMap();
    // Update pipeline bind data for all compute resources.
    for (const auto& pair : set) {
        if (Compute::ALL.count(pair.second)) {
            bool found = false;
            auto it = pComputeMap_.find(pair.second);
            if (it != pComputeMap_.end()) {
                it->second->setBindData(pair.first, bindDataMap.at(pair));
                continue;
            }
            it = pComputePendingMap_.find(pair.second);
            if (it != pComputePendingMap_.end()) {
                it->second->setBindData(pair.first, bindDataMap.at(pair));
                continue;
            }
            assert(false && "Pass type for compute resource not found");
        }
    }
}

bool Compute::Handler::recordPasses(const PASS& renderPassType, RenderPass::SubmitResource& submitResource) {
    for (auto& [key, pCompute] : pComputeMap_) {
        // if (pCompute->POST_PASS_TYPES.find(renderPassType) != pCompute->POST_PASS_TYPES.end()) {
        //    // Still need some kind of indication that this compute has been submitted.
        //    pCompute->record(passHandler().getFrameIndex(), submitResource);
        //    return true;
        //}
        if (pCompute->POST_PASS_TYPE == renderPassType) {
            pCompute->record(passHandler().getFrameIndex(), submitResource);
            return true;
        }
    }
    return false;
}

void Compute::Handler::submitResources(std::vector<const SubmitResource*>& pResources) {
    submitInfos_.clear();

    for (const auto& pResource : pResources) {
        submitInfos_.push_back({VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr});
        // TODO: find a way to validate that there are as many set waitDstStageMask flags
        // set as waitSemaphores.
        submitInfos_.back().waitSemaphoreCount = pResource->waitSemaphoreCount;
        submitInfos_.back().pWaitSemaphores = pResource->waitSemaphores.data();
        submitInfos_.back().pWaitDstStageMask = pResource->waitDstStageMasks.data();
        submitInfos_.back().commandBufferCount = pResource->commandBufferCount;
        submitInfos_.back().pCommandBuffers = pResource->commandBuffers.data();
        submitInfos_.back().signalSemaphoreCount = pResource->signalSemaphoreCount;
        submitInfos_.back().pSignalSemaphores = pResource->signalSemaphores.data();
    }
    pResources.clear();

    assert(false);  // This did not work when it was commited. I left this so I wouldn't have to make it again.
    // VkFence fence = pQueuedResources_.empty() && false ? passFences_[0] : VK_NULL_HANDLE;
    VkFence fence = passFences_[0];
    vk::assert_success(vkQueueSubmit(commandHandler().graphicsQueue(), submitInfos_.size(), submitInfos_.data(), fence));
}

void Compute::Handler::reset() {
    const auto& dev = shell().context().dev;
    for (auto& [passType, pCompute] : pComputeMap_) pCompute->destroy();
    pComputeMap_.clear();
    for (auto& [passType, pCompute] : pComputePendingMap_) pCompute->destroy();
    pComputePendingMap_.clear();
}
