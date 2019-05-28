#ifndef TYPES_H
#define TYPES_H

#include <glm/glm.hpp>
#include <functional>
#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

typedef uint32_t FlagBits;
// Type for the vertex buffer indices (this is also used in vkCmdBindIndexBuffer)
typedef uint32_t VB_INDEX_TYPE;
typedef uint8_t SCENE_INDEX_TYPE;

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

// TODO: check other places like LoadingResourceHandler to see if this
// could be used.
struct SubmitResource {
    std::vector<VkSemaphore> waitSemaphores;
    std::vector<VkPipelineStageFlags> waitDstStageMasks;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> signalSemaphores;
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

#endif  //! TYPE_H
