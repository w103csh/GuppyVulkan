#ifndef BUFFER_ITEM_H
#define BUFFER_ITEM_H

#include <assert.h>

#include <vulkan/vulkan.h>

namespace Buffer {

struct Info {
    VkDescriptorBufferInfo bufferInfo{VK_NULL_HANDLE, 0, 0};
    VkDeviceSize dataOffset = 0;
    uint32_t dynamicOffset = 0;
    VkDeviceSize resourcesOffset = 0;
};

class Item {
   public:
    Item(const Buffer::Info&& info) : BUFFER_INFO(info), DIRTY(false) {
        assert(BUFFER_INFO.bufferInfo.buffer != VK_NULL_HANDLE);
    }
    virtual ~Item() = default;

    uint32_t getDynamicOffset() { return static_cast<uint32_t>(BUFFER_INFO.dataOffset * BUFFER_INFO.bufferInfo.range); };

    const Buffer::Info BUFFER_INFO;
    bool DIRTY;

   protected:
    /*  Virtual inheritance only.

        I am going to assert here to make it clear that its best to avoid this
        constructor being called. It gave me some headaches, so this will hopefully
        force me to call the constructors in the same order. The order is that "the
        most derived class calls the constructor" of the virtually inherited
        class (this), so just add the public constructor above to that level.
    */
    Item() { assert(false); }
};

template <typename T>
class DataItem : public virtual Buffer::Item {
   public:
    typedef T DATA;

    DataItem(T* pData) : pData_(pData) { assert(pData_); }

    T& getRef() { return std::ref(*pData_); }
    virtual void setData() {}

   protected:
    T* pData_;
};  // namespace Buffer

}  // namespace Buffer

#endif  // !BUFFER_ITEM_H
