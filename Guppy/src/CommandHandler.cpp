
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
        createCmdBuffers(pools_.at(queueFamilyIndex), &cmds_.at(queueFamilyIndex), VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
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

void Command::Handler::createCmdBuffers(const VkCommandPool& pool, VkCommandBuffer* pCommandBuffers,
                                        VkCommandBufferLevel level, uint32_t count) {
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext = nullptr;
    alloc_info.commandPool = pool;
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

const VkQueue& Command::Handler::getQueue(const QUEUE type) {
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

const VkCommandPool& Command::Handler::getCmdPool(const QUEUE type) {
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

const VkCommandBuffer& Command::Handler::getCmd(const QUEUE type) {
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
    // compute
    it = std::find(fams.begin(), fams.end(), computeIndex());
    if (graphics && it == fams.end()) fams.push_back(computeIndex());

    return fams;
}

void Command::Handler::beginCmd(const VkCommandBuffer& cmd, const VkCommandBufferInheritanceInfo* inheritanceInfo) const {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.pNext = nullptr;
    begin_info.flags = inheritanceInfo == nullptr
                           ? 0
                           : VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_PASS_CONTINUE_BIT;
    begin_info.pInheritanceInfo = inheritanceInfo;
    vk::assert_success(vkBeginCommandBuffer(cmd, &begin_info));
}

void Command::Handler::endCmd(const VkCommandBuffer& cmd) const { vk::assert_success(vkEndCommandBuffer(cmd)); }
