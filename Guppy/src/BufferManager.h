#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <algorithm>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "BufferItem.h"
#include "Helpers.h"
#include "Shell.h"

namespace Buffer {
namespace Manager {

template <typename T>
class Data {
   public:
    Data(VkDeviceSize maxSize, VkDeviceSize alignment)  //
        : TOTAL_SIZE(maxSize * alignment), ALIGNMENT(alignment) {
        pData_ = (uint8_t *)std::calloc(1, TOTAL_SIZE);
    }

    const VkDeviceSize TOTAL_SIZE;
    const VkDeviceSize ALIGNMENT;

    inline const void *data() const { return pData_; }

    inline void set(VkDeviceSize index, const std::vector<T> &data) {
        auto offset = index * ALIGNMENT;
        assert(offset + (ALIGNMENT * data.size()) <= TOTAL_SIZE);
        for (uint64_t i = 0; i < data.size(); i++)  //
            std::memcpy((pData_ + (offset + (i * ALIGNMENT))), &data[i], sizeof(T));
    }

    inline T &get(VkDeviceSize index) {
        auto offset = index * ALIGNMENT;
        assert(offset + ALIGNMENT <= TOTAL_SIZE);
        return (T &)(*(pData_ + offset));
    }

   private:
    // TODO: change this to std::array so memory is initialized.
    // As of now all buffer memory copies junk into the device buffer,
    // so an initial dirty copy is necessary.
    uint8_t *pData_;
};

template <typename T>
struct Resource {
   public:
    Resource(VkDeviceSize totalSize, VkDeviceSize alignment)  //
        : data(std::forward<VkDeviceSize>(totalSize), std::forward<VkDeviceSize>(alignment)) {}
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceSize currentOffset = 0;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkMemoryRequirements memoryRequirements{};
    void *pMappedData = nullptr;
    Buffer::Manager::Data<T> data;
};

template <class TBase, class TDerived, template <typename> class TSmartPointer>
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
          FLAGS(flags),
          alignment_(sizeof(typename TDerived::DATA)) {}

    const std::string NAME;
    const VkDeviceSize MAX_SIZE;
    const bool KEEP_MAPPED;
    const VkBufferUsageFlags USAGE;
    const VkMemoryPropertyFlags PROPERTIES;
    const VkSharingMode MODE;
    const VkBufferCreateFlags FLAGS;

    virtual void init(const Shell::Context &ctx, std::vector<uint32_t> queueFamilyIndices = {}) {
        reset(ctx.dev);
        queueFamilyIndices_ = queueFamilyIndices;
        createBuffer(ctx);
    }

    template <typename TCreateInfo>
    void insert(const VkDevice &dev, TCreateInfo *pCreateInfo) {
        assert(pItems.size() < MAX_SIZE);
        auto info = fill(dev, std::vector<typename TDerived::DATA>(pCreateInfo->dataCount), pCreateInfo->countInRange);
        pItems.emplace_back(new TDerived(std::move(info), get(info), pCreateInfo));
        if (pCreateInfo == nullptr || pCreateInfo->update) updateData(dev, pItems.back()->BUFFER_INFO);
    }

    void insert(const VkDevice &dev, bool update = true,
                const std::vector<typename TDerived::DATA> &data = std::vector<typename TDerived::DATA>(1)) {
        assert(pItems.size() < MAX_SIZE);
        auto info = fill(dev, data, false);
        pItems.emplace_back(new TDerived(std::move(info), get(info)));
        if (update) updateData(dev, pItems.back()->BUFFER_INFO);
    }

    void updateData(const VkDevice &dev, const Buffer::Info &info, const int index = -1) {
        if (pItems[info.itemOffset]->dirty) {
            updateMappedMemory(dev, info, index);
            pItems[info.itemOffset]->dirty = false;
        }
    }

    TDerived &getTypedItem(const uint32_t &index) { return std::ref(*(TDerived *)(pItems.at(index).get())); }

    void destroy(const VkDevice &dev) {
        reset(dev);
        pItems.clear();
    }

    std::vector<TSmartPointer<TBase>> pItems;  // TODO: public?

   protected:
    virtual void setInfo(Buffer::Info &info){};

