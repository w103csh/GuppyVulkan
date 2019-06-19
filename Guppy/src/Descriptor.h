#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <vulkan/vulkan.h>

namespace Descriptor {
class Interface {
   public:
    virtual void setWriteInfo(VkWriteDescriptorSet& write, uint32_t index = 0) const = 0;
};
}  // namespace Descriptor

#endif  // !DESCRIPTOR_H
