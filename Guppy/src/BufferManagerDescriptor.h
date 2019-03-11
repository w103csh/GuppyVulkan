#ifndef BUFFER_MANAGER_DESCRIPTOR_H
#define BUFFER_MANAGER_DESCRIPTOR_H

#include <memory>
#include <string>
#include <vulkan/vulkan.h>

#include "BufferManager.h"

namespace Buffer {
namespace Manager {

template <class TBase, class TDerived, template <typename> class TSmartPointer = std::unique_ptr>
class Descriptor : public Buffer::Manager::Base<TBase, TDerived, TSmartPointer> {
   public:
    Descriptor(const std::string &&name, const DESCRIPTOR &&descriptorType, const VkDeviceSize &&maxSize,
               const bool &&keepMapped, const VkBufferUsageFlagBits &&usage, const VkMemoryPropertyFlagBits &&properties,
               const std::string &&macroName = "N/A", const VkSharingMode &&sharingMode = VK_SHARING_MODE_EXCLUSIVE,
               const VkBufferCreateFlags &&flags = 0)
        : Buffer::Manager::Base<TBase, TDerived, TSmartPointer>(std::forward<const std::string>(name),                     //
                                                                std::forward<const VkDeviceSize>(maxSize),                 //
                                                                std::forward<const bool>(keepMapped),                      //
                                                                std::forward<const VkBufferUsageFlagBits>(usage),          //
                                                                std::forward<const VkMemoryPropertyFlagBits>(properties),  //
                                                                std::forward<const VkSharingMode>(sharingMode),            //
                                                                std::forward<const VkBufferCreateFlags>(flags)),
          DESCRIPTOR_TYPE(descriptorType),
          MACRO_NAME(macroName) {}

    const DESCRIPTOR DESCRIPTOR_TYPE;
    const std::string MACRO_NAME;

   private:
    void setInfo(const Manager::Resource<typename TDerived::DATA> &resource, Buffer::Info &info) override {
        info.bufferInfo.buffer = resource.buffer;
        info.bufferInfo.range = sizeof TDerived::DATA;
        info.bufferInfo.offset =
            helpers::isDescriptorTypeDynamic(DESCRIPTOR_TYPE) ? 0 : (info.dataOffset * info.bufferInfo.range);
    }
};

}  // namespace Manager
}  // namespace Buffer

#endif  // !BUFFER_MANAGER_DESCRIPTOR_H
