#ifndef BUFFER_ITEM_H
#define BUFFER_ITEM_H

#include <assert.h>
#include <functional>

#include <vulkan/vulkan.h>

namespace Buffer {

struct CreateInfo {
    uint32_t dataCount = 1;
    bool update = true;
};

struct Info {
    VkDescriptorBufferInfo bufferInfo{VK_NULL_HANDLE, 0, 0};
    uint32_t count = 0;
    VkDeviceSize dataOffset = 0;
    VkDeviceSize itemOffset = 0;
    VkDeviceSize memoryOffset = 0;
    VkDeviceSize resourcesOffset = 0;
};

class Item {
   public:
    Item(const Buffer::Info&& info)  //
        : BUFFER_INFO(info), dirty(false) {
        assert(BUFFER_INFO.bufferInfo.buffer != VK_NULL_HANDLE);
    }
    virtual ~Item() = default;

    const Buffer::Info BUFFER_INFO;
    bool dirty;

   protected:
    /*  Virtual inheritance only.
        I am going to assert here to make it clear that its best to avoid this
        constructor being called. It gave me some headaches, so this will hopefully
        force me to call the constructors in the same order. The order is that "the
        most derived class calls the constructor" of the virtually inherited
        class (this), so just add the public constructor above to that level.
    */
    Item() : dirty(false) { assert(false); }
};

template <typename TDATA>
class DataItem : public virtual Buffer::Item {
   public:
    using DATA = TDATA;

    DataItem(TDATA* pData) : pData_(pData) { assert(pData_); }

    virtual void setData(const uint32_t index = 0) {}

   protected:
    TDATA* pData_;
};  // namespace Buffer

// This is pretty slow on a bunch of levels, but I'd rather just have it work atm.
template <typename TDATA>
class PerFramebufferDataItem : public Buffer::DataItem<TDATA> {
   public:
    PerFramebufferDataItem(TDATA* pData) : Buffer::DataItem<TDATA>(pData), data_(*pData) {}

   protected:
    void setData(const uint32_t index = UINT32_MAX) override {
        if (index == UINT32_MAX) {
            for (uint32_t i = 0; i < Item::BUFFER_INFO.count; i++)
                std::memcpy(&DataItem<TDATA>::pData_[i], &data_, sizeof(TDATA));
        } else {
            assert(index < Item::BUFFER_INFO.count);
            std::memcpy(&DataItem<TDATA>::pData_[index], &data_, sizeof(TDATA));
        }
        Item::dirty = true;
    }

    TDATA data_;
};

}  // namespace Buffer

#endif  // !BUFFER_ITEM_H
