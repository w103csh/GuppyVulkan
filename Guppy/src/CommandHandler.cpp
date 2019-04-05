
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
        pools_[queueFamilyIndex] = VK_NULL_HANDLE;
        createCmdPool(queueFamilyIndex, pools_.at(queueFamilyIndex));
        // BUFFERS
        cmds_[queueFamilyIndex] = VK_NULL_HANDLE;
        createCmdBuffers(pools_.at(queueFamilyIndex), &cmds_.at(queueFamilyIndex));
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
            vkFreeCommandBuffers(shell().context().dev, pools_.at(queueFamilyIndex), 1, &cmds_.at(queueFamilyIndex));
        // pools
        if (pools_.count(queueFamilyIndex))
            vkDestroyCommandPool(shell().context().dev, pools_.at(queueFamilyIndex), nullptr);
    }
    cmds_.clear();
    pools_.clear();
}

void Command::Handler::createCmdPool(const uint32_t queueFamilyIndex, VkCommandPool& cmdPool) {
    VkCommandPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_info.queueFamilyIndex = queueFamilyIndex;
    cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    vk::assert_success(vkCreateCommandPool(shell().context().dev, &cmd_pool_info, nullptr, &cmdPool));
}

void Command::Handler::createCmdBuffers(const VkCommandPool& cmdPool, VkCommandBuffer* pCommandBuffers,
                                        VkCommandBufferLevel level, uint32_t count) {
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.commandPool = cmdPool;
    alloc_info.level = level;
    alloc_info.commandBufferCount = count;

    vk::assert_success(vkAllocateCommandBuffers(shell().context().dev, &alloc_info, pCommandBuffers));
}

void Command::Handler::resetCmdBuffers() {
    for (auto& keyValue : cmds_) {
        vkResetCommandBuffer(keyValue.second, 0);
        beginCmd(keyValue.second);
    }
}

// INDEX
uint32_t Command::Handler::graphicsIndex() const { return shell().context().graphics_index; }
uint32_t Command::Handler::presentIndex() const { return shell().context().present_index; }
uint32_t Command::Handler::transferIndex() const { return shell().context().transfer_index; }
// QUEUE
const VkQueue& Command::Handler::graphicsQueue() const { return shell().context().queues.at(shell().context().graphics_index); }
const VkQueue& Command::Handler::presentQueue() const { return shell().context().queues.at(shell().context().present_index); }
const VkQueue& Command::Handler::transferQueue() const { return shell().context().queues.at(shell().context().transfer_index); }
// POOLS
const VkCommandPool& Command::Handler::graphicsCmdPool() const { return pools_.at(shell().context().graphics_index); }
const VkCommandPool& Command::Handler::presentCmdPool() const { return pools_.at(shell().context().present_index); }
const VkCommandPool& Command::Handler::transferCmdPool() const { return pools_.at(shell().context().transfer_index); }
// COMMANDS
const VkCommandBuffer& Command::Handler::graphicsCmd() const { return cmds_.at(shell().context().graphics_index); }
const VkCommandBuffer& Command::Handler::presentCmd() const { return cmds_.at(shell().context().present_index); }
const VkCommandBuffer& Command::Handler::transferCmd() const { return cmds_.at(shell().context().transfer_index); }

std::vector<uint32_t> Command::Handler::getUniqueQueueFamilies(bool graphics, bool present, bool transfer) const {
    std::vector<uint32_t> fams;
    std::vector<uint32_t>::iterator it;
    // graphics
    it = std::find(fams.begin(), fams.end(), graphicsIndex());
    if (graphics && it == fams.end()) fams.push_back(graphicsIndex());
    // present
    it = std::find(fams.begin(), fams.end(), presentIndex());
    if (graphics && it == fams.end()) fams.push_back(presentIndex());
    // transfer
    it = std::find(fams.begin(), fams.end(), transferIndex());
    if (graphics && it == fams.end()) fams.push_back(transferIndex());

    return fams;
}

void Command::Handler::beginCmd(const VkCommandBuffer& cmd, const VkCommandBufferInheritanceInfo* inheritanceInfo) const {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = inheritanceInfo == nullptr
                           ? 0
                           : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    begin_info.pInheritanceInfo = inheritanceInfo;
    vk::assert_success(vkBeginCommandBuffer(cmd, &begin_info));
}

void Command::Handler::endCmd(const VkCommandBuffer& cmd) const { vk::assert_success(vkEndCommandBuffer(cmd)); }
