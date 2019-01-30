
#include "LoadingHandler.h"

#include "Shell.h"
#include "CommandHandler.h"

Loading::Handler::Handler(Game* pGame) : Game::Handler(pGame){};

void Loading::Handler::init() {
    reset();
    cleanup();
}

// thread sync
std::unique_ptr<Loading::Resources> Loading::Handler::createLoadingResources() const {
    auto pLdgRes = std::make_unique<Loading::Resources>();

    // There should always be at least a graphics queue...
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.commandPool = commandHandler().graphicsCmdPool();
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;
    vk::assert_success(vkAllocateCommandBuffers(shell().context().dev, &alloc_info, &pLdgRes->graphicsCmd));
    // being recording
    commandHandler().beginCmd(pLdgRes->graphicsCmd);

    // There might not be a dedicated transfer queue...
    if (commandHandler().graphicsIndex() != commandHandler().transferIndex()) {
        alloc_info.commandPool = commandHandler().transferCmdPool();
        vk::assert_success(vkAllocateCommandBuffers(shell().context().dev, &alloc_info, &pLdgRes->transferCmd));
        // being recording
        commandHandler().beginCmd(pLdgRes->transferCmd);
    }

    return std::move(pLdgRes);
}

void Loading::Handler::loadSubmit(std::unique_ptr<Loading::Resources> pLdgRes) {
    auto queueFamilyIndices = commandHandler().getUniqueQueueFamilies(true, false, true);
    pLdgRes->shouldWait = queueFamilyIndices.size() > 1;

    // End buffer recording
    commandHandler().endCmd(pLdgRes->graphicsCmd);
    if (pLdgRes->shouldWait) commandHandler().endCmd(pLdgRes->transferCmd);

    // Fences for cleanup ...
    for (int i = 0; i < queueFamilyIndices.size(); i++) {
        VkFence fence;
        VkFenceCreateInfo fence_info = {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vk::assert_success(vkCreateFence(shell().context().dev, &fence_info, nullptr, &fence));
        pLdgRes->fences.push_back(fence);
    }

    // Semaphore
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vk::assert_success(vkCreateSemaphore(shell().context().dev, &semaphore_info, nullptr, &pLdgRes->semaphore));

    // Wait stages
    VkPipelineStageFlags wait_stages[] = {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};

    // Staging submit info
    VkSubmitInfo stag_sub_info = {};
    stag_sub_info.pNext = nullptr;
    stag_sub_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // Wait
    stag_sub_info.waitSemaphoreCount = 0;
    stag_sub_info.pWaitSemaphores = nullptr;
    stag_sub_info.pWaitDstStageMask = 0;
    // Command buffers
    stag_sub_info.commandBufferCount = 1;
    stag_sub_info.pCommandBuffers = &pLdgRes->transferCmd;
    // Signal
    stag_sub_info.signalSemaphoreCount = 1;
    stag_sub_info.pSignalSemaphores = &pLdgRes->semaphore;

    VkSubmitInfo wait_sub_info = {};
    if (pLdgRes->shouldWait) {
        // Wait submit info
        wait_sub_info = {};
        wait_sub_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        wait_sub_info.pNext = nullptr;
        // Wait
        wait_sub_info.waitSemaphoreCount = 1;
        wait_sub_info.pWaitSemaphores = &pLdgRes->semaphore;
        wait_sub_info.pWaitDstStageMask = wait_stages;
        // Command buffers
        wait_sub_info.commandBufferCount = 1;
        wait_sub_info.pCommandBuffers = &pLdgRes->graphicsCmd;
        // Signal
        wait_sub_info.signalSemaphoreCount = 0;
        wait_sub_info.pSignalSemaphores = nullptr;
    }

    // Sumbit ...
    if (pLdgRes->shouldWait) {
        vk::assert_success(vkQueueSubmit(commandHandler().transferQueue(), 1, &stag_sub_info, pLdgRes->fences[0]));
    }
    vk::assert_success(vkQueueSubmit(commandHandler().graphicsQueue(), 1, &wait_sub_info, pLdgRes->fences[1]));

    ldgResources_.push_back(std::move(pLdgRes));
}

void Loading::Handler::cleanup() {
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

bool Loading::Handler::destroyResource(Loading::Resources& resource) const {
    // Check fences for cleanup
    bool ready = true;
    for (auto& fence : resource.fences) ready &= vkGetFenceStatus(shell().context().dev, fence) == VK_SUCCESS;

    if (ready) {
        // Free stating resources
        for (auto& res : resource.stgResources) {
            vkDestroyBuffer(shell().context().dev, res.buffer, nullptr);
            vkFreeMemory(shell().context().dev, res.memory, nullptr);
        }
        resource.stgResources.clear();

        // Free fences
        for (auto& fence : resource.fences) vkDestroyFence(shell().context().dev, fence, nullptr);
        resource.fences.clear();

        // Free semaphores
        vkDestroySemaphore(shell().context().dev, resource.semaphore, nullptr);

        // Free command buffers
        vkFreeCommandBuffers(shell().context().dev, commandHandler().graphicsCmdPool(), 1, &resource.graphicsCmd);
        if (resource.shouldWait) {
            vkFreeCommandBuffers(shell().context().dev, commandHandler().transferCmdPool(), 1, &resource.transferCmd);
        }

        return true;
    }

    return false;
}
