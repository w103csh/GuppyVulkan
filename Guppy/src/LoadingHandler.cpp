/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "LoadingHandler.h"

#include "Constants.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"

Loading::Handler::Handler(Game* pGame) : Game::Handler(pGame){};

void Loading::Handler::init() {
    reset();
    tick();
}

// This used to be called cleanup but was executed during onTick, so I changed the name.
void Loading::Handler::tick() {
    // Check loading resources for cleanup
    if (!ldgResources_.empty()) {
        auto itRes = ldgResources_.begin();
        while (itRes != ldgResources_.end()) {
            auto& pRes = (*itRes);
            // Check if mesh loading resources can be cleaned up.
            if (destroyResource(*pRes)) {
                // Remove the resources from the list if all goes well.
                itRes = ldgResources_.erase(itRes);
            } else {
                ++itRes;
            }
        }
    }
}

// thread sync
std::unique_ptr<LoadingResource> Loading::Handler::createLoadingResources() const {
    auto pLdgRes = std::make_unique<LoadingResource>();

    // There should always be at least a graphics queue...
    vk::CommandBufferAllocateInfo allocInfo = {commandHandler().graphicsCmdPool(), vk::CommandBufferLevel::ePrimary, 1};
    pLdgRes->graphicsCmd = shell().context().dev.allocateCommandBuffers(allocInfo).front();
    // being recording
    commandHandler().beginCmd(pLdgRes->graphicsCmd);

    // There might not be a dedicated transfer queue...
    if (commandHandler().graphicsIndex() != commandHandler().transferIndex()) {
        allocInfo.commandPool = commandHandler().transferCmdPool();
        pLdgRes->transferCmd = shell().context().dev.allocateCommandBuffers(allocInfo).front();
        // being recording
        commandHandler().beginCmd(pLdgRes->transferCmd);
    }

    return pLdgRes;
}

void Loading::Handler::loadSubmit(std::unique_ptr<LoadingResource> pLdgRes) {
    const auto& ctx = shell().context();

    auto queueFamilyIndices = commandHandler().getUniqueQueueFamilies(true, false, true, false);
    pLdgRes->shouldWait = queueFamilyIndices.size() > 1;

    // End buffer recording
    pLdgRes->graphicsCmd.end();
    if (pLdgRes->shouldWait) pLdgRes->transferCmd.end();

    // Fences for cleanup ...
    for (int i = 0; i < queueFamilyIndices.size(); i++) {
        vk::FenceCreateInfo fenceInfo = {};
        pLdgRes->fences.push_back(ctx.dev.createFence(fenceInfo, ctx.pAllocator));
    }

    // Semaphore
    vk::SemaphoreCreateInfo semaphoreInfo = {};
    pLdgRes->semaphore = ctx.dev.createSemaphore(semaphoreInfo, ctx.pAllocator);

    // Wait stages
    vk::PipelineStageFlags wait_stages[] = {vk::PipelineStageFlagBits::eTransfer};

    // Staging submit info
    vk::SubmitInfo stagSubInfo = {};
    // Wait
    stagSubInfo.waitSemaphoreCount = 0;
    stagSubInfo.pWaitSemaphores = nullptr;
    stagSubInfo.pWaitDstStageMask = 0;
    // Command buffers
    stagSubInfo.commandBufferCount = 1;
    stagSubInfo.pCommandBuffers = &pLdgRes->transferCmd;
    // Signal
    stagSubInfo.signalSemaphoreCount = 1;
    stagSubInfo.pSignalSemaphores = &pLdgRes->semaphore;

    // Wait submit info
    vk::SubmitInfo waitSubInfo = {};
    if (pLdgRes->shouldWait) {
        // Wait
        waitSubInfo.waitSemaphoreCount = 1;
        waitSubInfo.pWaitSemaphores = &pLdgRes->semaphore;
        waitSubInfo.pWaitDstStageMask = wait_stages;
        // Command buffers
        waitSubInfo.commandBufferCount = 1;
        waitSubInfo.pCommandBuffers = &pLdgRes->graphicsCmd;
        // Signal
        waitSubInfo.signalSemaphoreCount = 0;
        waitSubInfo.pSignalSemaphores = nullptr;
    }

    // Sumbit ...
    if (pLdgRes->shouldWait) {
        commandHandler().transferQueue().submit({stagSubInfo}, pLdgRes->fences[0]);
    }
    commandHandler().graphicsQueue().submit({waitSubInfo}, pLdgRes->fences[1]);

    ldgResources_.push_back(std::move(pLdgRes));
}

void Loading::Handler::getFences(std::vector<vk::Fence>& fences) {
    for (const auto& res : ldgResources_) fences.insert(fences.end(), res->fences.begin(), res->fences.end());
}

bool Loading::Handler::destroyResource(LoadingResource& resource) const {
    const auto& ctx = shell().context();

    // Check fences for cleanup
    bool ready = true;
    for (auto& fence : resource.fences) ready &= ctx.dev.getFenceStatus(fence) == vk::Result::eSuccess;

    if (ready) {
        // Free stating resources
        for (auto& res : resource.stgResources) {
            ctx.dev.destroyBuffer(res.buffer, ctx.pAllocator);
            ctx.dev.freeMemory(res.memory, ctx.pAllocator);
        }
        resource.stgResources.clear();

        // Free fences
        for (auto& fence : resource.fences) ctx.dev.destroyFence(fence, ctx.pAllocator);
        resource.fences.clear();

        // Free semaphores
        ctx.dev.destroySemaphore(resource.semaphore, ctx.pAllocator);

        // Free command buffers
        ctx.dev.freeCommandBuffers(commandHandler().graphicsCmdPool(), 1, &resource.graphicsCmd);
        if (resource.shouldWait) {
            ctx.dev.freeCommandBuffers(commandHandler().transferCmdPool(), 1, &resource.transferCmd);
        }

        return true;
    }

    return false;
}
