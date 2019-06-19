
#include "RenderPassHandler.h"

#include <algorithm>
#include <set>

#include "Constants.h"
#ifdef USE_DEBUG_UI
#include "RenderPassImGui.h"
#endif
#include "RenderPassDefault.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "PipelineHandler.h"
#include "RenderPassSampler.h"
#include "UIHandler.h"

RenderPass::Handler::Handler(Game* pGame)
    : Game::Handler(pGame), frameIndex_(0), swpchnRes_{}, submitResources_{}, pPasses_() {
    for (const auto& type : RenderPass::ALL) {
        switch (type) {
            case RENDER_PASS::DEFAULT:
                pPasses_.emplace_back(
                    std::make_unique<RenderPass::Default>(std::ref(*this), pPasses_.size(), &DEFAULT_CREATE_INFO));
                break;
            case RENDER_PASS::IMGUI:
#ifdef USE_DEBUG_UI
                pPasses_.emplace_back(std::make_unique<RenderPass::ImGui>(std::ref(*this), pPasses_.size()));
#endif
                break;
            case RENDER_PASS::SAMPLER:
                pPasses_.emplace_back(std::make_unique<RenderPass::Sampler>(std::ref(*this), pPasses_.size()));
                break;
            default:
                assert(false && "Unhandled pass type");
                throw std::runtime_error("Unhandled pass type");
        }
    }
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

std::set<RENDER_PASS> RenderPass::Handler::getActivePassTypes(const PIPELINE& pipelineType) {
    // This is obviously to slow if ever used for anything meaningful.
    std::set<RENDER_PASS> passTypes;
    for (const auto& pPass : pPasses_) {
        if (pPass->TYPE == RENDER_PASS::IMGUI) continue;
        if (pipelineType == PIPELINE::ALL_ENUM) {
            for (const auto& pplnType : pPass->getPipelineTypes())
                if (pplnType == pipelineType) passTypes.insert(pPass->TYPE);
        } else {
            passTypes.insert(pPass->TYPE);
        }
    }
    return passTypes;
}

void RenderPass::Handler::init() {
    for (auto& pPass : pPasses_) {
        // LIFECYCLE
        pPass->init();
        pPass->create();
        pPass->postCreate();
    }

    // PIPELINES
    createPipelines();

    // SYNC
    createFences();
    submitInfos_.assign(3, {VK_STRUCTURE_TYPE_SUBMIT_INFO, nullptr});
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
    fences_.resize(shell().context().imageCount);
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = flags;
    for (auto& fence : fences_) vk::assert_success(vkCreateFence(shell().context().dev, &fenceInfo, nullptr, &fence));
}

void RenderPass::Handler::updateBindData(const pipelinePassSet& set) {
    const auto& bindDataMap = pipelineHandler().getPipelineBindDataMap();
    for (const auto& pair : set) getPass(pair.second)->setBindData(pair.first, bindDataMap.at(pair));
}

void RenderPass::Handler::attachSwapchain() {
    const auto& ctx = shell().context();

    // SWAPCHAIN
    createSwapchainResources();

    for (auto& pPass : pPasses_) {
        // LIFECYCLE
        pPass->setSwapchainInfo();
        pPass->createTarget();
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
    // wait for the last submission since we reuse frame data
    vk::assert_success(vkWaitForFences(ctx.dev, 1, &fences_[frameIndex_], true, UINT64_MAX));
    vk::assert_success(vkResetFences(ctx.dev, 1, &fences_[frameIndex_]));
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
        assert(resource.waitSemaphores.size() == resource.waitDstStageMasks.size());

        submitInfos_[i].waitSemaphoreCount = resource.waitSemaphoreCount;
        submitInfos_[i].pWaitSemaphores = resource.waitSemaphores.data();
        submitInfos_[i].pWaitDstStageMask = resource.waitDstStageMasks.data();
        submitInfos_[i].commandBufferCount = resource.commandBufferCount;
        submitInfos_[i].pCommandBuffers = resource.commandBuffers.data();
        submitInfos_[i].signalSemaphoreCount = resource.signalSemaphoreCount;
        submitInfos_[i].pSignalSemaphores = resource.signalSemaphores.data();
    }

    vk::assert_success(
        vkQueueSubmit(commandHandler().graphicsQueue(), pPasses_.size(), submitInfos_.data(), fences_[frameIndex_]));
}

void RenderPass::Handler::update() { frameIndex_ = (frameIndex_ + 1) % static_cast<uint8_t>(shell().context().imageCount); }

void RenderPass::Handler::detachSwapchain() {
    reset();
    destroySwapchainResources();
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
        if (!pPass->SWAPCHAIN_DEPENDENT) pPass->destroyTargetResources();
        pPass->destroy();
    }
    // FENCE
    for (auto& fence : fences_) vkDestroyFence(shell().context().dev, fence, nullptr);
    fences_.clear();
}

void RenderPass::Handler::reset() {
    for (auto& pPass : pPasses_) {
        if (pPass->SWAPCHAIN_DEPENDENT) {
            pPass->destroyTargetResources();
        }
    }
}
