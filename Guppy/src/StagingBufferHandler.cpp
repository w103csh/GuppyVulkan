
#include <thread>

#include "StagingBufferHandler.h"

StagingBufferHandler::StagingBufferHandler(const MyShell* sh, const VkDevice& dev, const CommandData* cmd_data)
    : sh_(sh), dev_(dev), cmd_data_(cmd_data), mutex_() {
    assert(sh != nullptr && dev != nullptr && cmd_data != nullptr);
}

// TODO: This should create / reset the wait command that gets submitted in end_recording_and_submit.
void StagingBufferHandler::begin_command_recording(VkCommandBuffer& cmd, BEGIN_TYPE type, VkCommandBufferResetFlags resetflags) {
    switch (type) {
        case BEGIN_TYPE::ALLOC: {
            VkCommandBufferAllocateInfo cmd_info = {};
            cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmd_info.pNext = NULL;
            cmd_info.commandPool = cmd_data_->cmd_pools[cmd_data_->transfer_queue_family];
            cmd_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmd_info.commandBufferCount = 1;

            vk::assert_success(vkAllocateCommandBuffers(dev_, &cmd_info, &cmd));
        } break;
        case BEGIN_TYPE::RESET: {
            vkResetCommandBuffer(cmd, resetflags);
        } break;
    }
    helpers::execute_begin_command_buffer(cmd);
}

// TODO: This needs work. begin_command_recording should create / reset the wait command that gets submitted here too.
void StagingBufferHandler::end_recording_and_submit(StagingBufferResource* resources, size_t num_resources, VkCommandBuffer& cmd,
                                                    uint32_t wait_queue_family, VkCommandBuffer* wait_cmd,
                                                    VkPipelineStageFlags* wait_stages, END_TYPE type) {
    if (wait_queue_family == cmd_data_->transfer_queue_family) {
        sh_->log(MyShell::LOG_ERR, "This command doesn't need to be staged like this! Maybe?");
    }
    bool should_wait = wait_queue_family != UINT32_MAX && wait_cmd != nullptr;

    CommandResources cmd_res_ref = {};  // new CommandResources();
    auto cmd_res = &cmd_res_ref;
    cmd_res->num_resources = num_resources;
    cmd_res->resources = resources;

    // FENCES FOR CLEANUP ...
    VkFenceCreateInfo fence_info;
    for (int i = 0; i < NUM_STAG_BUFF_FENCES; i++) {
        fence_info = {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if (!should_wait && i > 0) {
            // start second fence in signaled state because we are only submitting one queue
            fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        }
        vk::assert_success(vkCreateFence(dev_, &fence_info, nullptr, &cmd_res->fences[i]));
    }

    // SEMAPHORE
    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    vk::assert_success(vkCreateSemaphore(dev_, &semaphore_info, nullptr, &cmd_res->semaphore));

    // STAGING SUBMIT INFO
    VkSubmitInfo stag_sub_info = {};
    stag_sub_info.pNext = nullptr;
    stag_sub_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // WAIT
    stag_sub_info.waitSemaphoreCount = 0;
    stag_sub_info.pWaitSemaphores = nullptr;
    stag_sub_info.pWaitDstStageMask = 0;
    // COMMAND BUFFERS
    stag_sub_info.commandBufferCount = 1;
    stag_sub_info.pCommandBuffers = &cmd;
    // SIGNAL
    stag_sub_info.signalSemaphoreCount = 1;
    stag_sub_info.pSignalSemaphores = &cmd_res->semaphore;

    // move this outside?
    // for (uint32_t i = 0; i < stag_cmd_cnt; i++) execute_end_command_buffer(stag_cmds[i]);
    helpers::execute_end_command_buffer(cmd);

    VkSubmitInfo wait_sub_info = {};
    if (should_wait) {
        // WAITING SUBMIT INFO
        wait_sub_info = {};
        wait_sub_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        wait_sub_info.pNext = nullptr;
        // WAIT
        wait_sub_info.waitSemaphoreCount = 1;
        wait_sub_info.pWaitSemaphores = &cmd_res->semaphore;
        wait_sub_info.pWaitDstStageMask = wait_stages;
        // COMMAND BUFFERS
        wait_sub_info.commandBufferCount = 1;
        wait_sub_info.pCommandBuffers = wait_cmd;
        // SIGNAL
        wait_sub_info.signalSemaphoreCount = 0;
        wait_sub_info.pSignalSemaphores = nullptr;

        // move this outside?
        // for (uint32_t i = 0; i < wait_cmd_cnt; i++) execute_end_command_buffer(wait_cmds[i]);
        helpers::execute_end_command_buffer(*wait_cmd);
    }

    // SUBMIT ...
    vk::assert_success(vkQueueSubmit(cmd_data_->queues[cmd_data_->transfer_queue_family], 1, &stag_sub_info, cmd_res->fences[0]));
    if (wait_queue_family != UINT32_MAX) {
        vk::assert_success(vkQueueSubmit(cmd_data_->queues[wait_queue_family], 1, &wait_sub_info, cmd_res->fences[1]));
    }

    // DETACH THE CLEANUP
    // std::thread(&StagingBufferHandler::wait, this, cmd_res, type).detach();
    wait(cmd_res, END_TYPE::RESET);
}

void StagingBufferHandler::wait(CommandResources* cmd_res, END_TYPE type) {
    // WAIT FOR FENCES
    vk::assert_success(vkWaitForFences(dev_, NUM_STAG_BUFF_FENCES, cmd_res->fences.data(), VK_TRUE, UINT64_MAX));

    // FREE BUFFER
    for (size_t i = 0; i < cmd_res->num_resources; i++) {
        StagingBufferResource res = cmd_res->resources[i];
        vkDestroyBuffer(dev_, res.buffer, nullptr);
        vkFreeMemory(dev_, res.memory, nullptr);
    }
    // delete cmd_res->resources;

    // FREE FENCES
    for (auto& fence : cmd_res->fences) vkDestroyFence(dev_, fence, nullptr);

    // FREE SEMAPHORE
    vkDestroySemaphore(dev_, cmd_res->semaphore, nullptr);

    //// FREE COMMAND BUFFERS
    switch (type) {
        case END_TYPE::DESTROY: {
            vkFreeCommandBuffers(dev_, cmd_data_->cmd_pools[cmd_data_->transfer_queue_family], 1, &cmd_res->cmds[0]);
            if (cmd_res->wait_queue_family != UINT32_MAX) {
                vkFreeCommandBuffers(dev_, cmd_data_->cmd_pools[cmd_res->wait_queue_family], 1, &cmd_res->cmds[1]);
            }
        } break;
        case END_TYPE::RESET: {
            vkResetCommandBuffer(cmd_data_->cmds[cmd_data_->transfer_queue_family], 0);
            helpers::execute_begin_command_buffer(cmd_data_->cmds[cmd_data_->transfer_queue_family]);

            vkResetCommandBuffer(cmd_data_->cmds[cmd_res->wait_queue_family], 0);
            helpers::execute_begin_command_buffer(cmd_data_->cmds[cmd_res->wait_queue_family]);
        } break;
    }

    // delete cmd_res;
}