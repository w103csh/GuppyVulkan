#ifndef INSTANCE_MANAGER_H
#define INSTANCE_MANAGER_H

#include <glm/glm.hpp>
#include <utility>
#include <vulkan/vulkan.h>

#include "BufferManager.h"
#include "Instance.h"

namespace Instance {

template <class TDerived>
class Manager : public Buffer::Manager::Base<Instance::Base, TDerived, std::shared_ptr> {
   public:
    Manager(const std::string&& name, const VkDeviceSize&& maxSize)
        : Buffer::Manager::Base<Instance::Base, TDerived, std::shared_ptr>{
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