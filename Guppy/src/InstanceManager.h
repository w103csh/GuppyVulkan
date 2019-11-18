#ifndef INSTANCE_MANAGER_H
#define INSTANCE_MANAGER_H

#include <memory>
#include <string>
#include <type_traits>
#include <vulkan/vulkan.h>

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
    Manager(const std::string&& name, const VkDeviceSize&& maxSize, const bool&& keepMapped = true,
            const VkBufferUsageFlagBits&& usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
        : TManager(
              //
              std::forward<const std::string>(name),      //
              std::forward<const VkDeviceSize>(maxSize),  //
              std::forward<const bool>(keepMapped),       //
              std::forward<const VkBufferUsageFlagBits>(usage),
              // This used to not have the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT set. It was needed to work
              // with the macOS build. TBH I am not sure which would be faster - using the coherent bit,
              // or flusing and invalidating. I just set the bit because I didn't know how to test it atm.
              static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {}
    virtual ~Manager() = default;

    const DESCRIPTOR DESCRIPTOR_TYPE;

   private:
    // TODO: Should this, and in turn the entire class, be derived from Descriptor::Manager?
    void setInfo(Buffer::Info& info) override { info.bufferInfo.offset = 0; }
};

}  // namespace Instance

#endif  // !INSTANCE_MANAGER_H