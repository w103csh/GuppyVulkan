/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef DESCRIPTOR_CONSTANTS_H
#define DESCRIPTOR_CONSTANTS_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Types.h"
#include "UniformConstants.h"

enum class PASS : uint32_t;
// clang-format off
namespace Buffer { struct Info; }
// clang-format on

enum class DESCRIPTOR_SET {
    // DEFAULT
    UNIFORM_DEFAULT,
    UNIFORM_CAMERA_ONLY,
    UNIFORM_DEFCAM_DEFMAT_MX4,
    UNIFORM_CAM_MATOBJ3D,
    UNIFORM_OBJ3D,
    SAMPLER_DEFAULT,
    SAMPLER_CUBE_DEFAULT,
    PROJECTOR_DEFAULT,
    CAMERA_CUBE_MAP,
    // PBR
    UNIFORM_PBR,
    // PARALLAX
    UNIFORM_PARALLAX,
    SAMPLER_PARALLAX,
    // SCREEN SPACE
    UNIFORM_SCREEN_SPACE_DEFAULT,
    SAMPLER_SCREEN_SPACE_DEFAULT,
    SAMPLER_SCREEN_SPACE_HDR_LOG_BLIT,
    SAMPLER_SCREEN_SPACE_BLUR_A,
    SAMPLER_SCREEN_SPACE_BLUR_B,
    STORAGE_SCREEN_SPACE_COMPUTE_POST_PROCESS,
    STORAGE_IMAGE_SCREEN_SPACE_COMPUTE_DEFAULT,
    // MISCELLANEOUS
    SWAPCHAIN_IMAGE,
    // DEFERRED
    UNIFORM_DEFERRED_MRT,
    UNIFORM_DEFERRED_SSAO,
    UNIFORM_DEFERRED_COMBINE,
    SAMPLER_DEFERRED,
    SAMPLER_DEFERRED_SSAO_RANDOM,
    // SHADOW
    SHADOW_CUBE_UNIFORM_ONLY,
    SHADOW_CUBE_ALL,
    SAMPLER_SHADOW,
    SAMPLER_SHADOW_OFFSET,
    // TESSELLATION
    UNIFORM_TESSELLATION_DEFAULT,
    // GEOMETRY
    UNIFORM_GEOMETRY_DEFAULT,
    // PARTICLE
    UNIFORM_PRTCL_WAVE,
    UNIFORM_PRTCL_FOUNTAIN,
    PRTCL_EULER,
    PRTCL_ATTRACTOR,
    PRTCL_CLOTH,
    PRTCL_CLOTH_NORM,
    // WATER
    HFF,
    HFF_DEF,
    // Add new to DESCRIPTOR_SET_ALL in code file.
};

namespace Descriptor {

struct BindingInfo {
    DESCRIPTOR descType;
    // This can either be the name of a texture, or an offset number into a
    // texture with multiple samplers.
    std::string_view textureId = "";
    uint8_t uniqueDataSets = 1;
};

// key:     { binding, arrayElement }
typedef std::pair<uint32_t, uint32_t> bindingMapKey;
typedef std::pair<const bindingMapKey, BindingInfo> bindingMapKeyValue;
typedef std::map<bindingMapKey, BindingInfo> bindingMap;

struct CreateInfo {
    DESCRIPTOR type;
    Uniform::offsets offsets;
    std::string_view textureId = "";
};

// Wrapper class for multimap. Uniforms are unique to desriptor sets, but
// samplers can have multiples of the same type. This wraps the insert to
// enforce this.
struct OffsetsMap {
   public:
    using Type = std::multimap<DESCRIPTOR, Uniform::offsets>;
    OffsetsMap(const Type&& map = {}) : offsets_(map) {}
    void insert(const DESCRIPTOR& key, const Uniform::offsets& value);
    const auto& map() const { return offsets_; }

