
#include "CmdBufHandler.h"
#include "LoadingResourceHandler.h"

LoadingResourceHandler LoadingResourceHandler::inst_;
LoadingResourceHandler::LoadingResourceHandler() {}

void LoadingResourceHandler::init(const MyShell::Context& ctx) {
    inst_.cleanupResources();
    inst_.ctx_ = ctx;
}

std::unique_ptr<LoadingResources> LoadingResourceHandler::createLoadingResources() {
    auto pLdgRes = std::make_unique<LoadingResources>();

    // There should always be at least a graphics queue...
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.commandPool = CmdBufHandler::graphics_cmd_pool();
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;
    vk::assert_success(vkAllocateCommandBuffers(inst_.ctx_.dev, &alloc_info, &pLdgRes->graphicsCmd));
    // being recording
    CmdBufHandler::beginCmd(pLdgRes->graphicsCmd);

    // There might not be a dedicated transfer queue...
    if (CmdBufHandler::graphics_index() != CmdBufHandler::transfer_index()) {
        alloc_info.commandPool = CmdBufHandler::transfer_cmd_pool();
        vk::assert_success(vkAllocateCommandBuffers(inst_.ctx_.dev, &alloc_info, &pLdgRes->transferCmd));
        // being recording
        CmdBufHandler::beginCmd(pLdgRes->transferCmd);
    }

    return std::move(pLdgRes);
}

void LoadingResourceHandler::loadSubmit(std::unique_ptr<LoadingResources> pLdgRes) {
    auto queueFamilyIndices = CmdBufHandler::getUniqueQueueFamilies(true, false, true);
    pLdgRes->shouldWait = queueFamilyIndices.size() > 1;

    // End buffer recording
    CmdBufHandler::endCmd(pLdgRes->graphicsCmd);
    if (pLdgRes->shouldWait) CmdBufHandler::endCmd(pLdgRes->transferCmd);

    // Fences for cleanup ...
    for (int i = 0; i < queueFamilyIndices.size(); i++) {
        VkFence fence;
        VkFenceCreateInfo fence_info = {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        vk::assert_success(vkCreateFence(inst_.ctx_.dev, &fence_info, nullptr, &fence));
        pLdgRes->fences.push_back(fence);
    }

    // Semaphore
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vk::assert_success(vkCreateSemaphore(inst_.ctx_.dev, &semaphore_info, nullptr, &pLdgRes->semaphore));

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
        vk::assert_success(vkQueueSubmit(CmdBufHandler::transfer_queue(), 1, &stag_sub_info, pLdgRes->fences[0]));
    }
    vk::assert_success(vkQueueSubmit(CmdBufHandler::graphics_queue(), 1, &wait_sub_info, pLdgRes->fences[1]));

    inst_.ldgResources_.push_back(std::move(pLdgRes));
}

void LoadingResourceHandler::cleanupResources() {
    // Check loading resources for cleanup
    if (!inst_.ldgResources_.empty()) {
        auto itRes = inst_.ldgResources_.begin();
        while (itRes != inst_.ldgResources_.end()) {
            auto& pRes = (*itRes);
            // Check if mesh loading resources can be cleaned up.
            if (pRes->cleanup(inst_.ctx_.dev)) {
                // Remove the resources from the list if all goes well.
                itRes = inst_.ldgResources_.erase(itRes);
            } else {
                ++itRes;
            }
        }
    }
}

bool LoadingResources::cleanup(const VkDevice& dev) {
    // Check fences for cleanup
    bool ready = true;
    for (auto& fence : fences) ready &= vkGetFenceStatus(dev, fence) == VK_SUCCESS;

    if (ready) {
        // Free stating resources
        for (auto& res : stgResources) {
            vkDestroyBuffer(dev, res.buffer, nullptr);
            vkFreeMemory(dev, res.memory, nullptr);
        }
        stgResources.clear();

        // Free fences
        for (auto& fence : fences) vkDestroyFence(dev, fence, nullptr);
        fences.clear();

        // Free semaphores
        vkDestroySemaphore(dev, semaphore, nullptr);

        // Free command buffers
        vkFreeCommandBuffers(dev, CmdBufHandler::graphics_cmd_pool(), 1, &graphicsCmd);
        if (shouldWait) {
            vkFreeCommandBuffers(dev, CmdBufHandler::transfer_cmd_pool(), 1, &transferCmd);
        }

        return true;
    }

    return false;
}
