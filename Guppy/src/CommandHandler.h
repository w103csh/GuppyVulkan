#ifndef CMD_BUF_HANDLER_H
#define CMD_BUF_HANDLER_H

#include <map>
#include <vulkan/vulkan.h>

#include "Game.h"
#include "Shell.h"

namespace Command {

class Handler : public Game::Handler {
   public:
    Handler(Game *pGame);

    void init() override;

    void createCmdPool(const uint32_t queueFamilyIndex, VkCommandPool &cmdPool);
    inline void createCmdBuffers(const QUEUE type, VkCommandBuffer *pCommandBuffers,
                                 VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, uint32_t count = 1) {
        createCmdBuffers(getCmdPool(type), pCommandBuffers, level, count);
    }
    void resetCmdBuffers();

    // clang-format off
    // INDEX
    constexpr uint32_t Command::Handler::graphicsIndex() const { return shell().context().graphicsIndex; }
    constexpr uint32_t Command::Handler::presentIndex() const { return shell().context().presentIndex; }
    constexpr uint32_t Command::Handler::transferIndex() const { return shell().context().transferIndex; }
    constexpr uint32_t Command::Handler::computeIndex() const { return shell().context().computeIndex; }
    const uint32_t &getIndex(const QUEUE type);
    // QUEUE
    const VkQueue &graphicsQueue() const { return shell().context().queues.at(graphicsIndex()); }
    const VkQueue &presentQueue() const  { return shell().context().queues.at(presentIndex()); }
    const VkQueue &transferQueue() const { return shell().context().queues.at(transferIndex()); }
    const VkQueue &computeQueue() const  { return shell().context().queues.at(computeIndex()); }
    const VkQueue &getQueue(const QUEUE type);
    // POOLS
    const VkCommandPool &graphicsCmdPool() const { return pools_.at(graphicsIndex()); }
    const VkCommandPool &presentCmdPool() const  { return pools_.at(presentIndex()); }
    const VkCommandPool &transferCmdPool() const { return pools_.at(transferIndex()); }
    const VkCommandPool &computeCmdPool() const  { return pools_.at(computeIndex()); }
    const VkCommandPool &getCmdPool(const QUEUE type);
    // COMMANDS
    const VkCommandBuffer &graphicsCmd() const { return cmds_.at(graphicsIndex()); }
    const VkCommandBuffer &presentCmd() const  { return cmds_.at(presentIndex()); }
    const VkCommandBuffer &transferCmd() const { return cmds_.at(transferIndex()); }
    const VkCommandBuffer &computeCmd() const  { return cmds_.at(computeIndex()); }
    const VkCommandBuffer &getCmd(const QUEUE type);
    // clang-format on

    std::vector<uint32_t> getUniqueQueueFamilies(bool graphics = true, bool present = true, bool transfer = true,
                                                 bool compute = true) const;

    void beginCmd(const VkCommandBuffer &cmd, const VkCommandBufferInheritanceInfo *inheritanceInfo = nullptr) const;
    void endCmd(const VkCommandBuffer &cmd) const;

   private:
    void reset() override;

    void createCmdBuffers(const VkCommandPool &pool, VkCommandBuffer *pCommandBuffers, VkCommandBufferLevel level,
                          uint32_t count);

    std::map<uint32_t, VkCommandPool> pools_;
    std::map<uint32_t, VkCommandBuffer> cmds_;
};

}  // namespace Command

#endif  // !CMD_BUF_HANDLER_H
