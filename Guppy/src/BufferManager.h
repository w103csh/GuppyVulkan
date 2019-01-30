#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Shell.h"
#include "Helpers.h"

namespace Buffer {

class Item {
   public:
    struct Info {
        VkDeviceSize resourcesOffset;
        VkDeviceSize dataOffset;
        VkDescriptorBufferInfo bufferInfo;
    };

   protected:
    Info bufferInfo;
};

template <typename T>
class Manager {
   public:
    Manager(const std::string &&name, const VkDeviceSize &maxSize, const VkBufferUsageFlagBits &&usage,
            const VkMemoryPropertyFlagBits &&properties, const VkSharingMode &sharingMode = VK_SHARING_MODE_EXCLUSIVE,
            const VkBufferCreateFlags &&flags = 0)
        : NAME(name), MAX_SIZE(maxSize * sizeof T), USAGE(usage), PROPERTIES(properties), MODE(sharingMode), FLAGS(flags) {}

    const std::string NAME;
    const uint64_t MAX_SIZE;
    const VkBufferUsageFlags USAGE;
    const VkMemoryPropertyFlags PROPERTIES;
    const VkSharingMode MODE;
    const VkBufferCreateFlags FLAGS;

    void init(const Shell::Context &ctx, const Game::Settings &settings, std::vector<uint32_t> queueFamilyIndices = {}) {
        reset(ctx.dev);
        queueFamilyIndices_ = queueFamilyIndices;
        createBuffer(ctx, settings);
    }

    Item::Info insert(const VkDevice &dev, T &dataItem) {
        auto &resource = resources_.back();

        assert((resource.data.size() + 1) < MAX_SIZE &&
               "Either up the max size or figure out how to grow the \"resources_\".");
        assert(false && "Do dynamic size check!!!");

        Item::Info info = {};
        info.resourcesOffset = resources_.size() - 1;
        info.dataOffset = resource.currentOffset++;
        info.bufferInfo.buffer = resource.buffer;
        info.bufferInfo.range = sizeof T;
        info.bufferInfo.offset = info.dataOffset * info.bufferInfo.range;

        resource.data[info.dataOffset] = dataItem;

        // MAP MEMORY (TODO: should this be all at once?)
        vkMapMemory(dev, resource.memory, info.bufferInfo.offset, info.bufferInfo.range, 0,
                    (void **)&resource.data[info.dataOffset]);

        return info;
    }

    T &get(const Item::Info &info) { return resources_[info.resourcesOffset].data[info.dataOffset]; }

    void destroy(const Item::Info &info) { vkUnmapMemory(dev) }

    void destroy(const VkDevice &dev) { reset(dev); }

   private:
    struct Resource {
        VkDeviceSize currentOffset = 0;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        std::vector<T> data;
    };

    void reset(const VkDevice &dev) {
        for (auto &resource : resources_) {
            vkUnmapMemory(dev, resource.memory);
            vkDestroyBuffer(dev, resource.buffer, nullptr);
            vkFreeMemory(dev, resource.memory, nullptr);
        }
    }

    void createBuffer(const Shell::Context &ctx, const Game::Settings &settings) {
        resources_.push_back({});
        resources_.back().data.resize(MAX_SIZE);

        // CREATE BUFFER

        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.flags = FLAGS;
        createInfo.size = MAX_SIZE;
        createInfo.usage = USAGE;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices_.size());
        createInfo.pQueueFamilyIndices = queueFamilyIndices_.data();

        vk::assert_success(vkCreateBuffer(ctx.dev, &createInfo, nullptr, &resources_.back().buffer));

        // ALLOCATE DEVICE MEMORY

        VkMemoryRequirements memReqs;
        vkGetBufferMemoryRequirements(ctx.dev, resources_.back().buffer, &memReqs);

        assert(memReqs.size == MAX_SIZE && "Figure out how to deal with this! (\"range\" of add)");

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReqs.size;
        auto pass = helpers::getMemoryType(ctx.mem_props, memReqs.memoryTypeBits, PROPERTIES, &allocInfo.memoryTypeIndex);
        assert(pass && "No mappable, coherent memory");

        vk::assert_success(vkAllocateMemory(ctx.dev, &allocInfo, nullptr, &resources_.back().memory));

        // BIND MEMORY
        vk::assert_success(vkBindBufferMemory(ctx.dev, resources_.back().buffer, resources_.back().memory, 0));

        if (settings.enable_debug_markers) {
            std::string markerName = NAME + " block (" + std::to_string(resources_.size()) + ")";
            ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)resources_.back().buffer,
                                          VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, markerName.c_str());

            // ext::DebugMarkerSetObjectTag(ctx.dev, (uint64_t)resources_.back().buffer,
            // VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0,
            //                             sizeof(_UniformTag), &tag);
        }
    }

    // CREATE
    std::vector<uint32_t> queueFamilyIndices_;
    // DATA
    std::vector<Resource> resources_;
};

}  // namespace Buffer

#endif  // !BUFFER_MANAGER_H
