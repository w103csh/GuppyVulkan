/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef PIPELINE_CONSTANTS_H
#define PIPELINE_CONSTANTS_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

#include "DescriptorConstants.h"

enum class DESCRIPTOR_SET;
enum class PUSH_CONSTANT;
enum class PASS : uint32_t;
enum class SHADER;
enum class VERTEX;

enum class GRAPHICS : uint32_t {
    // DEFAULT
    TRI_LIST_COLOR = 0,
    LINE,
    POINT,
    TRI_LIST_TEX,
    CUBE,
    CUBE_MAP_COLOR,
    CUBE_MAP_LINE,
    CUBE_MAP_PT,
    CUBE_MAP_TEX,
    // PBR
    PBR_COLOR,
    PBR_TEX,
    // BLINN PHONG
    BP_TEX_CULL_NONE,
    // PARALLAX
    PARALLAX_SIMPLE,
    PARALLAX_STEEP,
    // SCREEN SPACE
    SCREEN_SPACE_DEFAULT,
    SCREEN_SPACE_HDR_LOG,
    SCREEN_SPACE_BRIGHT,
    SCREEN_SPACE_BLUR_A,
    SCREEN_SPACE_BLUR_B,
    // DEFERRED
    DEFERRED_MRT_TEX,
    DEFERRED_MRT_COLOR,
    DEFERRED_MRT_WF_COLOR,
    DEFERRED_MRT_PT,
    DEFERRED_MRT_LINE,
    DEFERRED_MRT_COLOR_RFL_RFR,
    DEFERRED_MRT_SKYBOX,
    DEFERRED_SSAO,
    DEFERRED_COMBINE,
    // SHADOW
    SHADOW_COLOR,
    SHADOW_TEX,
    // TESSELLATION
    TESSELLATION_BEZIER_4_DEFERRED,
    TESSELLATION_TRIANGLE_DEFERRED,
    // GEOMETRY
    GEOMETRY_SILHOUETTE_DEFERRED,
    // PARTICLE
    PRTCL_WAVE_DEFERRED,
    PRTCL_FOUNTAIN_DEFERRED,
    PRTCL_FOUNTAIN_EULER_DEFERRED,
    PRTCL_SHDW_FOUNTAIN_EULER,
    PRTCL_ATTR_PT_DEFERRED,
    PRTCL_CLOTH_DEFERRED,
    // HEIGHT FLUID FIELD
    HFF_CLMN_DEFERRED,
    HFF_WF_DEFERRED,
    HFF_OCEAN_DEFERRED,
    // OCEAN
    OCEAN_WF_DEFERRED,
    // Used to indicate bad data, and "all" in uniform offsets
    ALL_ENUM = UINT32_MAX,
    // Add new to PIPELINE_ALL and VERTEX_PIPELINE_MAP in code file.
};

enum class COMPUTE : uint32_t {
    // SCREEN SPACE
    SCREEN_SPACE_DEFAULT,
    // PARTICLE
    PRTCL_EULER,
    PRTCL_ATTR,
    PRTCL_CLOTH,
    PRTCL_CLOTH_NORM,
    // HEIGHT FLUID FIELD
    HFF_HGHT,
    HFF_NORM,
    // FFT
    FFT_ONE,
    // OCEAN
    OCEAN_DISP,
    OCEAN_FFT,
    // Used to indicate bad data, and "all" in uniform offsets
    ALL_ENUM = UINT32_MAX,
    // Add new to PIPELINE_ALL and VERTEX_PIPELINE_MAP in code file.
};

namespace Pipeline {

// clang-format off
struct IsGraphics {
    template <typename T>
    bool operator()(const T&) const { return false; }
    bool operator()(const GRAPHICS&) const { return true; }
};
struct GetGraphics {
    template <typename T>
    GRAPHICS operator()(const T&) const { return GRAPHICS::ALL_ENUM; }
    GRAPHICS operator()(const GRAPHICS& type) const { return type; }
};
struct IsCompute {
    template <typename T>
    bool operator()(const T&) const { return false; }
    bool operator()(const COMPUTE&) const { return true; }
};
struct GetCompute {
    template <typename T>
    COMPUTE operator()(const T&) const { return COMPUTE::ALL_ENUM; }
    COMPUTE operator()(const COMPUTE& type) const { return type; }
};
struct IsAll {
    template <typename T>
    bool operator()(const T&) const { return false; }
    bool operator()(const GRAPHICS& type) const { return type == GRAPHICS::ALL_ENUM; }
    bool operator()(const COMPUTE&  type) const { return type == COMPUTE::ALL_ENUM; }
};
// clang-format on

extern const std::vector<PIPELINE> ALL;
extern const std::map<VERTEX, std::set<PIPELINE>> VERTEX_MAP;
extern const std::set<PIPELINE> MESHLESS;

struct BindData {
    const PIPELINE type;
    const VkPipelineBindPoint bindPoint;
    const VkPipelineLayout layout;
    VkPipeline pipeline;
    VkShaderStageFlags pushConstantStages;
    const std::vector<PUSH_CONSTANT> pushConstantTypes;
    bool usesAdjacency;
};

// Map of pipeline/pass to bind data shared pointers
using pipelineBindDataMapKey = std::pair<PIPELINE, PASS>;
using pipelineBindDataMapKeyValue = std::pair<const pipelineBindDataMapKey, const std::shared_ptr<Pipeline::BindData>>;
using pipelineBindDataMap = std::map<pipelineBindDataMapKey, const std::shared_ptr<Pipeline::BindData>>;
using pipelineBindDataList = insertion_ordered_unique_keyvalue_list<PIPELINE, std::shared_ptr<Pipeline::BindData>>;

// Map of pass types to descriptor set text replace tuples
using shaderTextReplaceInfoMap = std::map<std::set<PASS>, Descriptor::Set::textReplaceTuples>;

struct CreateInfo {
    PIPELINE type;
    std::string name = "";
    std::set<SHADER> shaderTypes;
    std::vector<DESCRIPTOR_SET> descriptorSets;
    Descriptor::OffsetsMap uniformOffsets;
    std::vector<PUSH_CONSTANT> pushConstantTypes;
};

namespace Default {
struct PushConstant {
    glm::mat4 model;
};
extern const Pipeline::CreateInfo TRI_LIST_COLOR_CREATE_INFO;
extern const Pipeline::CreateInfo LINE_CREATE_INFO;
extern const Pipeline::CreateInfo POINT_CREATE_INFO;
extern const Pipeline::CreateInfo TRI_LIST_TEX_CREATE_INFO;
extern const Pipeline::CreateInfo CUBE_CREATE_INFO;
}  // namespace Default

namespace BP {
extern const Pipeline::CreateInfo TEX_CULL_NONE_CREATE_INFO;
}  // namespace BP

}  // namespace Pipeline

#endif  // !PIPELINE_CONSTANTS_H
