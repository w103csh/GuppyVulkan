
#include "RenderPassHandler.h"

#include <algorithm>

#ifdef USE_DEBUG_UI
#include "RenderPassImGui.h"
#endif
#include "ScreenSpace.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "ComputeHandler.h"
#include "LoadingHandler.h"
#include "PipelineHandler.h"

RenderPass::Handler::Handler(Game* pGame) : Game::Handler(pGame), frameIndex_(0), swpchnRes_{}, submitResources_{} {
    for (const auto& type : RenderPass::ACTIVE) {  // TODO: Should this be ALL?
        // clang-format off
        switch (type) {
            case PASS::DEFAULT:              pPasses_.emplace_back(std::make_unique<RenderPass::Base>(std::ref(*this), static_cast<uint32_t>(pPasses_.size()), &RenderPass::DEFAULT_CREATE_INFO)); break;
            case PASS::SAMPLER_PROJECT:      pPasses_.emplace_back(std::make_unique<RenderPass::Base>(std::ref(*this), static_cast<uint32_t>(pPasses_.size()), &RenderPass::PROJECT_CREATE_INFO)); break;
            case PASS::SAMPLER_DEFAULT:      pPasses_.emplace_back(std::make_unique<RenderPass::Base>(std::ref(*this), static_cast<uint32_t>(pPasses_.size()), &RenderPass::SAMPLER_DEFAULT_CREATE_INFO)); break;
            case PASS::SCREEN_SPACE:         pPasses_.emplace_back(std::make_unique<RenderPass::Base>(std::ref(*this), static_cast<uint32_t>(pPasses_.size()), &RenderPass::ScreenSpace::CREATE_INFO)); break;
            case PASS::SAMPLER_SCREEN_SPACE: pPasses_.emplace_back(std::make_unique<RenderPass::Base>(std::ref(*this), static_cast<uint32_t>(pPasses_.size()), &RenderPass::ScreenSpace::SAMPLER_CREATE_INFO)); break;
            case PASS::IMGUI:
#ifdef USE_DEBUG_UI
                                            pPasses_.emplace_back(std::make_unique<RenderPass::ImGui>(std::ref(*this), static_cast<uint32_t>(pPasses_.size())));
#endif
                break;
            default: assert(false && "Unhandled pass type"); exit(EXIT_FAILURE);
        }
    }
    // clang-format on
    assert(pPasses_.size() == RenderPass::ACTIVE.size());  // TODO: Should this be ALL?
    assert(pPasses_.size() <= RESOURCE_SIZE);
}

void RenderPass::Handler::init() {
    // At the moment it appears easiest to init iterating in one direction, and create
    // iterating in the the other. This shouldn't ever be an issue???

    bool isFinal = true;
    for (auto it = pPasses_.rbegin(); it != pPasses_.rend(); ++it) {
        (*it)->init(isFinal);
        // After the final swapchain dependent pass is found make sure any other swapchain
        // dependent passes know they are not the last one.
        if (isFinal) isFinal = !(*it)->hasTargetSwapchain();
    }
    assert(isFinal == false);

    // LIFECYCLE
    for (auto& pPass : pPasses_) {
        pPass->create();
        pPass->postCreate();
        if (!pPass->usesSwapchain()) pPass->createTarget();
    }

    // The texture clear flags are only useful in the previous loop, so
    // clear then straight away.
    targetClearFlags_.clear();

    // SYNC
    createFences();
    // TODO: should this just be an array too???? Ugh
    submitInfos_.assign(RESOURCE_SIZE, {VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr});
}

Uniform::offsetsMap RenderPass::Handler::makeUniformOffsetsMap() {
    // TODO: validation? This is actually merging things.
    Uniform::offsetsMap map;
    for (const auto& pPass : pPasses_) {
        for (const auto& keyValue : pPass->getDescriptorPipelineOffsets()) {
            auto searchKey = map.find(keyValue.first);
            if (searchKey != map.end()) {
                auto searchValue = searchKey->second.find(keyValue.second);
                if (searchValue != searchKey->second.end()) {
                    searchValue->second.insert(pPass->TYPE);
                } else {
                    searchKey->second[keyValue.second] = {pPass->TYPE};
                }
            } else {
                map[keyValue.first] = {{keyValue.second, {pPass->TYPE}}};
            }
        }
    }
    return map;
}

