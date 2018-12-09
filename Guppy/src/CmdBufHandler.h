#ifndef CMD_BUF_HANDLER_H
#define CMD_BUF_HANDLER_H

#include <vector>
#include <vulkan/vulkan.h>

#include "Shell.h"
#include "Singleton.h"

class CmdBufHandler : Singleton {
   public:
    static void init(const Shell::Context &ctx);
    static void destroy() { inst_.reset(); }

    enum class QUEUE { GRAPHICS = 0, PRESENT, TRANSFER };

    static void createCmdPool(const uint32_t queueFamilyIndex, VkCommandPool &cmdPool);
    static void createCmdBuffers(const VkCommandPool &cmdPool, VkCommandBuffer *pCommandBuffers,
                                VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY, uint32_t count = 1);
    static void resetCmdBuffers();

    // indices
    static inline uint32_t graphics_index() { return inst_.ctx_.graphics_index; }
    static inline uint32_t present_index() { return inst_.ctx_.present_index; }
    static inline uint32_t transfer_index() { return inst_.ctx_.transfer_index; }
    // memory properties
    static inline const VkPhysicalDeviceMemoryProperties &mem_props() { return inst_.ctx_.mem_props; }
    // queues
    static inline const VkQueue &graphics_queue() { return inst_.ctx_.queues[inst_.ctx_.graphics_index]; }
    static inline const VkQueue &present_queue() { return inst_.ctx_.queues[inst_.ctx_.present_index]; }
    static inline const VkQueue &transfer_queue() { return inst_.ctx_.queues[inst_.ctx_.transfer_index]; }
    // command pools
    static inline const VkCommandPool &graphics_cmd_pool() { return inst_.cmd_pools_[inst_.ctx_.graphics_index]; }
    static inline const VkCommandPool &present_cmd_pool() { return inst_.cmd_pools_[inst_.ctx_.present_index]; }
    static inline const VkCommandPool &transfer_cmd_pool() { return inst_.cmd_pools_[inst_.ctx_.transfer_index]; }
    // main commands
    static inline const VkCommandBuffer &graphics_cmd() { return inst_.cmds_[inst_.ctx_.graphics_index]; }
    static inline const VkCommandBuffer &present_cmd() { return inst_.cmds_[inst_.ctx_.present_index]; }
    static inline const VkCommandBuffer &transfer_cmd() { return inst_.cmds_[inst_.ctx_.transfer_index]; }
    // unique set
    static std::vector<uint32_t> getUniqueQueueFamilies(bool graphics = true, bool present = true, bool transfer = true);

    static void beginCmd(const VkCommandBuffer &cmd, const VkCommandBufferInheritanceInfo *inheritanceInfo = nullptr);
    static void endCmd(const VkCommandBuffer &cmd);

   private:
    CmdBufHandler(){};   // Prevent construction
    ~CmdBufHandler(){};  // Prevent destruction
    static CmdBufHandler inst_;
    void reset() override;

    Shell::Context ctx_;  // TODO: shared_ptr

    std::vector<VkCommandPool> cmd_pools_;
    std::vector<VkCommandBuffer> cmds_;
};

#endif  // !CMD_BUF_HANDLER_H
