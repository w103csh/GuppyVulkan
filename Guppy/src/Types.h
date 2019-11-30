/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef TYPES_H
#define TYPES_H

#include <glm/glm.hpp>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <variant>
#include <vector>
#include <vulkan/vulkan.h>

#include "Enum.h"

enum class PASS : uint32_t;
enum class GRAPHICS : uint32_t;
enum class COMPUTE : uint32_t;

using PIPELINE = std::variant<GRAPHICS, COMPUTE>;

using FlagBits = uint32_t;
// Type for the vertex buffer indices (this is also used in vkCmdBindIndexBuffer)

// INDEX TYPE
using VB_INDEX_TYPE = uint32_t;
// This value is dependent on the type set in vkCmdBindIndexBuffer. There is a list of valid restart values in the
// specification.
constexpr VB_INDEX_TYPE VB_INDEX_PRIMITIVE_RESTART = 0xFFFFFFFF;
constexpr VB_INDEX_TYPE BAD_VB_INDEX = VB_INDEX_PRIMITIVE_RESTART - 1;

// TODO: make a data structure so this can be const in the handlers.
template <typename TEnum, typename TType>
using enumPointerTypeMap = std::map<TEnum, std::unique_ptr<TType>>;

namespace Pipeline {
struct CreateInfoResources {
    // BLENDING
    std::vector<VkPipelineColorBlendAttachmentState> blendAttachmentStates;
    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {};
    // DYNAMIC
    VkDynamicState dynamicStates[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
    // INPUT ASSEMBLY
    std::vector<VkVertexInputBindingDescription> bindDescs;
    std::vector<VkVertexInputAttributeDescription> attrDescs;
    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {};
    // std::vector<VkVertexInputBindingDivisorDescriptionEXT> vertexInputBindDivDescs;
    // VkPipelineVertexInputDivisorStateCreateInfoEXT vertexInputDivInfo = {};
    // FIXED FUNCTION
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo = {};
    VkPipelineTessellationStateCreateInfo tessellationStateInfo = {};
    VkPipelineViewportStateCreateInfo viewportStateInfo = {};
    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo = {};
    VkPipelineMultisampleStateCreateInfo multisampleStateInfo = {};
    VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = {};
    // SHADER
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;
    std::vector<std::vector<VkSpecializationMapEntry>> specializationMapEntries;
    std::vector<VkSpecializationInfo> specializationInfo;
};
}  // namespace Pipeline

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

struct LoadingResource {
    LoadingResource()
        :  //
          shouldWait(false),
          graphicsCmd(VK_NULL_HANDLE),
          transferCmd(VK_NULL_HANDLE),
          semaphore(VK_NULL_HANDLE){};
    bool shouldWait;
    VkCommandBuffer graphicsCmd, transferCmd;
    std::vector<BufferResource> stgResources;
    std::vector<VkFence> fences;
    VkSemaphore semaphore;
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

using DESCRIPTOR = std::variant<  //
    UNIFORM,                      //
    UNIFORM_DYNAMIC,              //
    COMBINED_SAMPLER,             //
    STORAGE_IMAGE,                //
    STORAGE_BUFFER,               //
    STORAGE_BUFFER_DYNAMIC,       //
    INPUT_ATTACHMENT              //
    >;

// Why is this a multiset?
using pipelinePassSet = std::multiset<std::pair<PIPELINE, PASS>>;

struct ImageInfo {
    std::map<uint32_t, VkDescriptorImageInfo> descInfoMap;
    VkImage image = VK_NULL_HANDLE;
};

// This is just quick and dirty. Not fully featured in any way.
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

// This is just quick and dirty. Not fully featured in any way.
template <typename TKey, typename TValue>
struct insertion_ordered_unique_keyvalue_list {
   public:
    void insert(TKey key, TValue value) {
        auto it = keyOffsetMap_.find(key);
        if (it == keyOffsetMap_.end()) {
            auto insertPair = keyOffsetMap_.insert({key, static_cast<uint32_t>(valueList_.size())});
            assert(insertPair.second);
            valueList_.push_back(std::move(value));
            keyList_.push_back(key);
        } else {
            valueList_.at(it->second) = std::move(value);
            keyList_.at(it->second) = std::move(key);
        }
        assert(keyOffsetMap_.size() == valueList_.size() && valueList_.size() == keyList_.size());
    }
    constexpr void clear() {
        keyOffsetMap_.clear();
        valueList_.clear();
        keyList_.clear();
    }
    constexpr auto getOffset(const TKey key) const {
        auto it = keyOffsetMap_.find(key);  //
        return it == keyOffsetMap_.end() ? -1 : it->second;
    }
    constexpr const auto &getValue(const TKey key) const { return valueList_.at(keyOffsetMap_.at(key)); }
    constexpr bool hasKey(const TKey key) const { return keyOffsetMap_.count(key) > 0; }
    constexpr const auto &getValue(const uint32_t offset) const { return valueList_.at(offset); }
    constexpr const auto &getKey(const uint32_t offset) const { return keyList_.at(offset); }
    constexpr const auto &getKeyOffsetMap() const { return keyOffsetMap_; }
    constexpr const auto &getValues() const { return valueList_; }
    constexpr const auto &getKeys() const { return keyList_; }
    constexpr auto size() const {
        assert(keyOffsetMap_.size() == valueList_.size() && valueList_.size() == keyList_.size());
        return valueList_.size();
    }

   private:
    std::map<TKey, uint32_t> keyOffsetMap_;
    std::vector<TValue> valueList_;
    std::vector<TKey> keyList_;
};

#endif  //! TYPE_H