   private:
    Type offsets_;
};

// VISITORS
// clang-format off
struct GetTypeString {
    template <typename T> std::string operator()(const T&) const { assert(false); exit(EXIT_FAILURE); }
    std::string operator()(const UNIFORM& type)                 const { return std::string("UNIFORM "               + std::to_string(static_cast<int>(type))); }
    std::string operator()(const UNIFORM_DYNAMIC& type)         const { return std::string("UNIFORM_DYNAMIC "       + std::to_string(static_cast<int>(type))); }
    std::string operator()(const COMBINED_SAMPLER& type)        const { return std::string("COMBINED_SAMPLER "      + std::to_string(static_cast<int>(type))); }
    std::string operator()(const STORAGE_IMAGE& type)           const { return std::string("STORAGE_IMAGE "         + std::to_string(static_cast<int>(type))); }
    std::string operator()(const STORAGE_BUFFER& type)          const { return std::string("STORAGE_BUFFER "        + std::to_string(static_cast<int>(type))); }
    std::string operator()(const STORAGE_BUFFER_DYNAMIC& type)  const { return std::string("STORAGE_BUFFER_DYNAMIC "+ std::to_string(static_cast<int>(type))); }
    std::string operator()(const INPUT_ATTACHMENT& type)        const { return std::string("INPUT_ATTACHMENT "      + std::to_string(static_cast<int>(type))); }
};
struct GetVkBufferUsage {
    template <typename T> VkBufferUsageFlagBits operator()(const T&) const { assert(false); exit(EXIT_FAILURE); }
    VkBufferUsageFlagBits operator()(const UNIFORM&                     ) const { return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; }
    VkBufferUsageFlagBits operator()(const UNIFORM_DYNAMIC&             ) const { return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT; }
    VkBufferUsageFlagBits operator()(const STORAGE_IMAGE&               ) const { return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; }
    VkBufferUsageFlagBits operator()(const STORAGE_BUFFER&              ) const { return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT; }
    VkBufferUsageFlagBits operator()(const STORAGE_BUFFER_DYNAMIC& type ) const {
        switch (type) {
            case STORAGE_BUFFER_DYNAMIC::VERTEX: return VkBufferUsageFlagBits(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
            default: return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }
    }
};
struct GetVkDescriptorType {
    template <typename T> VkDescriptorType operator()(const T&) const { assert(false); exit(EXIT_FAILURE); }
    VkDescriptorType operator()(const UNIFORM&                  ) const { return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; }
    VkDescriptorType operator()(const UNIFORM_DYNAMIC&          ) const { return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC; }
    VkDescriptorType operator()(const COMBINED_SAMPLER&         ) const { return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; }
    VkDescriptorType operator()(const STORAGE_IMAGE&            ) const { return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; }
    VkDescriptorType operator()(const STORAGE_BUFFER&           ) const { return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; }
    VkDescriptorType operator()(const STORAGE_BUFFER_DYNAMIC&   ) const { return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC; }
    VkDescriptorType operator()(const INPUT_ATTACHMENT&         ) const { return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT; }
};
struct GetVkMemoryProperty {
    template <typename T> VkMemoryPropertyFlagBits operator()(const T& type) const {
        return VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
    VkMemoryPropertyFlagBits operator()(const STORAGE_BUFFER_DYNAMIC& type) const {
        switch (type) {
            case STORAGE_BUFFER_DYNAMIC::VERTEX: return VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            default: return VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        }
    }
};
struct HasOffsets {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const UNIFORM&) const { return true; }
    bool operator()(const STORAGE_BUFFER& type) const { return true; }
    //bool operator()(const STORAGE_IMAGE& type) const { return true; }
};
struct IsImage {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const COMBINED_SAMPLER&) const { return true; }
    bool operator()(const STORAGE_IMAGE&) const { return true; }
};
struct IsPipelineImage {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const COMBINED_SAMPLER& type) const { return type == COMBINED_SAMPLER::PIPELINE; }
    bool operator()(const STORAGE_IMAGE& type) const { return type == STORAGE_IMAGE::PIPELINE; }
    bool operator()(const INPUT_ATTACHMENT&) const { return true; }
};
struct HasPerFramebufferData {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const STORAGE_BUFFER& type) const {
        switch (type) {
            case STORAGE_BUFFER::POST_PROCESS:
                return true;
            default:
                return false;
        }
    }
    bool operator()(const UNIFORM& type) const {
        switch (type) {
            case UNIFORM::CAMERA_PERSPECTIVE_DEFAULT:
            case UNIFORM::CAMERA_PERSPECTIVE_CUBE_MAP:
            case UNIFORM::LIGHT_DIRECTIONAL_DEFAULT:
            case UNIFORM::LIGHT_POSITIONAL_DEFAULT:
            case UNIFORM::LIGHT_POSITIONAL_PBR:
            case UNIFORM::LIGHT_POSITIONAL_SHADOW:
            case UNIFORM::LIGHT_CUBE_SHADOW:
            case UNIFORM::LIGHT_SPOT_DEFAULT:
            case UNIFORM::PRTCL_WAVE:
                return true;
            default:
                return false;
        }
    }
    bool operator()(const UNIFORM_DYNAMIC& type) const {
        switch (type) {
            case UNIFORM_DYNAMIC::PRTCL_FOUNTAIN:
            case UNIFORM_DYNAMIC::PRTCL_ATTRACTOR:
            case UNIFORM_DYNAMIC::PRTCL_CLOTH:
            case UNIFORM_DYNAMIC::MATRIX_4:
            case UNIFORM_DYNAMIC::HFF:
                return true;
            default:
                return false;
        }
    }
};
//struct GetTextureImageInitialLayout {
//    template <typename T> VkImageLayout operator()(const T&) const { assert(false); exit(EXIT_FAILURE); }
//    VkImageLayout operator()(const COMBINED_SAMPLER&) const { return VK_IMAGE_LAYOUT_UNDEFINED; }
//    VkImageLayout operator()(const STORAGE_IMAGE&) const { return VK_IMAGE_LAYOUT_GENERAL; }
//};
struct GetTextureImageLayout {
    template <typename T> VkImageLayout operator()(const T&) const { assert(false); exit(EXIT_FAILURE); }
    VkImageLayout operator()(const COMBINED_SAMPLER& type) const {
        switch (type) {
            case COMBINED_SAMPLER::PIPELINE_DEPTH: return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            default: return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }
    VkImageLayout operator()(const STORAGE_IMAGE&) const { return VK_IMAGE_LAYOUT_GENERAL; }
    VkImageLayout operator()(const INPUT_ATTACHMENT&) const { return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; }
};
struct IsDynamic {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const UNIFORM_DYNAMIC&) const { return true; }
    bool operator()(const STORAGE_BUFFER_DYNAMIC&) const { return true; }
};
// COMBINED SAMPLER
struct IsCombinedSampler {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const COMBINED_SAMPLER&) const { return true; }
};
struct GetCombinedSampler {
    template <typename T> COMBINED_SAMPLER operator()(const T&) const { return COMBINED_SAMPLER::DONT_CARE; }
    COMBINED_SAMPLER operator()(const COMBINED_SAMPLER& type) const { return type; }
};
struct IsCombinedSamplerMaterial {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const COMBINED_SAMPLER& type) const { return type == COMBINED_SAMPLER::MATERIAL; }
};
struct IsCombinedSamplerPipeline {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const COMBINED_SAMPLER& type) const { return type == COMBINED_SAMPLER::PIPELINE || type == COMBINED_SAMPLER::PIPELINE_DEPTH; }
};
// STORAGE BUFFER
struct IsStorageBuffer {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const STORAGE_BUFFER&) const { return true; }
};
struct GetStorageBuffer {
    template <typename T> STORAGE_BUFFER operator()(const T&) const { return STORAGE_BUFFER::DONT_CARE; }
    STORAGE_BUFFER operator()(const STORAGE_BUFFER& type) const { return type; }
};
// STORAGE BUFFER DYNAMIC
struct IsStorageBufferDynamic {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const STORAGE_BUFFER_DYNAMIC&) const { return true; }
};
struct GetStorageBufferDynamic {
    template <typename T> STORAGE_BUFFER_DYNAMIC operator()(const T&) const { return STORAGE_BUFFER_DYNAMIC::DONT_CARE; }
    STORAGE_BUFFER_DYNAMIC operator()(const STORAGE_BUFFER_DYNAMIC& type) const { return type; }
};
// STORAGE IMAGE
struct IsStorageImage {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const STORAGE_IMAGE&) const { return true; }
};
struct GetStorageImage {
    template <typename T> STORAGE_IMAGE operator()(const T&) const { return STORAGE_IMAGE::DONT_CARE; }
    STORAGE_IMAGE operator()(const STORAGE_IMAGE& type) const { return type; }
};
struct IsSwapchainStorageImage {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const STORAGE_IMAGE& type) const { return type == STORAGE_IMAGE::SWAPCHAIN; }
};
// UNIFORM
struct IsUniform {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const UNIFORM&) const { return true; }
};
struct GetUniform {
    template <typename T> UNIFORM operator()(const T&) const { return UNIFORM::DONT_CARE; }
    UNIFORM operator()(const UNIFORM& type) const { return type; }
};
// UNIFORM DYNAMIC
struct IsUniformDynamic {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const UNIFORM_DYNAMIC&) const { return true; }
};
struct GetUniformDynamic {
    template <typename T> UNIFORM_DYNAMIC operator()(const T&) const { return UNIFORM_DYNAMIC::DONT_CARE; }
    UNIFORM_DYNAMIC operator()(const UNIFORM_DYNAMIC& type) const { return type; }
};
struct IsMaterial {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const UNIFORM_DYNAMIC& type) const { return
        type == UNIFORM_DYNAMIC::MATERIAL_DEFAULT ||
        type == UNIFORM_DYNAMIC::MATERIAL_OBJ3D ||
        type == UNIFORM_DYNAMIC::MATERIAL_PBR;
    }
};
// INPUT ATTACHMENT
struct IsInputAttachment {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const INPUT_ATTACHMENT&) const { return true; }
};
struct GetInputAttachment {
    template <typename T> INPUT_ATTACHMENT operator()(const T&) const { return INPUT_ATTACHMENT::DONT_CARE; }
    INPUT_ATTACHMENT operator()(const INPUT_ATTACHMENT& type) const { return type; }
};
// clang-format on

