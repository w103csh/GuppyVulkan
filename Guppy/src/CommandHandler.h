#ifndef CMD_BUF_HANDLER_H
#define CMD_BUF_HANDLER_H

#include <map>
#include <vulkan/vulkan.h>

#include "Game.h"

namespace Command {

class Handler : public Game::Handler {
   public:
    Handler(Game *pGame);

    void init() override;

    enum class QUEUE { GRAPHICS = 0, PRESENT, TRANSFER };

    void createCmdPool(const uint32_t queueFamilyIndex, VkCommandPool &cmdPool);
    void createCmdBuffers(const VkCommandPool &cmdPool, VkCommandBuffer *pCommandBuffers,
                          VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, uint32_t count = 1);
    void resetCmdBuffers();

    // INDEX
    uint32_t graphicsIndex() const;
    uint32_t presentIndex() const;
    uint32_t transferIndex() const;
    // QUEUE
    const VkQueue &graphicsQueue() const;
    const VkQueue &presentQueue() const;
    const VkQueue &transferQueue() const;
    // POOLS
    const VkCommandPool &graphicsCmdPool() const;
    const VkCommandPool &presentCmdPool() const;
    const VkCommandPool &transferCmdPool() const;
    // COMMANDS
    const VkCommandBuffer &graphicsCmd() const;
    const VkCommandBuffer &presentCmd() const;
    const VkCommandBuffer &transferCmd() const;

    std::vector<uint32_t> getUniqueQueueFamilies(bool graphics = true, bool present = true, bool transfer = true) const;

    void beginCmd(const VkCommandBuffer &cmd, const VkCommandBufferInheritanceInfo *inheritanceInfo = nullptr) const;
    void endCmd(const VkCommandBuffer &cmd) const;

   private:
    void reset() override;

    std::map<uint32_t, VkCommandPool> pools_;
    std::map<uint32_t, VkCommandBuffer> cmds_;
};

}  // namespace Command

#endif  // !CMD_BUF_HANDLER_H
