/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef SHADER_CONSTANTS_H
#define SHADER_CONSTANTS_H

#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vulkan/vulkan.hpp>

#include "DescriptorConstants.h"

enum class SHADER {
    LINK = -1,
    // These are index values, so the order here
    // must match order in ALL...
    // DEFAULT
    COLOR_VERT = 0,
    POINT_VERT,
    COLOR_FRAG,
    LINE_FRAG,
    TEX_VERT,
    TEX_FRAG,
    CUBE_VERT,
    CUBE_FRAG,
    // Faster versions
    VERT_COLOR,
    VERT_POINT,
    VERT_TEX,
    VERT_COLOR_CUBE_MAP,
    VERT_PT_CUBE_MAP,
    VERT_TEX_CUBE_MAP,
    VERT_SKYBOX,
    GEOM_COLOR_CUBE_MAP,
    GEOM_PT_CUBE_MAP,
    GEOM_TEX_CUBE_MAP,
    // PBR
    PBR_COLOR_FRAG,
    PBR_TEX_FRAG,
    // PARALLAX
    PARALLAX_VERT,
    PARALLAX_SIMPLE_FRAG,
    PARALLAX_STEEP_FRAG,
    // SCREEN SPACE
    SCREEN_SPACE_VERT,
    SCREEN_SPACE_FRAG,
    SCREEN_SPACE_HDR_LOG_FRAG,
    SCREEN_SPACE_BRIGHT_FRAG,
    SCREEN_SPACE_BLUR_FRAG,
    SCREEN_SPACE_COMP,
    // DEFERRED
    DEFERRED_VERT,
    DEFERRED_FRAG,
    DEFERRED_MS_FRAG,
    DEFERRED_MRT_TEX_CS_VERT,
    DEFERRED_MRT_TEX_FRAG,
    DEFERRED_MRT_COLOR_CS_VERT,
    DEFERRED_MRT_COLOR_FRAG,
    DEFERRED_MRT_POINT_FRAG,
    DEFERRED_MRT_COLOR_RFL_RFR_FRAG,
    DEFERRED_MRT_SKYBOX_FRAG,
    DEFERRED_SSAO_FRAG,
    // SHADOW
    SHADOW_COLOR_VERT,
    SHADOW_TEX_VERT,
    SHADOW_COLOR_CUBE_VERT,
    SHADOW_TEX_CUBE_VERT,
    SHADOW_CUBE_GEOM,
    SHADOW_FRAG,
    // TESSELLATION
    TESS_COLOR_VERT,
    BEZIER_4_TESC,
    BEZIER_4_TESE,
    PHONG_TRI_COLOR_TESC,
    PHONG_TRI_COLOR_TESE,
    // GEOMETRY
    WIREFRAME_GEOM,
    SILHOUETTE_GEOM,
    // PARTICLE
    PRTCL_WAVE_VERT,
    PRTCL_FOUNTAIN_VERT,
    PRTCL_FOUNTAIN_DEFERRED_MRT_FRAG,
    PRTCL_EULER_COMP,
    PRTCL_FOUNTAIN_EULER_VERT,
    PRTCL_SHDW_FOUNTAIN_EULER_VERT,
    PRTCL_ATTR_COMP,
    PRTCL_ATTR_VERT,
    PRTCL_CLOTH_COMP,
    PRTCL_CLOTH_NORM_COMP,
    PRTCL_CLOTH_VERT,
    // WATER
    HFF_HGHT_COMP,
    HFF_NORM_COMP,
    HFF_VERT,
    HFF_CLMN_VERT,
    // FFT
    FFT_ONE_COMP,
    // OCEAN
    OCEAN_DISP_COMP,
    OCEAN_FFT_COMP,
    OCEAN_VERT_INPUT_COMP,
    OCEAN_VERT,
    OCEAN_CDLOD_VERT,
    OCEAN_DEFERRED_MRT_FRAG,
    // CDLOD
    CDLOD_VERT,
    CDLOD_TEX_VERT,
#ifdef USE_VOLUMETRIC_LIGHTING
    // ...
#endif
    // Add new to SHADER_ALL and SHADER_LINK_MAP.
};

enum class SHADER_LINK {
    //
    COLOR_FRAG = 0,
    TEX_FRAG,
    UTILITY_VERT,
    UTILITY_FRAG,
    BLINN_PHONG,
    DEFAULT_MATERIAL,
    HELPERS,
    PBR_FRAG,
    PBR_MATERIAL,
    GEOMETRY_FRAG,
    PRTCL_FOUNTAIN,
    CDLOD,
    // Add new to SHADER_LINK_ALL and SHADER_LINK_MAP.
};

namespace Shader {

// directory of shader files relative to the root of the repo
const std::string BASE_DIRNAME = "shaders/";

struct CreateInfo {
    SHADER type;
    std::string name;
    std::string_view fileName;
    vk::ShaderStageFlagBits stage;
    std::set<SHADER_LINK> linkTypes;
    std::map<std::string, std::string> replaceMap;
};

// Pipeline shader stage create info map
using infoMapKey = std::tuple<SHADER, PIPELINE, std::set<PASS>>;
using infoMapValue = std::pair<Descriptor::Set::textReplaceTuples, vk::PipelineShaderStageCreateInfo>;
using infoMapKeyValue = std::pair<const infoMapKey, infoMapValue>;
using infoMap = std::multimap<infoMapKey, infoMapValue>;

namespace Link {

struct CreateInfo {
    SHADER_LINK type;
    std::string_view fileName;
    std::map<std::string, std::string> replaceMap;
};

}  // namespace Link

// extern const std::string LIGHT_DEFAULT_POSITIONAL_STRUCT_STRING;
// extern const std::string LIGHT_DEFAULT_SPOT_STRUCT_STRING;

extern const std::map<SHADER, Shader::CreateInfo> ALL;
extern const std::map<SHADER_LINK, Shader::Link::CreateInfo> LINK_ALL;
extern const std::map<SHADER, std::set<SHADER_LINK>> LINK_MAP;

/* When this was defined in Tessellation.h clang was throwing initialization
 *  errors, so I moved it here. I tried to make a TessellationConstants.h and it did
 *  not help the problem. The extern const initializers might be revealing that they
 *  are not reliable enough... ugh.
 */
namespace Tessellation {
extern const CreateInfo COLOR_VERT_CREATE_INFO;
extern const CreateInfo BEZIER_4_TESC_CREATE_INFO;
extern const CreateInfo BEZIER_4_TESE_CREATE_INFO;
extern const CreateInfo PHONG_TRI_COLOR_TESC_CREATE_INFO;
extern const CreateInfo PHONG_TRI_COLOR_TESE_CREATE_INFO;
}  // namespace Tessellation

}  // namespace Shader

#endif  // !SHADER_CONSTANTS_H
