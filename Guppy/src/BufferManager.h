#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "BufferItem.h"
#include "Shell.h"
#include "Helpers.h"

namespace Buffer {
namespace Manager {

template <typename T>
struct Resource {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceSize currentOffset = 0;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkMemoryRequirements memoryRequirements;
    void *pMappedData;
    std::vector<T> data;
};

template <class TBase, class TDerived, template <typename> class TSmartPointer = std::unique_ptr>
class Base {
   public:
    /*  Last I checked, forcing a "const maxSize" here is very important unless a proper
        allocator is written. The mapped memory, and the info objects that are handed out
        will be invalidated if there is a vector resize operation. So, for now I am just
        setting a max size, adding asserts that I don't request anything over the original
        allocation amount, and then recommend just increasing the max size... until I
        write the allocation code.
    */
    Base(const std::string &&name, const VkDeviceSize &&maxSize, const bool &&keepMapped,
         const VkBufferUsageFlagBits &&usage, const VkMemoryPropertyFlagBits &&properties,
         const VkSharingMode &&sharingMode = VK_SHARING_MODE_EXCLUSIVE, const VkBufferCreateFlags &&flags = 0)
        : NAME(name),
          MAX_SIZE(maxSize),
          KEEP_MAPPED(keepMapped),
          USAGE(usage),
          PROPERTIES(properties),
          MODE(sharingMode),
          FLAGS(flags) {}

    const std::string NAME;
    const VkDeviceSize MAX_SIZE;
    const bool KEEP_MAPPED;
    const VkBufferUsageFlags USAGE;
    const VkMemoryPropertyFlags PROPERTIES;
    const VkSharingMode MODE;
    const VkBufferCreateFlags FLAGS;

    virtual void init(const Shell::Context &ctx, const Game::Settings &settings,
                      std::vector<uint32_t> queueFamilyIndices = {}) {
        reset(ctx.dev);
        queueFamilyIndices_ = queueFamilyIndices;
        createBuffer(ctx, settings);
    }

    template <typename TCreateInfo>
    void insert(const VkDevice &dev, TCreateInfo *pCreateInfo) {
        assert(pItems.size() < MAX_SIZE);
        auto info = insert(dev, {});
        pItems.emplace_back(new TDerived(std::move(info), get(info), pCreateInfo));
        update(dev, pItems.back()->BUFFER_INFO);
    }

    void insert(const VkDevice &dev) {
        assert(pItems.size() < MAX_SIZE);
        auto info = insert(dev, {});
        pItems.emplace_back(new TDerived(std::move(info), get(info)));
    }

    void update(const VkDevice &dev, const Buffer::Info &info) {
        if (pItems[info.dataOffset]->DIRTY) {
            updateData(dev, info);
            pItems[info.dataOffset]->DIRTY = false;
        }
    }

    // TODO: change the caller so that all the memory can be updated at once.
    void updateData(const VkDevice &dev, const Buffer::Info &info) {
        auto &resource = resources_[info.resourcesOffset];
        auto pData = static_cast<uint8_t *>(resource.pMappedData) + (info.dataOffset * info.bufferInfo.range);
        if (KEEP_MAPPED) {
            memcpy(pData, &resource.data[info.dataOffset], static_cast<size_t>(info.bufferInfo.range));
            return;
        } else {
            // TODO: only map the region being copied???
            vk::assert_success(
                vkMapMemory(dev, resource.memory, 0, resource.memoryRequirements.size, 0, &resource.pMappedData));
            memcpy(pData, &resource.data[info.dataOffset], static_cast<size_t>(info.bufferInfo.range));
            vkUnmapMemory(dev, resource.memory);
        }
    }

    template <typename T>
    T &getTypedItem(TSmartPointer<TBase> &pItem) {
        return std::ref(*(T *)(pItem.get()));
    }

    void destroy(const VkDevice &dev) {
        reset(dev);
        pItems.clear();
    }

    std::vector<TSmartPointer<TBase>> pItems;  // TODO: public?

   protected:
    virtual void setInfo(const Manager::Resource<typename TDerived::DATA> &resource, Buffer::Info &info) = 0;

