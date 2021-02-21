/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
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
#include <vulkan/vulkan.hpp>

enum class QUEUE;

enum class MODEL_FILE_TYPE {
    //
    UNKNOWN = 0,
    OBJ,
};

using FlagBits = uint32_t;
// Type for the vertex buffer indices (this is also used in bindIndexBuffer)

// INDEX TYPE
using IndexBufferType = uint32_t;
// This value is dependent on the type set in bindIndexBuffer. There is a list of valid restart values in the
// specification.
constexpr IndexBufferType VB_INDEX_PRIMITIVE_RESTART = 0xFFFFFFFF;
constexpr IndexBufferType BAD_VB_INDEX = VB_INDEX_PRIMITIVE_RESTART - 1;

// TODO: make a data structure so this can be const in the handlers.
template <typename TEnum, typename TType>
using enumPointerTypeMap = std::map<TEnum, std::unique_ptr<TType>>;

// Structure for comparing char arrays
struct less_str {
    bool operator()(char const *a, char const *b) const { return std::strcmp(a, b) < 0; }
};

namespace Pipeline {
struct CreateInfoResources {
    // BLENDING
    std::vector<vk::PipelineColorBlendAttachmentState> blendAttachmentStates;
    vk::PipelineColorBlendStateCreateInfo colorBlendStateInfo;
    // DYNAMIC
    std::vector<vk::DynamicState> dynamicStates;
    vk::PipelineDynamicStateCreateInfo dynamicStateInfo;
    // INPUT ASSEMBLY
    std::vector<vk::VertexInputBindingDescription> bindDescs;
    std::vector<vk::VertexInputAttributeDescription> attrDescs;
    vk::PipelineVertexInputStateCreateInfo vertexInputStateInfo;
    // std::vector<vk::VertexInputBindingDivisorDescriptionEXT> vertexInputBindDivDescs;
    // vk::PipelineVertexInputDivisorStateCreateInfoEXT vertexInputDivInfo;
    // FIXED FUNCTION
    vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo;
    bool useTessellationInfo = false;
    vk::PipelineTessellationStateCreateInfo tessellationStateInfo;
    vk::PipelineViewportStateCreateInfo viewportStateInfo;
    vk::PipelineRasterizationStateCreateInfo rasterizationStateInfo;
    vk::PipelineMultisampleStateCreateInfo multisampleStateInfo;
    vk::PipelineDepthStencilStateCreateInfo depthStencilStateInfo;
    // SHADER
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStageInfos;
    std::vector<std::vector<vk::SpecializationMapEntry>> specializationMapEntries;
    std::vector<vk::SpecializationInfo> specializationInfo;
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
    vk::Buffer buffer;
    vk::DeviceMemory memory;
    vk::MemoryRequirements memoryRequirements;
};

struct ImageResource {
    vk::Format format;
    vk::Image image;
    vk::DeviceMemory memory;
    vk::ImageView view;
};

struct LoadingResource {
    bool shouldWait = false;
    vk::CommandBuffer graphicsCmd, transferCmd;
    std::vector<BufferResource> stgResources;
    std::vector<vk::Fence> fences;
    vk::Semaphore semaphore;
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
    vk::DeviceSize size;
    vk::DescriptorBufferInfo info;
    vk::Buffer buffer;
    vk::DeviceMemory memory;
};

struct DescriptorResource {
    vk::DescriptorBufferInfo info;
    vk::DeviceMemory memory;
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
    std::array<vk::Semaphore, size> waitSemaphores = {};
    std::array<vk::PipelineStageFlags, size> waitDstStageMasks = {};
    uint32_t commandBufferCount = 0;
    std::array<vk::CommandBuffer, size> commandBuffers = {};
    uint32_t signalSemaphoreCount = 0;
    std::array<vk::Semaphore, size> signalSemaphores = {};
    // Set below for stages the signal semaphores should wait on.
    std::array<vk::PipelineStageFlags, size> signalSrcStageMasks = {};
    QUEUE queueType;
    void resetCount() {
        waitSemaphoreCount = 0;
        commandBufferCount = 0;
        signalSemaphoreCount = 0;
    }
};

struct BarrierResource {
    std::vector<vk::MemoryBarrier> glblBarriers;
    std::vector<vk::BufferMemoryBarrier> buffBarriers;
    std::vector<vk::ImageMemoryBarrier> imgBarriers;
    inline void reset() {
        glblBarriers.clear();
        buffBarriers.clear();
        imgBarriers.clear();
    }
};

struct ImageInfo {
    std::map<uint32_t, vk::DescriptorImageInfo> descInfoMap;
    vk::Image image;
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

/*
 *  Just using a glm::vec4 to represent the equation for a plane:
 *      Ax + By + Cz + D = 0
 *  So for now,
 *      glm::vec4::x = A
 *      glm::vec4::y = B
 *      glm::vec4::z = C
 *      glm::vec4::w = D
 */
using plane = glm::vec4;

/*
 * Its hard to enforce the order below, but I should try to follow it.
 * [0] left
 * [1] right
 * [2] top
 * [3] bottom
 * [4] near
 * [5] far
 */
using frustumPlanes = std::array<plane, 6>;

using box = std::array<glm::vec3, 8>;

#endif  //! TYPE_H
