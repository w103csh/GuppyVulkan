
#include "RenderPassHandler.h"

#include <algorithm>
#include <set>

#include "ConstantsAll.h"
#ifdef USE_DEBUG_UI
#include "RenderPassImGui.h"
#endif
#include "ScreenSpace.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "PipelineHandler.h"
#include "UIHandler.h"

RenderPass::Handler::Handler(Game* pGame)
    : Game::Handler(pGame), frameIndex_(0), swpchnRes_{}, submitResources_{}, pPasses_() {
    for (const auto& type : RenderPass::ALL) {
        // clang-format off
        switch (type) {
            case RENDER_PASS::DEFAULT:              pPasses_.emplace_back(std::make_unique<RenderPass::Base>(std::ref(*this), pPasses_.size(), &RenderPass::DEFAULT_CREATE_INFO)); break;
            case RENDER_PASS::PROJECT:              pPasses_.emplace_back(std::make_unique<RenderPass::Base>(std::ref(*this), pPasses_.size(), &RenderPass::PROJECT_CREATE_INFO)); break;
            case RENDER_PASS::SAMPLER_DEFAULT:      pPasses_.emplace_back(std::make_unique<RenderPass::Base>(std::ref(*this), pPasses_.size(), &RenderPass::SAMPLER_DEFAULT_CREATE_INFO)); break;
            case RENDER_PASS::SCREEN_SPACE:         pPasses_.emplace_back(std::make_unique<RenderPass::Base>(std::ref(*this), pPasses_.size(), &RenderPass::ScreenSpace::CREATE_INFO)); break;
            //case RENDER_PASS::SCREEN_SPACE_SAMPLER: pPasses_.emplace_back(std::make_unique<RenderPass::Base>(std::ref(*this), pPasses_.size(), &RenderPass::ScreenSpace::SAMPLER_CREATE_INFO)); break;
            case RENDER_PASS::IMGUI:
#ifdef USE_DEBUG_UI
                pPasses_.emplace_back(std::make_unique<RenderPass::ImGui>(std::ref(*this), pPasses_.size()));
#endif
                break;
            default: assert(false && "Unhandled pass type"); exit(EXIT_FAILURE);
        }
    }
    // clang-format on
    assert(pPasses_.size() <= RESOURCE_SIZE);
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

std::set<RENDER_PASS> RenderPass::Handler::getActivePassTypes(const PIPELINE& pipelineTypeIn) {
    // This is obviously to slow if ever used for anything meaningful.
    std::set<RENDER_PASS> passTypes;
    for (const auto& pPass : pPasses_) {
        if (pPass->TYPE == RENDER_PASS::IMGUI) continue;
        if (pipelineTypeIn == PIPELINE::ALL_ENUM) {
            for (const auto& pipelineType : pPass->getPipelineTypes())
                if (pipelineType == pipelineTypeIn) passTypes.insert(pPass->TYPE);
        } else {
            passTypes.insert(pPass->TYPE);
        }
    }
    return passTypes;
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

    // PIPELINES
    createPipelines();

    // SYNC
    createFences();
    submitInfos_.assign(pPasses_.size(), {VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr});
}

void RenderPass::Handler::createPipelines() {
    /* Create pipeline to pass offset map. This way pipeline cache stuff
     *  can potentially be used to speed up creation or caching???? I am
     *  not going to worry about it other than this atm though.
     */
    pipelinePassSet set;
    for (const auto& pPass : pPasses_) {
        for (const auto keyValue : pPass->getPipelineBindDataMap()) {
            set.insert({keyValue.first, pPass->TYPE});
        }
    }
    pipelineHandler().createPipelines(set);
    updateBindData(set);
}

void RenderPass::Handler::createFences(VkFenceCreateFlags flags) {
    frameFences_.resize(shell().context().imageCount);
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = flags;
    for (auto& fence : frameFences_) vk::assert_success(vkCreateFence(shell().context().dev, &fenceInfo, nullptr, &fence));
}

void RenderPass::Handler::updateBindData(const pipelinePassSet& set) {
    const auto& bindDataMap = pipelineHandler().getPipelineBindDataMap();
    for (const auto& pair : set) getPass(pair.second)->setBindData(pair.first, bindDataMap.at(pair));
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

void RenderPass::Handler::createSwapchainResources() {
    const auto& ctx = shell().context();

    uint32_t imageCount;
    vk::assert_success(vkGetSwapchainImagesKHR(ctx.dev, ctx.swapchain, &imageCount, nullptr));

    // Get the image resources from the swapchain...
    swpchnRes_.images.resize(imageCount);
    swpchnRes_.views.resize(imageCount);
    vk::assert_success(vkGetSwapchainImagesKHR(ctx.dev, ctx.swapchain, &imageCount, swpchnRes_.images.data()));

    for (uint32_t i = 0; i < imageCount; i++) {
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

    // wait for the last submission since we reuse frame data
    vk::assert_success(vkWaitForFences(ctx.dev, static_cast<uint32_t>(fences_.size()), fences_.data(), true, UINT64_MAX));

    vk::assert_success(vkResetFences(ctx.dev, 1, &frameFences_[frameIndex_]));
}

void RenderPass::Handler::recordPasses() {
    const auto& ctx = shell().context();
    auto frameIndex = getFrameIndex();

    for (uint8_t i = 0; i < pPasses_.size(); i++) {
        auto& resource = submitResources_[i];
        resource.waitSemaphoreCount = resource.commandBufferCount = resource.signalSemaphoreCount = 0;

        // Add a pass resources
        if (i == 0) {
            // wait for back buffer...
            resource.waitSemaphores[resource.waitSemaphoreCount] = ctx.acquiredBackBuffer.acquireSemaphore;
            // back buffer flags...
            resource.waitDstStageMasks[resource.waitSemaphoreCount] = ctx.waitDstStageMask;
            resource.waitSemaphoreCount++;
        } else if (submitResources_[i - 1].signalSemaphoreCount) {
            // Take the signals from the previous pass
            std::memcpy(                                                            //
                &resource.waitSemaphores[resource.waitSemaphoreCount],              //
                submitResources_[i - 1].signalSemaphores.data(),                    //
                sizeof(VkSemaphore) * submitResources_[i - 1].signalSemaphoreCount  //
            );
            std::memcpy(                                                                     //
                &resource.waitDstStageMasks[resource.waitSemaphoreCount],                    //
                submitResources_[i - 1].signalSrcStageMasks.data(),                          //
                sizeof(VkPipelineStageFlags) * submitResources_[i - 1].signalSemaphoreCount  //
            );
            resource.waitSemaphoreCount += submitResources_[i - 1].signalSemaphoreCount;
        }

        // Record the pass and update the resources
        pPasses_[i]->record(frameIndex);
        pPasses_[i]->updateSubmitResource(submitResources_[i], frameIndex);

        // Always add the render semaphore to the last pass
        if ((static_cast<size_t>(i) + 1) == pPasses_.size()) {
            // signal back buffer...
            resource.signalSemaphores[resource.signalSemaphoreCount] = ctx.acquiredBackBuffer.renderSemaphore;
            resource.signalSemaphoreCount++;
        }
    }

    submit();
}

void RenderPass::Handler::submit() {
    for (uint8_t i = 0; i < pPasses_.size(); i++) {
        const auto& resource = submitResources_[i];
        // TODO: find a way to validate that there are as many set waitDstStageMask flags
        // set as waitSemaphores.
        submitInfos_[i].waitSemaphoreCount = resource.waitSemaphoreCount;
        submitInfos_[i].pWaitSemaphores = resource.waitSemaphores.data();
        submitInfos_[i].pWaitDstStageMask = resource.waitDstStageMasks.data();
        submitInfos_[i].commandBufferCount = resource.commandBufferCount;
        submitInfos_[i].pCommandBuffers = resource.commandBuffers.data();
        submitInfos_[i].signalSemaphoreCount = resource.signalSemaphoreCount;
        submitInfos_[i].pSignalSemaphores = resource.signalSemaphores.data();
    }

    vk::assert_success(
        vkQueueSubmit(commandHandler().graphicsQueue(), pPasses_.size(), submitInfos_.data(), frameFences_[frameIndex_]));
}

void RenderPass::Handler::update() { frameIndex_ = (frameIndex_ + 1) % static_cast<uint8_t>(shell().context().imageCount); }

void RenderPass::Handler::detachSwapchain() {
    reset();
    destroySwapchainResources();
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