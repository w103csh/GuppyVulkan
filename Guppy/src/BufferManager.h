/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef BUFFER_MANAGER_H
#define BUFFER_MANAGER_H

#include <algorithm>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>
#define DIAGNOSE false
#if DIAGNOSE
#include <iostream>
#endif

#include <Common/Context.h>
#include <Common/Helpers.h>

#include "BufferItem.h"
#include "Shell.h"

namespace Buffer {
namespace Manager {

template <typename T>
class Data {
   public:
    Data(vk::DeviceSize maxSize, vk::DeviceSize alignment)  //
        : TOTAL_SIZE(maxSize * alignment), ALIGNMENT(alignment) {
        pData_ = (uint8_t *)std::calloc(1, TOTAL_SIZE);
    }

    const vk::DeviceSize TOTAL_SIZE;
    const vk::DeviceSize ALIGNMENT;

    inline const void *data() const { return pData_; }

    inline void set(vk::DeviceSize index, const std::vector<T> &data) {
        auto offset = index * ALIGNMENT;
        assert(offset + (ALIGNMENT * data.size()) <= TOTAL_SIZE);
        for (uint64_t i = 0; i < data.size(); i++)  //
            std::memcpy((pData_ + (offset + (i * ALIGNMENT))), &data[i], sizeof(T));
    }

