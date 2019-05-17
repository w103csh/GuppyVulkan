#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <vulkan/vulkan.h>

namespace Descriptor {
class Interface {
   public:
    virtual void setWriteInfo(VkWriteDescriptorSet& write) const = 0;
};
}  // namespace Descriptor

#endif  // !DESCRIPTOR_H
