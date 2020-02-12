/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef INSTANCE_MANAGER_H
#define INSTANCE_MANAGER_H

#include <memory>
#include <string>
#include <type_traits>
#include <vulkan/vulkan.hpp>

#include "Constants.h"
#include "BufferManager.h"
#include "Instance.h"

namespace Instance {

// TODO: I think that this class should probably go away at this point, and I should just use the Descriptor::Manager.

template <class TBase, class TDerived>
class Manager : public Buffer::Manager::Base<TBase, TDerived, std::shared_ptr> {
    static_assert(std::is_base_of<Instance::Base, TBase>::value, "TBase must be a descendant of Instance::Base");
    using TManager = Buffer::Manager::Base<TBase, TDerived, std::shared_ptr>;

   public:
    Manager(const std::string&& name, const vk::DeviceSize&& maxSize, const bool&& keepMapped = true,
            const vk::BufferUsageFlagBits&& usage = vk::BufferUsageFlagBits::eVertexBuffer)
        : TManager(
              //
              std::forward<const std::string>(name),        //
              std::forward<const vk::DeviceSize>(maxSize),  //
              std::forward<const bool>(keepMapped),         //
              std::forward<const vk::BufferUsageFlagBits>(usage),
              // This used to not have the vk::MemoryPropertyFlagBits::eHostCoherent set. It was needed to work
              // with the macOS build. TBH I am not sure which would be faster - using the coherent bit,
              // or flusing and invalidating. I just set the bit because I didn't know how to test it atm.
              (vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal |
               vk::MemoryPropertyFlagBits::eHostCoherent)) {}
    virtual ~Manager() = default;

    const DESCRIPTOR DESCRIPTOR_TYPE;

   private:
    // TODO: Should this, and in turn the entire class, be derived from Descriptor::Manager?
    void setInfo(Buffer::Info& info) override { info.bufferInfo.offset = 0; }
};

}  // namespace Instance

#endif  // !INSTANCE_MANAGER_H