    inline T &get(vk::DeviceSize index) {
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
    Resource(vk::DeviceSize totalSize, vk::DeviceSize alignment)
        : buffer(),
          currentOffset(),
          memory(),
          memoryRequirements(),
          pMappedData(nullptr),
          data(std::forward<vk::DeviceSize>(totalSize), std::forward<vk::DeviceSize>(alignment)) {}
    vk::Buffer buffer;
    vk::DeviceSize currentOffset;
    vk::DeviceMemory memory;
    vk::MemoryRequirements memoryRequirements;
    void *pMappedData;
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
    Base(const std::string &&name, const vk::DeviceSize &&maxSize, const bool &&keepMapped,
         const vk::BufferUsageFlags &&usage, const vk::MemoryPropertyFlags &&properties,
         const vk::SharingMode &&sharingMode = vk::SharingMode::eExclusive, const vk::BufferCreateFlags &&flags = {})
        : NAME(name),
          MAX_SIZE(maxSize),
          KEEP_MAPPED(keepMapped),
          USAGE(usage),
          PROPERTIES(properties),
          MODE(sharingMode),
          FLAGS(flags),
          alignment_(sizeof(typename TDerived::DATA)) {}

    const std::string NAME;
    const vk::DeviceSize MAX_SIZE;
    const bool KEEP_MAPPED;
    const vk::BufferUsageFlags USAGE;
    const vk::MemoryPropertyFlags PROPERTIES;
    const vk::SharingMode MODE;
    const vk::BufferCreateFlags FLAGS;

    virtual void init(const Context &ctx, std::vector<uint32_t> queueFamilyIndices = {}) {
        reset(ctx);
        queueFamilyIndices_ = queueFamilyIndices;
        createBuffer(ctx);
    }

#if DIAGNOSE
    void diagnose(const Buffer::Info &info) const {
        std::cout << info.bufferInfo.buffer << std::endl;  //
    }
#else
    void diagnose(const Buffer::Info &info) const {}
#endif

    template <typename TCreateInfo>
    TDerived *insert(const vk::Device &dev, TCreateInfo *pCreateInfo) {
        assert(pItems.size() < MAX_SIZE);
        auto info = fill(dev, std::vector<typename TDerived::DATA>(pCreateInfo->dataCount), pCreateInfo->countInRange);
        diagnose(info);
        pItems.emplace_back(new TDerived(std::move(info), get(info), pCreateInfo));
        if (pCreateInfo == nullptr || pCreateInfo->update) updateData(dev, pItems.back()->BUFFER_INFO);
        return static_cast<TDerived *>(pItems.back().get());
    }

    TDerived *insert(const vk::Device &dev, bool update = true,
                     const std::vector<typename TDerived::DATA> &data = std::vector<typename TDerived::DATA>(1)) {
        assert(pItems.size() < MAX_SIZE);
        auto info = fill(dev, data, false);
        diagnose(info);
        pItems.emplace_back(new TDerived(std::move(info), get(info)));
        if (update) updateData(dev, pItems.back()->BUFFER_INFO);
        return static_cast<TDerived *>(pItems.back().get());
    }

    void updateData(const vk::Device &dev, const Buffer::Info &info, const int index = -1) {
        if (pItems[info.itemOffset]->dirty) {
            updateMappedMemory(dev, info, index);
            pItems[info.itemOffset]->dirty = false;
        }
    }

    TDerived &getTypedItem(const uint32_t &index) { return std::ref(*static_cast<TDerived *>(pItems.at(index).get())); }

    void destroy(const Context &ctx) {
        reset(ctx);
        pItems.clear();
    }

    std::vector<TSmartPointer<TBase>> pItems;  // TODO: public?

   protected:
    virtual void setInfo(Buffer::Info &info){};

    Buffer::Info fill(const vk::Device &dev, const std::vector<typename TDerived::DATA> data, const bool countInRange) {
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

    vk::DeviceSize alignment_;

   private:
    // TODO: change the caller so that all the memory can be updated at once.
    void updateMappedMemory(const vk::Device &dev, const Buffer::Info &info, const int index) {
        auto &resource = resources_[info.resourcesOffset];

        // If the index was set offset into both memory pointers.
        auto memoryOffset = info.memoryOffset + ((index == -1) ? 0 : (index * resource.data.ALIGNMENT));
        auto dataOffset = info.dataOffset + ((index == -1) ? 0 : index);

        auto pData = static_cast<uint8_t *>(resource.pMappedData) + memoryOffset;
        // If index is not set copy the entire range for the item.
        vk::DeviceSize range = (index == -1) ? (info.count * resource.data.ALIGNMENT) : resource.data.ALIGNMENT;

        if (KEEP_MAPPED) {
            memcpy(pData, &resource.data.get(dataOffset), range);
        } else {
            // TODO: Only map the region that is being copied???
            resource.pMappedData = dev.mapMemory(resource.memory, 0, resource.memoryRequirements.size);

            memcpy(pData, &resource.data.get(dataOffset), range);
            dev.unmapMemory(resource.memory);
        }
    }

    void reset(const Context &ctx) {
        for (auto &resource : resources_) {
            if (KEEP_MAPPED) ctx.dev.unmapMemory(resource.memory);
            ctx.dev.destroyBuffer(resource.buffer, ctx.pAllocator);
            ctx.dev.freeMemory(resource.memory, ctx.pAllocator);
        }
    }

    void createBuffer(const Context &ctx) {
        resources_.push_back({MAX_SIZE, alignment_});
        auto &resource = resources_.back();

        // TODO: I FORGOT TO USE A STAGING BUFFER HERE TO SPEED THIS UP!!!

        // CREATE BUFFER

        vk::BufferCreateInfo createInfo = {};
        createInfo.flags = FLAGS;
        createInfo.size = MAX_SIZE * alignment_;
        createInfo.usage = USAGE;
        createInfo.sharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices_.size());
        createInfo.pQueueFamilyIndices = queueFamilyIndices_.data();
        resource.buffer = ctx.dev.createBuffer(createInfo, ctx.pAllocator);

        // ALLOCATE DEVICE MEMORY
        resource.memoryRequirements = ctx.dev.getBufferMemoryRequirements(resource.buffer);

        assert(resource.memoryRequirements.size == createInfo.size &&
               "Figure out how to deal with this! (\"range\" of add)");

        vk::MemoryAllocateInfo allocInfo;
        allocInfo.setAllocationSize(resource.memoryRequirements.size);
        auto pass = helpers::getMemoryType(ctx.memProps, resource.memoryRequirements.memoryTypeBits, PROPERTIES,
                                           &allocInfo.memoryTypeIndex);
        assert(pass && "No mappable, coherent memory");

        resource.memory = ctx.dev.allocateMemory(allocInfo, ctx.pAllocator);

        // MAP MEMORY

        // TODO: why doesn't below work WWWWTTTTTTFFFFFFFFFFF!!!!!!
        // auto pData = resource.data.data();
        //  resource.pMappedData = ctx.dev.mapMemory(resource.memory, 0, memReqs.size);
        resource.pMappedData = ctx.dev.mapMemory(resource.memory, 0, resource.memoryRequirements.size);

        /*  Copying all the memory here is probably a redunant init step. The way its written now,
            each item will do a memcpy if dirty after creation. Maybe just make that a necessary step,
            and leave the rest of the buffer garbage.
        */
        memcpy(resource.pMappedData, resource.data.data(), static_cast<size_t>(resource.memoryRequirements.size));
        if (!KEEP_MAPPED) ctx.dev.unmapMemory(resource.memory);

        // BIND MEMORY

        ctx.dev.bindBufferMemory(resource.buffer, resource.memory, 0);
        std::string markerName = NAME + " block (" + std::to_string(resources_.size()) + ")";
        // ctx.dbg.setMarkerName(resource.buffer, markerName.c_str());
        // ctx.dbg.setMarkerTag(resource.buffer, markerName.c_str(), tag);
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
