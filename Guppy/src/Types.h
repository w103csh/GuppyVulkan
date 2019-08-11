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
enum class PASS : uint32_t;

using FlagBits = uint32_t;
// Type for the vertex buffer indices (this is also used in vkCmdBindIndexBuffer)
using VB_INDEX_TYPE = uint32_t;

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

template <uint8_t size>
struct SubmitResource {
    uint32_t waitSemaphoreCount = 0;
    std::array<VkSemaphore, size> waitSemaphores = {};
    std::array<VkPipelineStageFlags, size> waitDstStageMasks = {};
    uint32_t commandBufferCount = 0;
    std::array<VkCommandBuffer, size> commandBuffers = {};
    uint32_t signalSemaphoreCount = 0;
    std::array<VkSemaphore, size> signalSemaphores = {};
    // Set below for stages the signal semaphores should wait on.
    std::array<VkPipelineStageFlags, size> signalSrcStageMasks = {};
    QUEUE queueType;
    void resetCount() {
        waitSemaphoreCount = 0;
        commandBufferCount = 0;
        signalSemaphoreCount = 0;
    }
};

struct BarrierResource {
    std::vector<VkMemoryBarrier> glblBarriers;
    std::vector<VkBufferMemoryBarrier> buffBarriers;
    std::vector<VkImageMemoryBarrier> imgBarriers;
    inline void reset() {
        glblBarriers.clear();
        buffBarriers.clear();
        imgBarriers.clear();
    }
};

using DESCRIPTOR = std::variant<UNIFORM, UNIFORM_DYNAMIC, COMBINED_SAMPLER, STORAGE_IMAGE, STORAGE_BUFFER, INPUT_ATTACHMENT>;

// Why is this a multiset?
using pipelinePassSet = std::multiset<std::pair<PIPELINE, PASS>>;

struct ImageInfo {
    std::map<uint32_t, VkDescriptorImageInfo> descInfoMap;
    VkImage image = VK_NULL_HANDLE;
};

template <typename T>
struct insertion_ordered_unique_list {
   public:
    void insert(T newElement) {
        for (const auto &element : list_)
            if (element == newElement) return;
        list_.emplace_back(std::move(newElement));
    }
    constexpr void clear() { list_.clear(); }
    constexpr const auto &getList() const { return list_; }

   private:
    std::vector<T> list_;
};

#endif  //! TYPE_H