    Buffer::Info fill(const VkDevice &dev, const std::vector<typename TDerived::DATA> data, const bool countInRange) {
        assert(resources_.size() && "Did you initialize the manager?");
        auto &resource = resources_.back();

        bool isValid = true;
        isValid = !data.empty();
        assert(isValid && "\"data\" cannot be empty.");
        isValid &= ((resource.currentOffset + data.size()) * alignment_) <= resource.data.TOTAL_SIZE;
        assert(isValid && "Either increase the max size, or figure out how to grow the \"resources_\".");
        // TODO: other validation? also, does above make any sense?

        Buffer::Info info = {};
        info.bufferInfo.buffer = resource.buffer;
        info.bufferInfo.range = countInRange ? alignment_ * data.size() : alignment_;
        // Note: The offset for descriptor buffer info is 0 for a dynamic buffer. This
        // is overriden in "Descriptor::Manager::setInfo".
        info.memoryOffset = info.bufferInfo.offset = alignment_ * resource.currentOffset;
        info.resourcesOffset = resources_.size() - 1;
        info.dataOffset = resource.currentOffset;
        // TODO: putting this here could be super confusing. It relies on the update
        // functions to create a new item.
        info.itemOffset = static_cast<uint32_t>(pItems.size());

        resource.data.set(resource.currentOffset, data);
        resource.currentOffset += data.size();
        info.count = static_cast<uint32_t>(data.size());
        assert(info.count > 0);

        // Let derived classes manipulate the info data if necessary.
        setInfo(info);

        return info;
    }

    VkDeviceSize alignment_;

   private:
    // TODO: change the caller so that all the memory can be updated at once.
    void updateMappedMemory(const VkDevice &dev, const Buffer::Info &info, const int index) {
        auto &resource = resources_[info.resourcesOffset];

        // If the index was set offset into both memory pointers.
        auto memoryOffset = info.memoryOffset + ((index == -1) ? 0 : (index * resource.data.ALIGNMENT));
        auto dataOffset = info.dataOffset + ((index == -1) ? 0 : index);

        auto pData = static_cast<uint8_t *>(resource.pMappedData) + memoryOffset;
        // If index is not set copy the entire range for the item.
        VkDeviceSize range = (index == -1) ? (info.count * resource.data.ALIGNMENT) : resource.data.ALIGNMENT;

        if (KEEP_MAPPED) {
            memcpy(pData, &resource.data.get(dataOffset), range);
        } else {
            // TODO: Only map the region that is being copied???
            vk::assert_success(
                vkMapMemory(dev, resource.memory, 0, resource.memoryRequirements.size, 0, &resource.pMappedData));
            memcpy(pData, &resource.data.get(dataOffset), range);
            vkUnmapMemory(dev, resource.memory);
        }
    }

    void reset(const VkDevice &dev) {
        for (auto &resource : resources_) {
            if (KEEP_MAPPED) vkUnmapMemory(dev, resource.memory);
            vkDestroyBuffer(dev, resource.buffer, nullptr);
            vkFreeMemory(dev, resource.memory, nullptr);
        }
    }

    void createBuffer(const Shell::Context &ctx) {
        resources_.push_back({MAX_SIZE, alignment_});
        auto &resource = resources_.back();

        // TODO: I FORGOT TO USE A STAGING BUFFER HERE TO SPEED THIS UP!!!

        // CREATE BUFFER

        VkBufferCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.flags = FLAGS;
        createInfo.size = MAX_SIZE * alignment_;
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
        auto pass = helpers::getMemoryType(ctx.memProps, resource.memoryRequirements.memoryTypeBits, PROPERTIES,
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
            each item will do a memcpy if dirty after creation. Maybe just make that a necessary step,
            and leave the rest of the buffer garbage.
        */
        memcpy(resource.pMappedData, resource.data.data(), static_cast<size_t>(resource.memoryRequirements.size));
        if (!KEEP_MAPPED) vkUnmapMemory(ctx.dev, resource.memory);

        // BIND MEMORY

        vk::assert_success(vkBindBufferMemory(ctx.dev, resource.buffer, resource.memory, 0));

        if (ctx.debugMarkersEnabled) {
            std::string markerName = NAME + " block (" + std::to_string(resources_.size()) + ")";
            ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)resource.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                          markerName.c_str());

            // ext::DebugMarkerSetObjectTag(ctx.dev, (uint64_t)resources_.back().buffer,
            // VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0,
            //                             sizeof(_UniformTag), &tag);
        }
    }

    typename TDerived::DATA *get(const Buffer::Info &info) {
        return &resources_[info.resourcesOffset].data.get(info.dataOffset);
    }

    std::vector<uint32_t> queueFamilyIndices_;
    std::vector<Manager::Resource<typename TDerived::DATA>> resources_;
};

}  // namespace Manager
}  // namespace Buffer

#endif  // !BUFFER_MANAGER_H
