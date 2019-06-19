#ifndef TYPES_H
#define TYPES_H

#include <glm/glm.hpp>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <variant>
#include <vulkan/vulkan.h>

#include "Enum.h"

enum class PIPELINE : uint32_t;
enum class RENDER_PASS : uint32_t;

using FlagBits = uint32_t;
// Type for the vertex buffer indices (this is also used in vkCmdBindIndexBuffer)
using VB_INDEX_TYPE = uint32_t;
using SCENE_INDEX_TYPE = uint8_t;

// TODO: make a data structure so this can be const in the handlers.
template <typename TEnum, typename TType>
using enumPointerTypeMap = std::map<TEnum, std::unique_ptr<TType>>;

class NonCopyable {
   public:
    NonCopyable() = default;
    ~NonCopyable() = default;

   private:
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
    NonCopyable(NonCopyable &&) = delete;
    NonCopyable &operator=(NonCopyable &&) = delete;
};

struct BufferResource {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkMemoryRequirements memoryRequirements{};
};

struct ImageResource {
    VkFormat format{};
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
};

// template <typename T>
// struct SharedStruct : std::enable_shared_from_this<T> {
//    const std::shared_ptr<const T> getPtr() const { return shared_from_this(); }
//};

struct Ray {
    glm::vec3 e;              // start
    glm::vec3 d;              // end
    glm::vec3 direction;      // non-normalized direction vector from "e" to "d"
    glm::vec3 directionUnit;  // normalized "direction"
};

struct DescriptorBufferResources {
    uint32_t count;
    VkDeviceSize size;
    VkDescriptorBufferInfo info;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

struct DescriptorResource {
    VkDescriptorBufferInfo info = {};
    VkDeviceMemory memory = VK_NULL_HANDLE;
};

template <typename TEnum>
struct hash_pair_enum_size_t {
    inline std::size_t operator()(const std::pair<TEnum, size_t> &p) const {
        std::hash<int> int_hasher;
        std::hash<size_t> size_t_hasher;
        return int_hasher(static_cast<int>(p.first)) ^ size_t_hasher(p.second);
    }
};

using DESCRIPTOR = std::variant<UNIFORM, UNIFORM_DYNAMIC, COMBINED_SAMPLER>;

// Why is this a multiset?
using pipelinePassSet = std::multiset<std::pair<PIPELINE, RENDER_PASS>>;

#endif  //! TYPE_H
