#ifndef CMDBUFRESOURCES_H
#define CMDBUFRESOURCES_H

#include <set>
#include <vector>
#include <vulkan/vulkan.h>

#include "MyShell.h"

class CmdBufResources {
   public:
    static void init(const MyShell::Context &ctx);
    static void destroy() { inst_.reset(); }

    CmdBufResources(const CmdBufResources &) = delete;             // Prevent construction by copying
    CmdBufResources &operator=(const CmdBufResources &) = delete;  // Prevent assignment

    enum class QUEUE { GRAPHICS = 0, PRESENT, TRANSFER };

    // indices
    static inline uint32_t graphics_index() { return inst_.graphics_queue_family_; }
    static inline uint32_t present_index() { return inst_.present_queue_family_; }
    static inline uint32_t transfer_index() { return inst_.transfer_queue_family_; }
    // memory properties
    static inline const VkPhysicalDeviceMemoryProperties &mem_props() { return inst_.mem_props_; }
    // queues
    static inline VkQueue &graphics_queue() { return inst_.queues_[inst_.graphics_queue_family_]; }
    static inline VkQueue &present_queue() { return inst_.queues_[inst_.present_queue_family_]; }
    static inline VkQueue &transfer_queue() { return inst_.queues_[inst_.transfer_queue_family_]; }
    // command pools
    static inline VkCommandPool &graphics_cmd_pool() { return inst_.cmd_pools_[inst_.graphics_queue_family_]; }
    static inline VkCommandPool &present_cmd_pool() { return inst_.cmd_pools_[inst_.present_queue_family_]; }
    static inline VkCommandPool &transfer_cmd_pool() { return inst_.cmd_pools_[inst_.transfer_queue_family_]; }
    // main commands
    static inline VkCommandBuffer &graphics_cmd() { return inst_.cmds_[inst_.graphics_queue_family_]; }
    static inline VkCommandBuffer &present_cmd() { return inst_.cmds_[inst_.present_queue_family_]; }
    static inline VkCommandBuffer &transfer_cmd() { return inst_.cmds_[inst_.transfer_queue_family_]; }
    // unique set
    static std::set<uint32_t> getUniqueQueueFamilies(bool graphics = true, bool present = true, bool transfer = true);

    static void beginCmd(const VkCommandBuffer &cmd);
    static void endCmd(const VkCommandBuffer &cmd);

   private:
    CmdBufResources();   // Prevent construction
    ~CmdBufResources();  // Prevent unwanted destruction

    static CmdBufResources inst_;

    void reset();

    VkDevice dev_;
    VkPhysicalDeviceMemoryProperties mem_props_;
    uint32_t graphics_queue_family_;
    uint32_t present_queue_family_;
    uint32_t transfer_queue_family_;
    std::vector<VkQueue> queues_;
    std::vector<VkCommandPool> cmd_pools_;
    std::vector<VkCommandBuffer> cmds_;
};

#endif  // !CMDBUFRESOURCES_H