namespace Set {

constexpr std::string_view MACRO_ID_PREFIX = "_DS_";
extern const std::set<DESCRIPTOR_SET> ALL;

struct CreateInfo {
    DESCRIPTOR_SET type;
    std::string macroName;
    Descriptor::bindingMap bindingMap;
};

struct ResourceInfo {
    // TODO: Should always be the same as the BindingInfo descCount. Maybe make it a pointer?
    uint32_t uniqueDataSets = 0;
    uint32_t descCount = 0;
    std::vector<ImageInfo> imageInfos;
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    VkBufferView texelBufferView = VK_NULL_HANDLE;
    void setWriteInfo(const uint32_t index, VkWriteDescriptorSet& write) const;
    void reset();
};

using resourceInfoMap = std::map<std::pair<uint32_t, DESCRIPTOR>, ResourceInfo>;
using resourceInfoMapSetsPair = std::pair<std::shared_ptr<resourceInfoMap>, std::vector<VkDescriptorSet>>;

struct BindData {
    uint32_t firstSet;
    std::vector<std::vector<VkDescriptorSet>> descriptorSets;
    std::map<DESCRIPTOR_SET, std::shared_ptr<resourceInfoMap>> setResourceInfoMap;
    std::vector<uint32_t> dynamicOffsets;
};

using bindDataMap = std::map<std::set<PASS>, Descriptor::Set::BindData>;

// Data for creation and merging of descriptor set layouts.
struct Resource {
    OffsetsMap offsets;
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    std::set<PASS> passTypes;
    std::set<PIPELINE> pipelineTypes;
    VkShaderStageFlags stages = 0;
};

struct Helper {
    std::set<PASS> passTypes1;                   // Pass types the helper was made for.
    const Descriptor::Set::Resource* pResource;  // Set resource
    uint32_t resourceOffset;                     // Set resource offset
    DESCRIPTOR_SET setType;                      // Set type
    std::set<PASS> passTypes2;  // Pass types the resource can be used for (Can be different from resource pointer's type
                                // set because of culling. Saved for use in potential culling of shader modules).
};
using helpers = std::vector<Helper>;
using resourceHelpers = std::vector<helpers>;

// Shader text replace info
using textReplaceTuple = std::tuple<std::string, uint8_t, OffsetsMap, std::set<PASS>>;
using textReplaceTuples = std::vector<textReplaceTuple>;

namespace Default {
extern const CreateInfo UNIFORM_CREATE_INFO;
extern const CreateInfo UNIFORM_CAMERA_ONLY_CREATE_INFO;
extern const CreateInfo UNIFORM_DEFCAM_DEFMAT_MX4;
extern const CreateInfo UNIFORM_CAM_MATOBJ3D_CREATE_INFO;
extern const CreateInfo UNIFORM_OBJ3D_CREATE_INFO;
extern const CreateInfo SAMPLER_CREATE_INFO;
extern const CreateInfo CUBE_SAMPLER_CREATE_INFO;
extern const CreateInfo PROJECTOR_SAMPLER_CREATE_INFO;
extern const CreateInfo CAM_CUBE_CREATE_INFO;
}  // namespace Default

}  // namespace Set

std::string GetPerframeBufferWarning(const DESCRIPTOR descType, const Buffer::Info& buffInfo,
                                     const Set::ResourceInfo& resInfo, bool doAssert = true);

}  // namespace Descriptor

#endif  // !DESCRIPTOR_CONSTANTS_H
