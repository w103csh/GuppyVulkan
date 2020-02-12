/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include <algorithm>

#include "CommandHandler.h"

#include "Helpers.h"
#include "Shell.h"

Command::Handler::Handler(Game* pGame) : Game::Handler(pGame) {}

void Command::Handler::init() {
    reset();

    auto uniqueQueueFamilies = getUniqueQueueFamilies();
    for (const auto& queueFamilyIndex : uniqueQueueFamilies) {
        // POOLS
        pools_[queueFamilyIndex] = vk::CommandPool{};
        createCmdPool(queueFamilyIndex, pools_.at(queueFamilyIndex));
        // BUFFERS
        cmds_[queueFamilyIndex] = vk::CommandBuffer{};
        createCmdBuffers(pools_.at(queueFamilyIndex), &cmds_.at(queueFamilyIndex), vk::CommandBufferLevel::ePrimary, 1);
        // TODO: Begin recording the default command buffers???
        beginCmd(cmds_[queueFamilyIndex]);
    };
}

void Command::Handler::reset() {
    // TODO: maybe wait for idle???
    auto uniqueQueueFamilies = getUniqueQueueFamilies();
    // owned command buffers
    for (const auto& queueFamilyIndex : uniqueQueueFamilies) {
        // owned command buffers
        if (cmds_.count(queueFamilyIndex))
            shell().context().dev.freeCommandBuffers(pools_.at(queueFamilyIndex), cmds_.at(queueFamilyIndex));
        // pools
        if (pools_.count(queueFamilyIndex))
            shell().context().dev.destroyCommandPool(pools_.at(queueFamilyIndex), ALLOC_PLACE_HOLDER);
    }
    cmds_.clear();
    pools_.clear();
}

void Command::Handler::createCmdPool(const uint32_t queueFamilyIndex, vk::CommandPool& cmdPool) {
    vk::CommandPoolCreateInfo cmdPoolInfo = {
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        queueFamilyIndex,
    };
    cmdPool = shell().context().dev.createCommandPool(cmdPoolInfo, ALLOC_PLACE_HOLDER);
}

void Command::Handler::createCmdBuffers(const vk::CommandPool& pool, vk::CommandBuffer* pCommandBuffers,
                                        vk::CommandBufferLevel level, uint32_t count) {
    vk::CommandBufferAllocateInfo allocInfo = {pool, level, count};
    helpers::checkVkResult(shell().context().dev.allocateCommandBuffers(&allocInfo, pCommandBuffers));
}

void Command::Handler::resetCmdBuffers() {
    for (auto& keyValue : cmds_) {
        keyValue.second.reset({});
        beginCmd(keyValue.second);
    }
}

const uint32_t& Command::Handler::getIndex(const QUEUE type) {
    // clang-format off
    switch (type) {
        case QUEUE::GRAPHICS: return shell().context().graphicsIndex;
        case QUEUE::PRESENT:  return shell().context().presentIndex;
        case QUEUE::TRANSFER: return shell().context().transferIndex;
        case QUEUE::COMPUTE:  return shell().context().computeIndex;
        default: assert(false); exit(EXIT_FAILURE);
    }
    // clang-format on
}

const vk::Queue& Command::Handler::getQueue(const QUEUE type) {
    // clang-format off
    switch (type) {
        case QUEUE::GRAPHICS: return shell().context().queues.at(shell().context().graphicsIndex);
        case QUEUE::PRESENT:  return shell().context().queues.at(shell().context().presentIndex);
        case QUEUE::TRANSFER: return shell().context().queues.at(shell().context().transferIndex);
        case QUEUE::COMPUTE:  return shell().context().queues.at(shell().context().computeIndex);
        default: assert(false); exit(EXIT_FAILURE);
    }
    // clang-format on
}

const vk::CommandPool& Command::Handler::getCmdPool(const QUEUE type) {
    // clang-format off
    switch (type) {
        case QUEUE::GRAPHICS: return pools_.at(shell().context().graphicsIndex);
        case QUEUE::PRESENT:  return pools_.at(shell().context().presentIndex);
        case QUEUE::TRANSFER: return pools_.at(shell().context().transferIndex);
        case QUEUE::COMPUTE:  return pools_.at(shell().context().computeIndex);
        default: assert(false); exit(EXIT_FAILURE);
    }
    // clang-format on
}

const vk::CommandBuffer& Command::Handler::getCmd(const QUEUE type) {
    // clang-format off
    switch (type) {
        case QUEUE::GRAPHICS: return cmds_.at(shell().context().graphicsIndex);
        case QUEUE::PRESENT: return cmds_.at(shell().context().presentIndex);
        case QUEUE::TRANSFER: return cmds_.at(shell().context().transferIndex);
        case QUEUE::COMPUTE: return cmds_.at(shell().context().computeIndex);
        default: assert(false); exit(EXIT_FAILURE);
    }
    // clang-format on
}

std::vector<uint32_t> Command::Handler::getUniqueQueueFamilies(bool graphics, bool present, bool transfer,
                                                               bool compute) const {
    std::vector<uint32_t> fams;
    std::vector<uint32_t>::iterator it;

    // graphics
    it = std::find(fams.begin(), fams.end(), graphicsIndex());
    if (graphics && it == fams.end()) fams.push_back(graphicsIndex());
    // present
    it = std::find(fams.begin(), fams.end(), presentIndex());
    if (present && it == fams.end()) fams.push_back(presentIndex());
    // transfer
    it = std::find(fams.begin(), fams.end(), transferIndex());
    if (transfer && it == fams.end()) fams.push_back(transferIndex());
    // compute
    it = std::find(fams.begin(), fams.end(), computeIndex());
    if (compute && it == fams.end()) fams.push_back(computeIndex());

    return fams;
}

void Command::Handler::beginCmd(const vk::CommandBuffer& cmd,
                                const vk::CommandBufferInheritanceInfo* inheritanceInfo) const {
    vk::CommandBufferBeginInfo beginInfo = {
        inheritanceInfo == nullptr
            ? vk::CommandBufferUsageFlagBits{}
            : vk::CommandBufferUsageFlagBits::eSimultaneousUse | vk::CommandBufferUsageFlagBits::eRenderPassContinue,
        inheritanceInfo,
    };
    cmd.begin(beginInfo);
}
