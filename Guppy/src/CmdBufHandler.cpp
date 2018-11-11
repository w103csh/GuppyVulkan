
#include <algorithm>

#include "CmdBufHandler.h"

CmdBufHandler CmdBufHandler::inst_;

void CmdBufHandler::reset() {
    // TODO: maybe wait for idle???
    // owned command buffers
    for (size_t i = 0; i < cmd_pools_.size(); i++) vkFreeCommandBuffers(ctx_.dev, cmd_pools_[i], 1, &cmds_[i]);
    cmds_.clear();
    // pools
    for (auto& cmd_pool : cmd_pools_) vkDestroyCommandPool(ctx_.dev, cmd_pool, nullptr);
    cmd_pools_.clear();
}

void CmdBufHandler::init(const MyShell::Context& ctx) {
    // Clean up owned memory...
    inst_.reset();

    inst_.ctx_ = ctx;

    auto uniq = inst_.getUniqueQueueFamilies();

    inst_.cmd_pools_.resize(uniq.size());
    inst_.cmds_.resize(uniq.size());

    for (const auto& index : uniq) {
        // POOLS
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.queueFamilyIndex = index;
        cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        vk::assert_success(vkCreateCommandPool(inst_.ctx_.dev, &cmd_pool_info, nullptr, &inst_.cmd_pools_[index]));

        // BUFFERS
        VkCommandBufferAllocateInfo cmd = {};
        cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd.pNext = nullptr;
        cmd.commandPool = inst_.cmd_pools_[index];
        cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd.commandBufferCount = 1;

        vk::assert_success(vkAllocateCommandBuffers(inst_.ctx_.dev, &cmd, &inst_.cmds_[index]));

        // TODO: Begin recording the default command buffers???
        inst_.beginCmd(inst_.cmds_[index]);
    };
}

std::vector<uint32_t> CmdBufHandler::getUniqueQueueFamilies(bool graphics, bool present, bool transfer) {
    std::vector<uint32_t> fams;
    std::vector<uint32_t>::iterator it;
    // graphics
    it = std::find(fams.begin(), fams.end(), inst_.graphics_index());
    if (graphics && it == fams.end()) fams.push_back(inst_.graphics_index());
    // present
    it = std::find(fams.begin(), fams.end(), inst_.present_index());
    if (graphics && it == fams.end()) fams.push_back(inst_.present_index());
    // transfer
    it = std::find(fams.begin(), fams.end(), inst_.transfer_index());
    if (graphics && it == fams.end()) fams.push_back(inst_.transfer_index());

    return fams;
}

void CmdBufHandler::beginCmd(const VkCommandBuffer& cmd, const VkCommandBufferInheritanceInfo* inheritanceInfo) {
    VkCommandBufferBeginInfo cmd_info = {};
    cmd_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_info.pNext = nullptr;
    cmd_info.flags = inheritanceInfo == nullptr
                         ? 0
                         : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    cmd_info.pInheritanceInfo = inheritanceInfo;
    vk::assert_success(vkBeginCommandBuffer(cmd, &cmd_info));
}

void CmdBufHandler::endCmd(const VkCommandBuffer& cmd) { vk::assert_success(vkEndCommandBuffer(cmd)); }
