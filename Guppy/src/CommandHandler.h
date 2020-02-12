/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef CMD_BUF_HANDLER_H
#define CMD_BUF_HANDLER_H

#include <map>
#include <vulkan/vulkan.hpp>

#include "Game.h"
#include "Shell.h"

namespace Command {

class Handler : public Game::Handler {
   public:
    Handler(Game *pGame);

    void init() override;

    void createCmdPool(const uint32_t queueFamilyIndex, vk::CommandPool &cmdPool);
    inline void createCmdBuffers(const QUEUE type, vk::CommandBuffer *pCommandBuffers,
                                 vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary, uint32_t count = 1) {
        createCmdBuffers(getCmdPool(type), pCommandBuffers, level, count);
    }
    void resetCmdBuffers();

    // clang-format off
    // INDEX
    constexpr uint32_t graphicsIndex() const { return shell().context().graphicsIndex; }
    constexpr uint32_t presentIndex() const {  return shell().context().presentIndex; }
    constexpr uint32_t transferIndex() const { return shell().context().transferIndex; }
    constexpr uint32_t computeIndex() const {  return shell().context().computeIndex; }
    const uint32_t &getIndex(const QUEUE type);
    // QUEUE
    const vk::Queue &graphicsQueue() const { return shell().context().queues.at(graphicsIndex()); }
    const vk::Queue &presentQueue() const  { return shell().context().queues.at(presentIndex()); }
    const vk::Queue &transferQueue() const { return shell().context().queues.at(transferIndex()); }
    const vk::Queue &computeQueue() const  { return shell().context().queues.at(computeIndex()); }
    const vk::Queue &getQueue(const QUEUE type);
    // POOLS
    const vk::CommandPool &graphicsCmdPool() const { return pools_.at(graphicsIndex()); }
    const vk::CommandPool &presentCmdPool() const  { return pools_.at(presentIndex()); }
    const vk::CommandPool &transferCmdPool() const { return pools_.at(transferIndex()); }
    const vk::CommandPool &computeCmdPool() const  { return pools_.at(computeIndex()); }
    const vk::CommandPool &getCmdPool(const QUEUE type);
    // COMMANDS
    const vk::CommandBuffer &graphicsCmd() const { return cmds_.at(graphicsIndex()); }
    const vk::CommandBuffer &presentCmd() const  { return cmds_.at(presentIndex()); }
    const vk::CommandBuffer &transferCmd() const { return cmds_.at(transferIndex()); }
    const vk::CommandBuffer &computeCmd() const  { return cmds_.at(computeIndex()); }
    const vk::CommandBuffer &getCmd(const QUEUE type);
    // clang-format on

    std::vector<uint32_t> getUniqueQueueFamilies(bool graphics = true, bool present = true, bool transfer = true,
                                                 bool compute = true) const;

    void beginCmd(const vk::CommandBuffer &cmd, const vk::CommandBufferInheritanceInfo *inheritanceInfo = nullptr) const;

   private:
    void reset() override;

    void createCmdBuffers(const vk::CommandPool &pool, vk::CommandBuffer *pCommandBuffers, vk::CommandBufferLevel level,
                          uint32_t count);

    std::map<uint32_t, vk::CommandPool> pools_;
    std::map<uint32_t, vk::CommandBuffer> cmds_;
};

}  // namespace Command

#endif  // !CMD_BUF_HANDLER_H
