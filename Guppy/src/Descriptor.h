/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef DESCRIPTOR_H
#define DESCRIPTOR_H

#include <vulkan/vulkan.h>

#include "Constants.h"
#include "BufferItem.h"
#include "DescriptorConstants.h"

namespace Descriptor {

constexpr uint32_t PAUSED_UPDATE = UINT32_MAX;

class Interface {
   public:
    virtual void setDescriptorInfo(Set::ResourceInfo& info, const uint32_t index) const = 0;
};

class Base : public virtual Buffer::Item, public Descriptor::Interface {
   public:
    void setDescriptorInfo(Set::ResourceInfo& info, const uint32_t index) const override;
    virtual_inline auto getDescriptorType() const { return descType_; }
    virtual void updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) {}

   protected:
    Base(const DESCRIPTOR&& descType) : descType_(descType) {}

   private:
    VkDescriptorBufferInfo getBufferInfo(const uint32_t index = 0) const;

    DESCRIPTOR descType_;
};

}  // namespace Descriptor

#endif  // !DESCRIPTOR_H
