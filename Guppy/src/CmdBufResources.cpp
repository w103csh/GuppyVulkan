
#include "CmdBufResources.h"

CmdBufResources::CmdBufResources() {}
CmdBufResources::~CmdBufResources() {}

void CmdBufResources::reset() {
    // TODO: maybe wait for idle???
    // owned command buffers
    for (size_t i = 0; i < cmd_pools_.size(); i++) vkFreeCommandBuffers(dev_, cmd_pools_[i], 1, &cmds_[i]);
    cmd_pools_.clear();
    // pools
    for (auto& cmd_pool : cmd_pools_) vkDestroyCommandPool(dev_, cmd_pool, nullptr);
    cmd_pools_.clear();
}

void CmdBufResources::init(const MyShell::Context& ctx) {
    // Clean up owned memory...
    inst_.reset();

    // transfer things from context... this is probably dangerous.
    inst_.dev_ = ctx.dev;
    inst_.graphics_queue_family_ = ctx.graphics_index;
    inst_.present_queue_family_ = ctx.present_index;
    inst_.transfer_queue_family_ = ctx.transfer_index;
    inst_.queues_ = ctx.queues;
    inst_.mem_props_ = ctx.physical_dev_props[ctx.physical_dev_index].memory_properties;

    auto uniq = inst_.getUniqueQueueFamilies();

    inst_.cmd_pools_.resize(uniq.size());
    inst_.cmds_.resize(uniq.size());

    for (const auto& index : uniq) {
        // POOLS
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.queueFamilyIndex = index;
        cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        vk::assert_success(vkCreateCommandPool(inst_.dev_, &cmd_pool_info, nullptr, &inst_.cmd_pools_[index]));

        // BUFFERS
        VkCommandBufferAllocateInfo cmd = {};
        cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd.pNext = nullptr;
        cmd.commandPool = inst_.cmd_pools_[index];
        cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd.commandBufferCount = 1;

        vk::assert_success(vkAllocateCommandBuffers(inst_.dev_, &cmd, &inst_.cmds_[index]));

        // TODO: Begin recording the default command buffers???
        CmdBufResources::beginCmd(inst_.cmds_[index]);
    };
}

std::set<uint32_t> CmdBufResources::getUniqueQueueFamilies(bool graphics, bool present, bool transfer) {
    std::set<uint32_t> fams;
    if (graphics) fams.insert(inst_.graphics_queue_family_);
    if (present) fams.insert(inst_.present_queue_family_);
    if (transfer) fams.insert(inst_.transfer_queue_family_);
    return fams;
}

void CmdBufResources::beginCmd(const VkCommandBuffer& cmd) {
    VkCommandBufferBeginInfo cmd_info = {};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_info.pNext = nullptr;
    cmd_info.flags = 0;
    cmd_info.pInheritanceInfo = nullptr;
    vk::assert_success(vkBeginCommandBuffer(cmd, &cmd_info));
}

void CmdBufResources::endCmd(const VkCommandBuffer& cmd) { vk::assert_success(vkEndCommandBuffer(cmd)); }