    Buffer::Info insert(const VkDevice &dev, typename TDerived::DATA &&dataItem) {
        auto &resource = resources_.back();

        bool isValid = true;

        isValid &= resource.currentOffset < resource.data.size();
        if (!isValid) assert(isValid && "Either up the max size or figure out how to grow the \"resources_\".");
        // TODO: other validation?

        Buffer::Info info = {};
        info.resourcesOffset = resources_.size() - 1;
        info.dataOffset = resource.currentOffset++;
        // SET INFO
        setInfo(resource, info);

        resource.data[info.dataOffset] = dataItem;

        return info;
    }

   private:
    void reset(const VkDevice &dev) {
        for (auto &resource : resources_) {
            if (KEEP_MAPPED) vkUnmapMemory(dev, resource.memory);
            vkDestroyBuffer(dev, resource.buffer, nullptr);
            vkFreeMemory(dev, resource.memory, nullptr);
        }
    }

    void createBuffer(const Shell::Context &ctx, const Game::Settings &settings) {
        resources_.push_back({});
        auto &resource = resources_.back();
        resource.data.resize(MAX_SIZE);

        // CREATE BUFFER

        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.flags = FLAGS;
        createInfo.size = MAX_SIZE * sizeof TDerived::DATA;
        createInfo.usage = USAGE;
        createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices_.size());
        createInfo.pQueueFamilyIndices = queueFamilyIndices_.data();

        vk::assert_success(vkCreateBuffer(ctx.dev, &createInfo, nullptr, &resource.buffer));

        // ALLOCATE DEVICE MEMORY

        vkGetBufferMemoryRequirements(ctx.dev, resource.buffer, &resource.memoryRequirements);

        assert(resource.memoryRequirements.size == createInfo.size &&
               "Figure out how to deal with this! (\"range\" of add)");

        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = resource.memoryRequirements.size;
        auto pass = helpers::getMemoryType(ctx.mem_props, resource.memoryRequirements.memoryTypeBits, PROPERTIES,
                                           &allocInfo.memoryTypeIndex);
        assert(pass && "No mappable, coherent memory");

        vk::assert_success(vkAllocateMemory(ctx.dev, &allocInfo, nullptr, &resource.memory));

        // MAP MEMORY

        // TODO: why doesn't below work WWWWTTTTTTFFFFFFFFFFF!!!!!!
        // auto pData = resource.data.data();
        // vk::assert_success(vkMapMemory(ctx.dev, resource.memory, 0, memReqs.size, 0, (void **)&pData));

        vk::assert_success(
            vkMapMemory(ctx.dev, resource.memory, 0, resource.memoryRequirements.size, 0, &resource.pMappedData));
        /*  Copying all the memory here is probably a redunant init step. The way its written now,
            each item will do a memcpy if DIRTY after creation. Maybe just make that a necessary step,
            and leave the rest of the buffer garbage.
        */
        memcpy(resource.pMappedData, resource.data.data(), static_cast<size_t>(resource.memoryRequirements.size));
        if (!KEEP_MAPPED) vkUnmapMemory(ctx.dev, resource.memory);

        // BIND MEMORY

        vk::assert_success(vkBindBufferMemory(ctx.dev, resource.buffer, resource.memory, 0));

        if (settings.enable_debug_markers) {
            std::string markerName = NAME + " block (" + std::to_string(resources_.size()) + ")";
            ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)resource.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                          markerName.c_str());

            // ext::DebugMarkerSetObjectTag(ctx.dev, (uint64_t)resources_.back().buffer,
            // VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0,
            //                             sizeof(_UniformTag), &tag);
        }
    }

    typename TDerived::DATA *get(const Buffer::Info &info) {
        return &resources_[info.resourcesOffset].data[info.dataOffset];
    }

    std::vector<uint32_t> queueFamilyIndices_;
    std::vector<Manager::Resource<typename TDerived::DATA>> resources_;
};

}  // namespace Manager
}  // namespace Buffer

#endif  // !BUFFER_MANAGER_H
