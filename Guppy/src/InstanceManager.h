#ifndef INSTANCE_MANAGER_H
#define INSTANCE_MANAGER_H

#include <memory>
#include <string>
#include <type_traits>
#include <vulkan/vulkan.h>

#include "BufferManager.h"
#include "Instance.h"

namespace Instance {

template <class TBase, class TDerived>
class Manager : public Buffer::Manager::Base<TBase, TDerived, std::shared_ptr> {
    static_assert(std::is_base_of<Instance::Base, TBase>::value, "TBase must be a descendant of Instance::Base");

   public:
    Manager(const std::string&& name, const VkDeviceSize&& maxSize)
        : Buffer::Manager::Base<TBase, TDerived, std::shared_ptr>{
              std::forward<const std::string>(name),
              std::forward<const VkDeviceSize>(maxSize),
              true,
              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
              static_cast<VkMemoryPropertyFlagBits>(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
          } {}
};

}  // namespace Instance

#endif  // !INSTANCE_MANAGER_H