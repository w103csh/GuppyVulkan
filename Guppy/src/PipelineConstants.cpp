/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "PipelineConstants.h"

#include "ConstantsAll.h"
#include "Enum.h"

namespace Pipeline {

const std::vector<PIPELINE> ALL = {
    GRAPHICS::TRI_LIST_COLOR,
    GRAPHICS::LINE,
    GRAPHICS::POINT,
    GRAPHICS::TRI_LIST_TEX,
    GRAPHICS::CUBE,
    GRAPHICS::CUBE_MAP_COLOR,
    GRAPHICS::CUBE_MAP_LINE,
    GRAPHICS::CUBE_MAP_PT,
    GRAPHICS::CUBE_MAP_TEX,
    GRAPHICS::PBR_COLOR,
    GRAPHICS::PBR_TEX,
    GRAPHICS::BP_TEX_CULL_NONE,
    GRAPHICS::PARALLAX_SIMPLE,
    GRAPHICS::PARALLAX_STEEP,
    GRAPHICS::SCREEN_SPACE_DEFAULT,
    GRAPHICS::SCREEN_SPACE_HDR_LOG,
    GRAPHICS::SCREEN_SPACE_BRIGHT,
    GRAPHICS::SCREEN_SPACE_BLUR_A,
    GRAPHICS::SCREEN_SPACE_BLUR_B,
    // GRAPHICS::SCREEN_SPACE_COMPUTE_DEFAULT,
    GRAPHICS::DEFERRED_MRT_TEX,
    GRAPHICS::DEFERRED_MRT_COLOR,
    GRAPHICS::DEFERRED_MRT_WF_COLOR,
    GRAPHICS::DEFERRED_MRT_PT,
    GRAPHICS::DEFERRED_MRT_LINE,
    GRAPHICS::DEFERRED_MRT_COLOR_RFL_RFR,
    GRAPHICS::DEFERRED_MRT_SKYBOX,
    GRAPHICS::DEFERRED_COMBINE,
    GRAPHICS::DEFERRED_SSAO,
    GRAPHICS::SHADOW_COLOR,
    GRAPHICS::SHADOW_TEX,
// TODO: make tess/geom pipeline/mesh optional based on the context flags.
#ifndef VK_USE_PLATFORM_MACOS_MVK
    GRAPHICS::TESS_BEZIER_4_DEFERRED,
    GRAPHICS::TESS_PHONG_TRI_COLOR_DEFERRED,
    GRAPHICS::TESS_PHONG_TRI_COLOR_WF_DEFERRED,
    GRAPHICS::GEOMETRY_SILHOUETTE_DEFERRED,
#endif
    GRAPHICS::PRTCL_WAVE_DEFERRED,
    GRAPHICS::PRTCL_FOUNTAIN_DEFERRED,
    COMPUTE::PRTCL_EULER,
    GRAPHICS::PRTCL_FOUNTAIN_EULER_DEFERRED,
    GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER,
    COMPUTE::PRTCL_ATTR,
    GRAPHICS::PRTCL_ATTR_PT_DEFERRED,
    COMPUTE::PRTCL_CLOTH,
    COMPUTE::PRTCL_CLOTH_NORM,
    GRAPHICS::PRTCL_CLOTH_DEFERRED,
    COMPUTE::HFF_HGHT,
    COMPUTE::HFF_NORM,
    GRAPHICS::HFF_CLMN_DEFERRED,
    GRAPHICS::HFF_WF_DEFERRED,
    GRAPHICS::HFF_OCEAN_DEFERRED,
    COMPUTE::FFT_ONE,
    COMPUTE::OCEAN_DISP,
    COMPUTE::OCEAN_FFT,
    GRAPHICS::OCEAN_WF_DEFERRED,
    GRAPHICS::OCEAN_SURFACE_DEFERRED,
    GRAPHICS::CDLOD_WF_DEFERRED,
};

const std::map<VERTEX, std::set<PIPELINE>> VERTEX_MAP = {
    {
        VERTEX::COLOR,
        {
            GRAPHICS::TRI_LIST_COLOR,
            GRAPHICS::LINE,
            GRAPHICS::POINT,
            GRAPHICS::CUBE_MAP_COLOR,
            GRAPHICS::CUBE_MAP_LINE,
            GRAPHICS::CUBE_MAP_PT,
            GRAPHICS::PBR_COLOR,
            GRAPHICS::CUBE,
            GRAPHICS::DEFERRED_MRT_COLOR,
            GRAPHICS::DEFERRED_MRT_WF_COLOR,
            GRAPHICS::DEFERRED_MRT_PT,
            GRAPHICS::DEFERRED_MRT_LINE,
            GRAPHICS::DEFERRED_MRT_COLOR_RFL_RFR,
            GRAPHICS::DEFERRED_MRT_SKYBOX,
            GRAPHICS::SHADOW_COLOR,
            GRAPHICS::TESS_BEZIER_4_DEFERRED,
            GRAPHICS::TESS_PHONG_TRI_COLOR_DEFERRED,
            GRAPHICS::TESS_PHONG_TRI_COLOR_WF_DEFERRED,
            GRAPHICS::GEOMETRY_SILHOUETTE_DEFERRED,
            GRAPHICS::PRTCL_WAVE_DEFERRED,
            GRAPHICS::PRTCL_FOUNTAIN_DEFERRED,
            GRAPHICS::PRTCL_FOUNTAIN_EULER_DEFERRED,
            GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER,
        },
    },
    {
        VERTEX::TEXTURE,
        {
            GRAPHICS::TRI_LIST_TEX,
            GRAPHICS::CUBE_MAP_TEX,
            GRAPHICS::PBR_TEX,
            GRAPHICS::BP_TEX_CULL_NONE,
            GRAPHICS::PARALLAX_SIMPLE,
            GRAPHICS::PARALLAX_STEEP,
            GRAPHICS::DEFERRED_MRT_TEX,
            GRAPHICS::SHADOW_TEX,
        },
    },
    {
        VERTEX::SCREEN_QUAD,
        {
            GRAPHICS::SCREEN_SPACE_DEFAULT,
            GRAPHICS::SCREEN_SPACE_HDR_LOG,
            GRAPHICS::SCREEN_SPACE_BRIGHT,
            GRAPHICS::SCREEN_SPACE_BLUR_A,
            GRAPHICS::SCREEN_SPACE_BLUR_B,
            GRAPHICS::DEFERRED_COMBINE,
            GRAPHICS::DEFERRED_SSAO,
        },
    },
    {
        VERTEX::DONT_CARE,
        {
            COMPUTE::SCREEN_SPACE_DEFAULT,
            COMPUTE::PRTCL_EULER,
            COMPUTE::PRTCL_ATTR,
            GRAPHICS::PRTCL_ATTR_PT_DEFERRED,
            COMPUTE::PRTCL_CLOTH,
            GRAPHICS::PRTCL_CLOTH_DEFERRED,
            COMPUTE::HFF_HGHT,
            COMPUTE::HFF_NORM,
            GRAPHICS::HFF_CLMN_DEFERRED,
            GRAPHICS::HFF_WF_DEFERRED,
            GRAPHICS::HFF_OCEAN_DEFERRED,
            COMPUTE::FFT_ONE,
            COMPUTE::OCEAN_DISP,
            COMPUTE::OCEAN_FFT,
            GRAPHICS::OCEAN_WF_DEFERRED,
            GRAPHICS::OCEAN_SURFACE_DEFERRED,
        },
    },
};

// Types listed here will pass the test in the RenderPass::Base::update
// and be allowed to collect descriptor set binding data without a mesh.
const std::set<PIPELINE> MESHLESS = {
    GRAPHICS::SCREEN_SPACE_DEFAULT,  //
    GRAPHICS::SCREEN_SPACE_HDR_LOG,  //
    GRAPHICS::SCREEN_SPACE_BRIGHT,   //
    GRAPHICS::SCREEN_SPACE_BLUR_A,   //
    GRAPHICS::SCREEN_SPACE_BLUR_B,   //
    GRAPHICS::DEFERRED_COMBINE,      //
    GRAPHICS::SHADOW_COLOR,          //
    GRAPHICS::SHADOW_TEX,            //
};

// DEFAULT
namespace Default {

// TRIANGLE LIST COLOR
const Pipeline::CreateInfo TRI_LIST_COLOR_CREATE_INFO = {
    GRAPHICS::TRI_LIST_COLOR,
    "Default Triangle List Color",
    {SHADER::COLOR_VERT, SHADER::COLOR_FRAG},
    {
        {DESCRIPTOR_SET::UNIFORM_DEFAULT, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)},
#if DO_PROJECTOR
        {DESCRIPTOR_SET::PROJECTOR_DEFAULT, vk::ShaderStageFlagBits::eFragment},
#endif
    },
    // Descriptor::OffsetsMap::Type{
    //    {UNIFORM::FOG_DEFAULT, {1}},
    //    //{UNIFORM::LIGHT_POSITIONAL_DEFAULT, {2}},
    //},
};

// LINE
const Pipeline::CreateInfo LINE_CREATE_INFO = {
    GRAPHICS::LINE,
    "Default Line",
    {SHADER::COLOR_VERT, SHADER::LINE_FRAG},
    {{DESCRIPTOR_SET::UNIFORM_DEFAULT, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)}},
};

