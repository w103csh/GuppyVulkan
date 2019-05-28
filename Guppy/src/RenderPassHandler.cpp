
#include "RenderPassHandler.h"

#include "Constants.h"
#ifdef USE_DEBUG_UI
#include "RenderPassImGui.h"
#endif
#include "RenderPassDefault.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "RenderPassSampler.h"
#include "UIHandler.h"

RenderPass::Handler::Handler(Game* pGame)  //
    : Game::Handler(pGame), defaultOffset_(BAD_OFFSET), uiOffset_(BAD_OFFSET), frameIndex_(0) {
    // DEFAULT
    pPasses_.emplace_back(std::make_unique<RenderPass::Default>(std::ref(*this)));
    defaultOffset_ = pPasses_.size() - 1;

    // UI
#ifdef USE_DEBUG_UI
    pPasses_.emplace_back(std::make_unique<RenderPass::ImGui>(std::ref(*this)));
#else
    pPasses_.emplace_back(nullptr);
#endif
    uiOffset_ = pPasses_.size() - 1;

    // GENERAL
    // pPasses_.emplace_back(std::make_shared<RenderPass::Sampler>(std::ref(*this)));

    assert(defaultOffset_ < pPasses_.size() && uiOffset_ < pPasses_.size());
}

void RenderPass::Handler::init() {
    const auto& ctx = shell().context();

    for (auto& pPass : pPasses_) {
        if (pPass != nullptr) {
            // LIFECYCLE
            pPass->init();
            pPass->create();
            pPass->postCreate();
        }
    }
}

void RenderPass::Handler::attachSwapchain() {
    const auto& ctx = shell().context();

    // SWAPCHAIN
    createSwapchainResources();

    for (auto& pPass : pPasses_) {
        if (pPass != nullptr) {
            // LIFECYCLE
            pPass->setSwapchainInfo();
            pPass->createTarget();
        }
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
}

void RenderPass::Handler::acquireBackBuffer() {
    const auto& ctx = shell().context();

    auto& fence = getMainPass()->data.fences[frameIndex_];
    // wait for the last submission since we reuse frame data
    vk::assert_success(vkWaitForFences(ctx.dev, 1, &fence, true, UINT64_MAX));
    vk::assert_success(vkResetFences(ctx.dev, 1, &fence));

    const Shell::BackBuffer& back = ctx.acquiredBackBuffer;
}

void RenderPass::Handler::recordPasses() {
    const auto& ctx = shell().context();
    const Shell::BackBuffer& back = ctx.acquiredBackBuffer;

    std::vector<SubmitResource> resources;
    SubmitResource resource;

    // MAIN
    getMainPass()->record();
    resource = {};
    resource.waitSemaphores.push_back(back.acquireSemaphore);    // wait for back buffer...
    resource.waitDstStageMasks.push_back(ctx.waitDstStageMask);  // back buffer flags...
#ifndef USE_DEBUG_UI
    resource.signalSemaphores.push_back(back.renderSemaphore);  // signal back buffer...
#endif
    getMainPass()->getSubmitResource(frameIndex_, resource);
    resources.push_back(resource);

    // UI
#ifdef USE_DEBUG_UI
    getUIPass()->record();
    resource = {};
    resource.waitSemaphores.push_back(
        getMainPass()->data.semaphores[frameIndex_]);           // wait on scene... (must be earlier in submission scope)
    resource.signalSemaphores.push_back(back.renderSemaphore);  // signal back buffer...
    getUIPass()->getSubmitResource(frameIndex_, resource);
    resources.push_back(resource);
#endif

    submit(resources);
}

void RenderPass::Handler::submit(const std::vector<SubmitResource>& resources) {
    std::vector<VkSubmitInfo> infos;
    infos.reserve(resources.size());

    for (const auto& resource : resources) {
        assert(resource.waitSemaphores.size() == resource.waitDstStageMasks.size());

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(resource.waitSemaphores.size());
        submitInfo.pWaitSemaphores = resource.waitSemaphores.data();
        submitInfo.pWaitDstStageMask = resource.waitDstStageMasks.data();
        submitInfo.commandBufferCount = static_cast<uint32_t>(resource.commandBuffers.size());
        submitInfo.pCommandBuffers = resource.commandBuffers.data();
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(resource.signalSemaphores.size());
        submitInfo.pSignalSemaphores = resource.signalSemaphores.data();
        infos.push_back(submitInfo);
    }

    vk::assert_success(vkQueueSubmit(commandHandler().graphicsQueue(), static_cast<uint32_t>(infos.size()), infos.data(),
                                     getMainPass()->data.fences[frameIndex_]));
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
    for (auto& pPass : pPasses_) {
        if (pPass != nullptr) {
            pPass->destroy();
        }
    }
    pPasses_.clear();
}

void RenderPass::Handler::reset() {
    for (auto& pPass : pPasses_) {
        if (pPass != nullptr) {
            pPass->destroyTargetResources();
        }
    }
}