void RenderPass::Handler::getActivePassTypes(std::set<PASS>& types, const PIPELINE& pipelineTypeIn) {
    // This is obviously to slow if ever used for anything meaningful.
    for (const auto& pPass : pPasses_) {
        if (pPass->TYPE == PASS::IMGUI) continue;
        if (pipelineTypeIn == PIPELINE::ALL_ENUM) {
            for (const auto& pipelineType : pPass->getPipelineTypes())
                if (pipelineType == pipelineTypeIn) types.insert(pPass->TYPE);
        } else {
            types.insert(pPass->TYPE);
        }
    }
}

void RenderPass::Handler::addPipelinePassPairs(pipelinePassSet& set) {
    /* Create pipeline to pass offset map. This way pipeline cache stuff
     *  can potentially be used to speed up creation or caching???? I am
     *  not going to worry about it other than this atm though.
     */
    for (const auto& pPass : pPasses_) {
        for (const auto keyValue : pPass->getPipelineBindDataMap()) {
            set.insert({keyValue.first, pPass->TYPE});
        }
    }
}

void RenderPass::Handler::updateBindData(const pipelinePassSet& set) {
    const auto& bindDataMap = pipelineHandler().getPipelineBindDataMap();
    // Update pipeline bind data for all render passes.
    for (const auto& pair : set) {
        if (RenderPass::ALL.count(pair.second))  //
            getPass(pair.second)->setBindData(pair.first, bindDataMap.at(pair));
    }
}

void RenderPass::Handler::createFences(VkFenceCreateFlags flags) {
    frameFences_.resize(shell().context().imageCount);
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = flags;
    for (auto& fence : frameFences_) vk::assert_success(vkCreateFence(shell().context().dev, &fenceInfo, nullptr, &fence));
}

void RenderPass::Handler::attachSwapchain() {
    const auto& ctx = shell().context();

    // SWAPCHAIN
    createSwapchainResources();

    // LIFECYCLE
    for (auto& pPass : pPasses_) {
        if (pPass->usesSwapchain()) pPass->createTarget();
    }
}

void RenderPass::Handler::detachSwapchain() {
    reset();
    destroySwapchainResources();
}

void RenderPass::Handler::createSwapchainResources() {
    const auto& ctx = shell().context();

    uint32_t imageCount = ctx.imageCount;
    vk::assert_success(vkGetSwapchainImagesKHR(ctx.dev, ctx.swapchain, &imageCount, nullptr));

    // Get the image resources from the swapchain...
    swpchnRes_.images.resize(imageCount);
    swpchnRes_.views.resize(imageCount);
    vk::assert_success(vkGetSwapchainImagesKHR(ctx.dev, ctx.swapchain, &imageCount, swpchnRes_.images.data()));

    for (uint32_t i = 0; i < ctx.imageCount; i++) {
        helpers::createImageView(ctx.dev, swpchnRes_.images[i], 1, ctx.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT,
                                 VK_IMAGE_VIEW_TYPE_2D, 1, swpchnRes_.views[i]);
    }

    assert(shell().context().imageCount == swpchnRes_.images.size());
    assert(swpchnRes_.images.size() == swpchnRes_.views.size());
}

void RenderPass::Handler::acquireBackBuffer() {
    const auto& ctx = shell().context();
    fences_.clear();

    // TODO: Use the semaphores. This just waits for everything.
    loadingHandler().getFences(fences_);
    fences_.push_back(frameFences_[frameIndex_]);

    // wait for the last submission since we reuse frame data.
    vk::assert_success(vkWaitForFences(ctx.dev, static_cast<uint32_t>(fences_.size()), fences_.data(), VK_TRUE, UINT64_MAX));
    vk::assert_success(vkResetFences(ctx.dev, 1, &frameFences_[frameIndex_]));
}

