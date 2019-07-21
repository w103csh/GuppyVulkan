
#include "RenderPassHandler.h"

#include <algorithm>

#ifdef USE_DEBUG_UI
#include "RenderPassImGui.h"
#endif
#include "RenderPassScreenSpace.h"
#include "ScreenSpace.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "ComputeHandler.h"
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "PipelineHandler.h"
#include "TextureHandler.h"

Descriptor::Set::RenderPass::SwapchainImage::SwapchainImage(Descriptor::Handler& handler)
    : Set::Base{
          handler,
          DESCRIPTOR_SET::SWAPCHAIN_IMAGE,
          "_DS_SWAPCHAIN_IMAGE",
          {
              {{0, 0}, {STORAGE_IMAGE::SWAPCHAIN, ::RenderPass::SWAPCHAIN_TARGET_ID}},
          },
      } {}

RenderPass::Handler::Handler(Game* pGame) : Game::Handler(pGame), frameIndex_(0), swpchnRes_{}, submitResources_{} {
    for (const auto& type : RenderPass::ACTIVE) {  // TODO: Should this be ALL?
        // clang-format off
        switch (type) {
            case PASS::DEFAULT:              pPasses_.emplace_back(std::make_unique<RenderPass::Base>(             std::ref(*this), static_cast<uint32_t>(pPasses_.size()), &RenderPass::DEFAULT_CREATE_INFO)); break;
            case PASS::SAMPLER_PROJECT:      pPasses_.emplace_back(std::make_unique<RenderPass::Base>(             std::ref(*this), static_cast<uint32_t>(pPasses_.size()), &RenderPass::PROJECT_CREATE_INFO)); break;
            case PASS::SAMPLER_DEFAULT:      pPasses_.emplace_back(std::make_unique<RenderPass::Base>(             std::ref(*this), static_cast<uint32_t>(pPasses_.size()), &RenderPass::SAMPLER_DEFAULT_CREATE_INFO)); break;
            case PASS::SCREEN_SPACE:         pPasses_.emplace_back(std::make_unique<RenderPass::ScreenSpace::Base>(std::ref(*this), static_cast<uint32_t>(pPasses_.size()), &RenderPass::ScreenSpace::CREATE_INFO)); break;
            case PASS::SAMPLER_SCREEN_SPACE: pPasses_.emplace_back(std::make_unique<RenderPass::ScreenSpace::Base>(std::ref(*this), static_cast<uint32_t>(pPasses_.size()), &RenderPass::ScreenSpace::SAMPLER_CREATE_INFO)); break;
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
    createCmds();
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

void RenderPass::Handler::createCmds() {
    cmdList_.resize(10);
    for (auto& cmds : cmdList_) {
        cmds.resize(1);
        commandHandler().createCmdBuffers(commandHandler().graphicsCmdPool(), cmds.data(), VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                          cmds.size());
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
    vk::assert_success(vkGetSwapchainImagesKHR(ctx.dev, ctx.swapchain, &imageCount, swpchnRes_.images.data()));

    bool wasTexDestroyed = false;
    swpchnRes_.views.resize(imageCount);
    for (uint32_t i = 0; i < ctx.imageCount; i++) {
        helpers::createImageView(ctx.dev, swpchnRes_.images[i], 1, ctx.surfaceFormat.format, VK_IMAGE_ASPECT_COLOR_BIT,
                                 VK_IMAGE_VIEW_TYPE_2D, 1, swpchnRes_.views[i]);

        // TEXTURE
        auto pTexture = textureHandler().getTexture(SWAPCHAIN_TARGET_ID, i);
        assert(pTexture && pTexture->status != STATUS::READY);
        // Check if the swapchain was destroy before being recreated.
        wasTexDestroyed = pTexture->status == STATUS::DESTROYED;
        // Update swapchain texture descriptor info.
        auto& sampler = pTexture->samplers[0];
        sampler.image = swpchnRes_.images[i];
        sampler.view = swpchnRes_.views[i];
        Texture::Handler::createDescInfo(pTexture->DESCRIPTOR_TYPE, sampler);

        pTexture->status = STATUS::READY;
    }
    if (wasTexDestroyed) descriptorHandler().updateBindData({(std::string(SWAPCHAIN_TARGET_ID))});

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

    // auto& cmd = cmdList_[frameIndex][0];
    // vkResetCommandBuffer(cmd, 0);
    // VkCommandBufferBeginInfo bufferInfo = {};
    // bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //// bufferInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    // bufferInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    // vk::assert_success(vkBeginCommandBuffer(cmd, &bufferInfo));

    uint8_t passIndex = 0;
    for (; passIndex < pPasses_.size(); passIndex++) {
        SubmitResource* pResource = &submitResources_[passIndex];
        pResource->resetCount();

        if (passIndex == 0) {
            // wait for back buffer...
            pResource->waitSemaphores[pResource->waitSemaphoreCount] = ctx.acquiredBackBuffer.acquireSemaphore;
            // back buffer flags...
            pResource->waitDstStageMasks[pResource->waitSemaphoreCount] = ctx.waitDstStageMask;
            pResource->waitSemaphoreCount++;
        } else if (submitResources_[passIndex - 1].signalSemaphoreCount) {
            // Take the signals from the previous pass
            std::memcpy(                                                                    //
                &pResource->waitSemaphores[pResource->waitSemaphoreCount],                  //
                submitResources_[passIndex - 1].signalSemaphores.data(),                    //
                sizeof(VkSemaphore) * submitResources_[passIndex - 1].signalSemaphoreCount  //
            );
            std::memcpy(                                                                             //
                &pResource->waitDstStageMasks[pResource->waitSemaphoreCount],                        //
                submitResources_[passIndex - 1].signalSrcStageMasks.data(),                          //
                sizeof(VkPipelineStageFlags) * submitResources_[passIndex - 1].signalSemaphoreCount  //
            );
            pResource->waitSemaphoreCount += submitResources_[passIndex - 1].signalSemaphoreCount;
        }

        // Record the pass and update the resources
        pPasses_[passIndex]->record(frameIndex);
        pPasses_[passIndex]->updateSubmitResource(submitResources_[passIndex], frameIndex);

        // Check if a compute pass is needed.
        // computeHandler().recordPasses(pPasses_[passIndex]->TYPE, *pResource);

        // Always add the render semaphore to the last pass
        if ((static_cast<size_t>(passIndex) + 1) == pPasses_.size()) {
            // signal back buffer...
            pResource->signalSemaphores[pResource->signalSemaphoreCount] = ctx.acquiredBackBuffer.renderSemaphore;
            pResource->signalSemaphoreCount++;
        }
    }

    submit(passIndex);
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
        for (uint32_t j = 0; j < pResource->commandBufferCount; j++)  //
            vk::assert_success(vkEndCommandBuffer(pResource->commandBuffers[j]));
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
    for (uint32_t i = 0; i < swpchnRes_.views.size(); i++) {
        vkDestroyImageView(shell().context().dev, swpchnRes_.views[i], nullptr);
        // Update swapchain texture status.
        auto pTexture = textureHandler().getTexture(SWAPCHAIN_TARGET_ID, i);
        assert(pTexture);
        pTexture->status = STATUS::DESTROYED;
    }
    swpchnRes_.views.clear();
    // Images are destroyed by vkDestroySwapchainKHR
    swpchnRes_.images.clear();
}

void RenderPass::Handler::destroy() {
    // PASS
    for (auto& pPass : pPasses_) {
        if (!pPass->usesSwapchain()) pPass->destroyTargetResources();
        pPass->destroy();
    }
    // COMMAND
    for (auto& cmds : cmdList_) {
        helpers::destroyCommandBuffers(shell().context().dev, commandHandler().graphicsCmdPool(), cmds);
        cmds.clear();
    }
    cmdList_.clear();
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
