/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef DESCRIPTOR_CONSTANTS_H
#define DESCRIPTOR_CONSTANTS_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include <Common/Types.h>

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
    TESS_DEFAULT,
    TESS_PHONG,
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
    // FFT
    FFT_DEFAULT,
    // OCEAN
    OCEAN_DEFAULT,
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
    std::string operator()(const UNIFORM_TEXEL_BUFFER& type)    const { return std::string("UNIFORM_TEXEL_BUFFER "  + std::to_string(static_cast<int>(type))); }
    std::string operator()(const COMBINED_SAMPLER& type)        const { return std::string("COMBINED_SAMPLER "      + std::to_string(static_cast<int>(type))); }
    std::string operator()(const STORAGE_IMAGE& type)           const { return std::string("STORAGE_IMAGE "         + std::to_string(static_cast<int>(type))); }
    std::string operator()(const STORAGE_BUFFER& type)          const { return std::string("STORAGE_BUFFER "        + std::to_string(static_cast<int>(type))); }
    std::string operator()(const STORAGE_BUFFER_DYNAMIC& type)  const { return std::string("STORAGE_BUFFER_DYNAMIC "+ std::to_string(static_cast<int>(type))); }
    std::string operator()(const INPUT_ATTACHMENT& type)        const { return std::string("INPUT_ATTACHMENT "      + std::to_string(static_cast<int>(type))); }
};
struct GetVulkanBufferUsage {
    template <typename T> vk::BufferUsageFlags operator()(const T&) const { assert(false); exit(EXIT_FAILURE); }
    vk::BufferUsageFlags operator()(const UNIFORM&)                         const { return vk::BufferUsageFlagBits::eUniformBuffer; }
    vk::BufferUsageFlags operator()(const UNIFORM_DYNAMIC&)                 const { return vk::BufferUsageFlagBits::eUniformBuffer; }
    vk::BufferUsageFlags operator()(const UNIFORM_TEXEL_BUFFER&)            const { return vk::BufferUsageFlagBits::eUniformTexelBuffer; }
    vk::BufferUsageFlags operator()(const STORAGE_IMAGE&)                   const { return vk::BufferUsageFlagBits::eStorageBuffer; }
    vk::BufferUsageFlags operator()(const STORAGE_BUFFER&)                  const { return vk::BufferUsageFlagBits::eStorageBuffer; }
    vk::BufferUsageFlags operator()(const STORAGE_BUFFER_DYNAMIC& type )    const {
        switch (type) {
            case STORAGE_BUFFER_DYNAMIC::VERTEX: return vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer;
            default: return vk::BufferUsageFlagBits::eStorageBuffer;
        }
    }
};
struct GetVulkanDescriptorType {
    template <typename T> vk::DescriptorType operator()(const T&) const { assert(false); exit(EXIT_FAILURE); }
    vk::DescriptorType operator()(const UNIFORM&)                   const { return vk::DescriptorType::eUniformBuffer; }
    vk::DescriptorType operator()(const UNIFORM_DYNAMIC&)           const { return vk::DescriptorType::eUniformBufferDynamic; }
    vk::DescriptorType operator()(const UNIFORM_TEXEL_BUFFER&)      const { return vk::DescriptorType::eUniformTexelBuffer; }
    vk::DescriptorType operator()(const COMBINED_SAMPLER&)          const { return vk::DescriptorType::eCombinedImageSampler; }
    vk::DescriptorType operator()(const STORAGE_IMAGE&)             const { return vk::DescriptorType::eStorageImage; }
    vk::DescriptorType operator()(const STORAGE_BUFFER&)            const { return vk::DescriptorType::eStorageBuffer; }
    vk::DescriptorType operator()(const STORAGE_BUFFER_DYNAMIC&)    const { return vk::DescriptorType::eStorageBufferDynamic; }
    vk::DescriptorType operator()(const INPUT_ATTACHMENT&)          const { return vk::DescriptorType::eInputAttachment; }
};
struct GetVulkanMemoryProperty {
    template <typename T> vk::MemoryPropertyFlags operator()(const T& type) const {
        return vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;
    }
    vk::MemoryPropertyFlags operator()(const STORAGE_BUFFER_DYNAMIC& type) const {
        switch (type) {
            case STORAGE_BUFFER_DYNAMIC::VERTEX: return 
                (vk::MemoryPropertyFlagBits::eHostVisible
#if !(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
                | vk::MemoryPropertyFlagBits::eDeviceLocal
#endif
                | vk::MemoryPropertyFlagBits::eHostCoherent);
            default: return 
                (vk::MemoryPropertyFlagBits::eHostVisible
                | vk::MemoryPropertyFlagBits::eHostCoherent);
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
    bool operator()(const COMBINED_SAMPLER& type)       const { return type == COMBINED_SAMPLER::PIPELINE; }
    bool operator()(const STORAGE_IMAGE& type)          const { return type == STORAGE_IMAGE::PIPELINE; }
    bool operator()(const UNIFORM_TEXEL_BUFFER& type)   const { return type == UNIFORM_TEXEL_BUFFER::PIPELINE; }
    bool operator()(const INPUT_ATTACHMENT&)            const { return true; }
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
            case UNIFORM_DYNAMIC::OCEAN:
                return true;
            default:
                return false;
        }
    }
};
//struct GetTextureImageInitialLayout {
//    template <typename T> vk::ImageLayout operator()(const T&) const { assert(false); exit(EXIT_FAILURE); }
//    vk::ImageLayout operator()(const COMBINED_SAMPLER&) const { return vk::ImageLayout::eUndefined; }
//    vk::ImageLayout operator()(const STORAGE_IMAGE&) const { return vk::ImageLayout::eGeneral; }
//};
struct GetTextureImageLayout {
    template <typename T> vk::ImageLayout operator()(const T&) const { assert(false); exit(EXIT_FAILURE); }
    vk::ImageLayout operator()(const COMBINED_SAMPLER& type) const {
        switch (type) {
            case COMBINED_SAMPLER::PIPELINE_DEPTH:  return vk::ImageLayout::eDepthStencilReadOnlyOptimal;
            default:                                return vk::ImageLayout::eShaderReadOnlyOptimal;
        }
    }
    vk::ImageLayout operator()(const STORAGE_IMAGE&)    const { return vk::ImageLayout::eGeneral; }
    vk::ImageLayout operator()(const INPUT_ATTACHMENT&) const { return vk::ImageLayout::eShaderReadOnlyOptimal; }
};
struct IsDynamic {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const UNIFORM_DYNAMIC&)         const { return true; }
    bool operator()(const STORAGE_BUFFER_DYNAMIC&)  const { return true; }
};
// COMBINED SAMPLER
struct IsCombinedSampler {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const COMBINED_SAMPLER&)        const { return true; }
};
struct GetCombinedSampler {
    template <typename T> COMBINED_SAMPLER operator()(const T&) const { return COMBINED_SAMPLER::DONT_CARE; }
    COMBINED_SAMPLER operator()(const COMBINED_SAMPLER& type)   const { return type; }
};
struct IsCombinedSamplerMaterial {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const COMBINED_SAMPLER& type)   const { return type == COMBINED_SAMPLER::MATERIAL; }
};
struct IsCombinedSamplerPipeline {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const COMBINED_SAMPLER& type)   const { return type == COMBINED_SAMPLER::PIPELINE || type == COMBINED_SAMPLER::PIPELINE_DEPTH; }
};
// STORAGE BUFFER
struct IsStorageBuffer {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const STORAGE_BUFFER&)          const { return true; }
};
struct GetStorageBuffer {
    template <typename T> STORAGE_BUFFER operator()(const T&)   const { return STORAGE_BUFFER::DONT_CARE; }
    STORAGE_BUFFER operator()(const STORAGE_BUFFER& type)       const { return type; }
};
// STORAGE BUFFER DYNAMIC
struct IsStorageBufferDynamic {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const STORAGE_BUFFER_DYNAMIC&)  const { return true; }
};
struct GetStorageBufferDynamic {
    template <typename T> STORAGE_BUFFER_DYNAMIC operator()(const T&)       const { return STORAGE_BUFFER_DYNAMIC::DONT_CARE; }
    STORAGE_BUFFER_DYNAMIC operator()(const STORAGE_BUFFER_DYNAMIC& type)   const { return type; }
};
// STORAGE IMAGE
struct IsStorageImage {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const STORAGE_IMAGE&)           const { return true; }
};
struct GetStorageImage {
    template <typename T> STORAGE_IMAGE operator()(const T&)    const { return STORAGE_IMAGE::DONT_CARE; }
    STORAGE_IMAGE operator()(const STORAGE_IMAGE& type)         const { return type; }
};
struct IsSwapchainStorageImage {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const STORAGE_IMAGE& type)      const { return type == STORAGE_IMAGE::SWAPCHAIN; }
};
// UNIFORM
struct IsUniform {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const UNIFORM&)                 const { return true; }
};
struct GetUniform {
    template <typename T> UNIFORM operator()(const T&)  const { return UNIFORM::DONT_CARE; }
    UNIFORM operator()(const UNIFORM& type)             const { return type; }
};
// UNIFORM DYNAMIC
struct IsUniformDynamic {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const UNIFORM_DYNAMIC&)         const { return true; }
};
struct GetUniformDynamic {
    template <typename T> UNIFORM_DYNAMIC operator()(const T&)  const { return UNIFORM_DYNAMIC::DONT_CARE; }
    UNIFORM_DYNAMIC operator()(const UNIFORM_DYNAMIC& type)     const { return type; }
};
struct IsMaterial {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const UNIFORM_DYNAMIC& type) const { return
        type == UNIFORM_DYNAMIC::MATERIAL_DEFAULT ||
        type == UNIFORM_DYNAMIC::MATERIAL_OBJ3D ||
        type == UNIFORM_DYNAMIC::MATERIAL_PBR;
    }
};
// UNIFORM TEXEL BUFFER
struct IsUniformTexelBuffer {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const UNIFORM_TEXEL_BUFFER&)    const { return true; }
};
struct GetUniformTexelBuffer {
    template <typename T> UNIFORM_TEXEL_BUFFER operator()(const T&)     const { return UNIFORM_TEXEL_BUFFER::DONT_CARE; }
    UNIFORM_TEXEL_BUFFER operator()(const UNIFORM_TEXEL_BUFFER& type)   const { return type; }
};
// INPUT ATTACHMENT
struct IsInputAttachment {
    template <typename T> bool operator()(const T&) const { return false; }
    bool operator()(const INPUT_ATTACHMENT&)        const { return true; }
};
struct GetInputAttachment {
    template <typename T> INPUT_ATTACHMENT operator()(const T&) const { return INPUT_ATTACHMENT::DONT_CARE; }
    INPUT_ATTACHMENT operator()(const INPUT_ATTACHMENT& type)   const { return type; }
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
    std::vector<vk::DescriptorBufferInfo> bufferInfos;
    vk::BufferView texelBufferView;
    void setWriteInfo(const uint32_t index, vk::WriteDescriptorSet& write) const;
    void reset();
};

using resourceInfoMap = std::map<std::pair<uint32_t, DESCRIPTOR>, ResourceInfo>;
using resourceInfoMapSetsPair = std::pair<std::shared_ptr<resourceInfoMap>, std::vector<vk::DescriptorSet>>;

struct BindData {
    uint32_t firstSet;
    std::vector<std::vector<vk::DescriptorSet>> descriptorSets;
    std::map<DESCRIPTOR_SET, std::shared_ptr<resourceInfoMap>> setResourceInfoMap;
    std::vector<uint32_t> dynamicOffsets;
};

using bindDataMap = std::map<std::set<PASS>, Descriptor::Set::BindData>;

// Data for creation and merging of descriptor set layouts.
struct Resource {
    OffsetsMap offsets;
    vk::DescriptorSetLayout layout;
    std::set<PASS> passTypes;
    std::set<PIPELINE> pipelineTypes;
    vk::ShaderStageFlags stages;
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