void RenderPass::Handler::recordPasses() {
    const auto& ctx = shell().context();
    auto frameIndex = getFrameIndex();

    uint8_t submitIndex = 0;
    for (uint8_t passIndex = submitIndex; passIndex < pPasses_.size(); passIndex++, submitIndex++) {
        SubmitResource* pResource = &submitResources_[submitIndex];
        pResource->resetCount();

        if (submitIndex == 0) {
            // wait for back buffer...
            pResource->waitSemaphores[pResource->waitSemaphoreCount] = ctx.acquiredBackBuffer.acquireSemaphore;
            // back buffer flags...
            pResource->waitDstStageMasks[pResource->waitSemaphoreCount] = ctx.waitDstStageMask;
            pResource->waitSemaphoreCount++;
        }

        // Check if a compute pass is needed.
        if (computeHandler().recordPasses(pPasses_[passIndex]->TYPE, *pResource)) {
            assert(submitIndex != 0);
            pResource = &submitResources_[++submitIndex];
            pResource->resetCount();
        }

        // Record the pass and update the resources
        pPasses_[passIndex]->record(frameIndex);
        pPasses_[passIndex]->updateSubmitResource(submitResources_[submitIndex], frameIndex);

        // Always add the render semaphore to the last pass
        if ((static_cast<size_t>(passIndex) + 1) == pPasses_.size()) {
            // signal back buffer...
            pResource->signalSemaphores[pResource->signalSemaphoreCount] = ctx.acquiredBackBuffer.renderSemaphore;
            pResource->signalSemaphoreCount++;
        }
    }

    submit(submitIndex);
}

void RenderPass::Handler::submit(const uint8_t submitCount) {
    const SubmitResource* pResource;
    VkSubmitInfo* pInfo;
    for (uint8_t i = 0; i < submitCount; i++) {
        pResource = &submitResources_[i];
        pInfo = &submitInfos_[i];
        pInfo->waitSemaphoreCount = pResource->waitSemaphoreCount;
        pInfo->pWaitSemaphores = pResource->waitSemaphores.data();
        pInfo->pWaitDstStageMask = pResource->waitDstStageMasks.data();
        pInfo->commandBufferCount = pResource->commandBufferCount;
        pInfo->pCommandBuffers = pResource->commandBuffers.data();
        pInfo->signalSemaphoreCount = pResource->signalSemaphoreCount;
        pInfo->pSignalSemaphores = pResource->signalSemaphores.data();
    }

    vk::assert_success(
        vkQueueSubmit(commandHandler().graphicsQueue(), submitCount, &submitInfos_[0], frameFences_[frameIndex_]));
}

void RenderPass::Handler::update() {
    // At one point something was here
}

void RenderPass::Handler::updateFrameIndex() {
    frameIndex_ = (frameIndex_ + 1) % static_cast<uint8_t>(shell().context().imageCount);
}

bool RenderPass::Handler::shouldClearTarget(const std::string& targetId) {
    if (targetClearFlags_.count(targetId)) return false;
    targetClearFlags_.insert(targetId);
    return true;
}

void RenderPass::Handler::destroySwapchainResources() {
    for (auto& view : swpchnRes_.views) vkDestroyImageView(shell().context().dev, view, nullptr);
    swpchnRes_.views.clear();
    // Images are destroyed by vkDestroySwapchainKHR
    swpchnRes_.images.clear();
}

void RenderPass::Handler::destroy() {
    // PASSES
    for (auto& pPass : pPasses_) {
        if (!pPass->usesSwapchain()) pPass->destroyTargetResources();
        pPass->destroy();
    }
    // FENCE
    for (auto& fence : frameFences_) vkDestroyFence(shell().context().dev, fence, nullptr);
    frameFences_.clear();
}

void RenderPass::Handler::reset() {
    for (auto& pPass : pPasses_) {
        if (pPass->usesSwapchain()) {
            pPass->destroyTargetResources();
        }
    }
}
