#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <vulkan/vulkan.h>

#include "BufferItem.h"
#include "DescriptorConstants.h"

namespace Descriptor {

class Interface {
   public:
    virtual void setDescriptorInfo(Set::ResourceInfo& info, const uint32_t index) const = 0;
};

class Base : public virtual Buffer::Item, public Descriptor::Interface {
   public:
    void setDescriptorInfo(Set::ResourceInfo& info, const uint32_t index) const override;

   private:
    VkDescriptorBufferInfo getBufferInfo(const uint32_t index = 0) const;

   protected:
    Base() {}
};

}  // namespace Descriptor

#endif  // !DESCRIPTOR_H