// POINT
const Pipeline::CreateInfo POINT_CREATE_INFO = {
    GRAPHICS::POINT,
    "Default Point",
    {SHADER::POINT_VERT, SHADER::LINE_FRAG},
    {{DESCRIPTOR_SET::UNIFORM_DEFAULT, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)}},
};

// TRIANGLE LIST TEXTURE
const Pipeline::CreateInfo TRI_LIST_TEX_CREATE_INFO = {
    GRAPHICS::TRI_LIST_TEX,
    "Default Triangle List Texture",
    {SHADER::TEX_VERT, SHADER::TEX_FRAG},
    {
        {DESCRIPTOR_SET::UNIFORM_DEFAULT, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)},
        {DESCRIPTOR_SET::SAMPLER_DEFAULT, vk::ShaderStageFlagBits::eFragment},
        // {DESCRIPTOR_SET::PROJECTOR_DEFAULT, vk::ShaderStageFlagBits::eFragment},
    },
};

// CUBE
const Pipeline::CreateInfo CUBE_CREATE_INFO = {
    GRAPHICS::CUBE,
    "Cube Pipeline",
    {SHADER::CUBE_VERT, SHADER::CUBE_FRAG},
    {
        {DESCRIPTOR_SET::UNIFORM_DEFAULT, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)},
        {DESCRIPTOR_SET::SAMPLER_CUBE_DEFAULT, vk::ShaderStageFlagBits::eFragment},
    },
};

}  // namespace Default

// BLINN PHONG
namespace BP {

// TEXTURE CULL NONE
const Pipeline::CreateInfo TEX_CULL_NONE_CREATE_INFO = {
    GRAPHICS::BP_TEX_CULL_NONE,
    "Blinn Phong Texture Cull None",
    {SHADER::TEX_VERT, SHADER::TEX_FRAG},
    {
        {DESCRIPTOR_SET::UNIFORM_DEFAULT, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)},
        {DESCRIPTOR_SET::SAMPLER_DEFAULT, vk::ShaderStageFlagBits::eFragment},
        // {DESCRIPTOR_SET::PROJECTOR_DEFAULT, vk::ShaderStageFlagBits::eFragment},
    },
};

}  // namespace BP

}  // namespace Pipeline