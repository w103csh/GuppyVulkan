/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "ShaderConstants.h"

#include "Cdlod.h"
#include "Cloth.h"
#include "Deferred.h"
#include "FFT.h"
#include "Geometry.h"
#include "HeightFieldFluid.h"
#include "Ocean.h"
#include "OceanComputeWork.h"
#include "PBR.h"
#include "Parallax.h"
#include "Particle.h"
#include "ScreenSpace.h"
#include "Shadow.h"
#include "Tessellation.h"

//#define MULTILINE(...) #__VA_ARGS__

namespace Shader {

// SHADER INFO

const CreateInfo COLOR_VERT_CREATE_INFO = {
    SHADER::COLOR_VERT,                //
    "Default Color Vertex Shader",     //
    "color.vert",                      //
    vk::ShaderStageFlagBits::eVertex,  //
    {SHADER_LINK::UTILITY_VERT},
};

const CreateInfo COLOR_FRAG_CREATE_INFO = {
    SHADER::COLOR_FRAG,
    "Default Color Fragment Shader",
    "color.frag",
    vk::ShaderStageFlagBits::eFragment,
    {
        SHADER_LINK::COLOR_FRAG,
        SHADER_LINK::UTILITY_FRAG,
        SHADER_LINK::BLINN_PHONG,
        SHADER_LINK::DEFAULT_MATERIAL,
    },
};

const CreateInfo LINE_FRAG_CREATE_INFO = {
    SHADER::LINE_FRAG,                   //
    "Default Line Fragment Shader",      //
    "line.frag",                         //
    vk::ShaderStageFlagBits::eFragment,  //
    {SHADER_LINK::UTILITY_FRAG},
};

const CreateInfo TEX_VERT_CREATE_INFO = {
    SHADER::TEX_VERT,                  //
    "Default Texture Vertex Shader",   //
    "texture.vert",                    //
    vk::ShaderStageFlagBits::eVertex,  //
    {SHADER_LINK::UTILITY_VERT},
};

const CreateInfo TEX_FRAG_CREATE_INFO = {
    SHADER::TEX_FRAG,
    "Default Texture Fragment Shader",
    "texture.frag",
    vk::ShaderStageFlagBits::eFragment,
    {
        SHADER_LINK::TEX_FRAG,
        SHADER_LINK::UTILITY_FRAG,
        SHADER_LINK::BLINN_PHONG,
        SHADER_LINK::DEFAULT_MATERIAL,
    },
};

const CreateInfo CUBE_VERT_CREATE_INFO = {
    SHADER::CUBE_VERT,                 //
    "Cube Vertex Shader",              //
    "cube.vert",                       //
    vk::ShaderStageFlagBits::eVertex,  //
    {SHADER_LINK::DEFAULT_MATERIAL},
};

const CreateInfo CUBE_FRAG_CREATE_INFO = {
    SHADER::CUBE_FRAG,                   //
    "Cube Fragment Shader",              //
    "cube.frag",                         //
    vk::ShaderStageFlagBits::eFragment,  //
    {
        SHADER_LINK::DEFAULT_MATERIAL,
        SHADER_LINK::UTILITY_FRAG,
    },
};

const CreateInfo POINT_VERT_CREATE_INFO = {
    SHADER::POINT_VERT,
    "Default Point Vertex Shader",
    "point.vert",
    vk::ShaderStageFlagBits::eVertex,
};

const CreateInfo VERT_COLOR_CREATE_INFO = {
    SHADER::VERT_COLOR,
    "Vertex Color Shader",
    "vert.color.glsl",
    vk::ShaderStageFlagBits::eVertex,
};

const CreateInfo VERT_PT_CREATE_INFO = {
    SHADER::VERT_POINT,
    "Vertex Point Shader",
    "vert.point.glsl",
    vk::ShaderStageFlagBits::eVertex,
};

const CreateInfo VERT_TEX_CREATE_INFO = {
    SHADER::VERT_TEX,
    "Vertex Texture Shader",
    "vert.texture.glsl",
    vk::ShaderStageFlagBits::eVertex,
};

const CreateInfo VERT_COLOR_CUBE_MAP_CREATE_INFO = {
    SHADER::VERT_COLOR_CUBE_MAP,
    "Vertex Color Cube Map Shader",
    "vert.color.cube.glsl",
    vk::ShaderStageFlagBits::eVertex,
};

const CreateInfo VERT_PT_CUBE_MAP_CREATE_INFO = {
    SHADER::VERT_PT_CUBE_MAP,
    "Vertex Point Cube Map Shader",
    "vert.point.cube.glsl",
    vk::ShaderStageFlagBits::eVertex,
};

const CreateInfo VERT_TEX_CUBE_MAP_CREATE_INFO = {
    SHADER::VERT_TEX_CUBE_MAP,
    "Vertex Texture Cube Map Shader",
    "vert.texture.cube.glsl",
    vk::ShaderStageFlagBits::eVertex,
};

const CreateInfo VERT_SKYBOX_CREATE_INFO = {
    SHADER::VERT_SKYBOX,
    "Vertex Skybox Shader",
    "vert.skybox.glsl",
    vk::ShaderStageFlagBits::eVertex,
};

const CreateInfo GEOM_COLOR_CREATE_INFO = {
    SHADER::GEOM_COLOR_CUBE_MAP,
    "Geometry Color Cube Map Shader",
    "geom.color.cube.glsl",
    vk::ShaderStageFlagBits::eGeometry,
};

const CreateInfo GEOM_PT_CREATE_INFO = {
    SHADER::GEOM_PT_CUBE_MAP,
    "Geometry Point Cube Map Shader",
    "geom.point.cube.glsl",
    vk::ShaderStageFlagBits::eGeometry,
};

const CreateInfo GEOM_TEX_CREATE_INFO = {
    SHADER::GEOM_TEX_CUBE_MAP,
    "Geometry Texture Cube Map Shader",
    "geom.texture.cube.glsl",
    vk::ShaderStageFlagBits::eGeometry,
};

namespace Link {

// LINK SHADER INFO

const CreateInfo COLOR_FRAG_CREATE_INFO = {
    SHADER_LINK::COLOR_FRAG,  //
    "link.color.frag",        //
};

const CreateInfo TEX_FRAG_CREATE_INFO = {
    SHADER_LINK::TEX_FRAG,  //
    "link.texture.frag",    //
};

const CreateInfo UTILITY_VERT_CREATE_INFO = {
    SHADER_LINK::UTILITY_VERT,  //
    "link.utility.vert.glsl",   //
};

const CreateInfo UTILITY_FRAG_CREATE_INFO = {
    SHADER_LINK::UTILITY_FRAG,  //
    "link.utility.frag.glsl",   //
};

const CreateInfo BLINN_PHONG_CREATE_INFO = {
    SHADER_LINK::BLINN_PHONG,  //
    "link.blinnPhong.glsl",    //
};

const CreateInfo DEFAULT_MATERIAL_CREATE_INFO = {
    SHADER_LINK::DEFAULT_MATERIAL,  //
    "link.default.material.glsl",   //
};

const CreateInfo HELPERS_CREATE_INFO = {
    SHADER_LINK::HELPERS,  //
    "link.helpers.glsl",   //
};

}  // namespace Link

namespace Tessellation {

const CreateInfo COLOR_VERT_CREATE_INFO = {
    SHADER::TESS_COLOR_VERT,
    "Tessellation Color Vertex Shader",
    "vert.color.tess.passthrough.glsl",
    vk::ShaderStageFlagBits::eVertex,
};
const CreateInfo BEZIER_4_TESC_CREATE_INFO = {
    SHADER::BEZIER_4_TESC,
    "Bezier 4 Control Point Tessellation Control Shader",
    "tesc.bezier4.glsl",
    vk::ShaderStageFlagBits::eTessellationControl,
};
const CreateInfo BEZIER_4_TESE_CREATE_INFO = {
    SHADER::BEZIER_4_TESE,
    "Bezier 4 Control Point Tessellation Evaluation Shader",
    "tese.bezier4.glsl",
    vk::ShaderStageFlagBits::eTessellationEvaluation,
};
const CreateInfo PHONG_TRI_COLOR_TESC_CREATE_INFO = {
    SHADER::PHONG_TRI_COLOR_TESC,
    "Phong Tessellation Triangle Color Control Shader",
    "tesc.phong.triangle.color.glsl",
    vk::ShaderStageFlagBits::eTessellationControl,
};
const CreateInfo PHONG_TRI_COLOR_TESE_CREATE_INFO = {
    SHADER::PHONG_TRI_COLOR_TESE,  //
    "Phong Tessellation Triangle Color Evaluation Shader",
    "tese.phong.triangle.color.glsl",
    vk::ShaderStageFlagBits::eTessellationEvaluation,
};

}  // namespace Tessellation

// const std::string LIGHT_DEFAULT_POSITIONAL_STRUCT_STRING = MULTILINE(
//    //
//    struct LightDefaultPositional {
//        vec3 position;  // Light position in eye coords.
//        uint flags;
//        // 16
//        vec3 La;  // Ambient light intesity
//        // rem 4
//        vec3 L;  // Diffuse and specular light intensity
//        // rem 4
//    };
//    //
//);

// const std::string LIGHT_DEFAULT_SPOT_STRUCT_STRING = MULTILINE(
//    //
//    struct LightDefaultSpot {
//        vec3 position;
//        uint flags;
//        // 16
//        vec3 La;
//        float exponent;
//        // 16
//        vec3 L;
//        float cutoff;
//        // 16
//        vec3 direction;
//        // rem 4
//    };
//    //
//);

const std::map<SHADER, Shader::CreateInfo> ALL = {
    // DEFAULT
    {SHADER::COLOR_VERT, Shader::COLOR_VERT_CREATE_INFO},
    {SHADER::POINT_VERT, Shader::POINT_VERT_CREATE_INFO},
    {SHADER::COLOR_FRAG, Shader::COLOR_FRAG_CREATE_INFO},
    {SHADER::LINE_FRAG, Shader::LINE_FRAG_CREATE_INFO},
    {SHADER::TEX_VERT, Shader::TEX_VERT_CREATE_INFO},
    {SHADER::TEX_FRAG, Shader::TEX_FRAG_CREATE_INFO},
    {SHADER::CUBE_VERT, Shader::CUBE_VERT_CREATE_INFO},
    {SHADER::CUBE_FRAG, Shader::CUBE_FRAG_CREATE_INFO},
    // Faster versions
    {SHADER::VERT_COLOR, Shader::VERT_COLOR_CREATE_INFO},
    {SHADER::VERT_POINT, Shader::VERT_PT_CREATE_INFO},
    {SHADER::VERT_TEX, Shader::VERT_TEX_CREATE_INFO},
    {SHADER::VERT_COLOR_CUBE_MAP, Shader::VERT_COLOR_CUBE_MAP_CREATE_INFO},
    {SHADER::VERT_PT_CUBE_MAP, Shader::VERT_PT_CUBE_MAP_CREATE_INFO},
    {SHADER::VERT_TEX_CUBE_MAP, Shader::VERT_TEX_CUBE_MAP_CREATE_INFO},
    {SHADER::VERT_SKYBOX, Shader::VERT_SKYBOX_CREATE_INFO},
    {SHADER::GEOM_COLOR_CUBE_MAP, Shader::GEOM_COLOR_CREATE_INFO},
    {SHADER::GEOM_PT_CUBE_MAP, Shader::GEOM_PT_CREATE_INFO},
    {SHADER::GEOM_TEX_CUBE_MAP, Shader::GEOM_TEX_CREATE_INFO},
    // PBR
    {SHADER::PBR_COLOR_FRAG, Shader::PBR::COLOR_FRAG_CREATE_INFO},
    {SHADER::PBR_TEX_FRAG, Shader::PBR::TEX_FRAG_CREATE_INFO},
    // PARALLAX
    {SHADER::PARALLAX_VERT, Shader::Parallax::VERT_CREATE_INFO},
    {SHADER::PARALLAX_SIMPLE_FRAG, Shader::Parallax::SIMPLE_CREATE_INFO},
    {SHADER::PARALLAX_STEEP_FRAG, Shader::Parallax::STEEP_CREATE_INFO},
    // SCREEN SPACE
    {SHADER::SCREEN_SPACE_VERT, Shader::ScreenSpace::VERT_CREATE_INFO},
    {SHADER::SCREEN_SPACE_FRAG, Shader::ScreenSpace::FRAG_CREATE_INFO},
    {SHADER::SCREEN_SPACE_HDR_LOG_FRAG, Shader::ScreenSpace::FRAG_HDR_LOG_CREATE_INFO},
    {SHADER::SCREEN_SPACE_BRIGHT_FRAG, Shader::ScreenSpace::FRAG_BRIGHT_CREATE_INFO},
    {SHADER::SCREEN_SPACE_BLUR_FRAG, Shader::ScreenSpace::FRAG_BLUR_CREATE_INFO},
    {SHADER::SCREEN_SPACE_COMP, Shader::ScreenSpace::COMP_CREATE_INFO},
    // DEFERRED
    {SHADER::DEFERRED_VERT, Shader::Deferred::VERT_CREATE_INFO},
    {SHADER::DEFERRED_FRAG, Shader::Deferred::FRAG_CREATE_INFO},
    {SHADER::DEFERRED_MS_FRAG, Shader::Deferred::FRAG_MS_CREATE_INFO},
    {SHADER::DEFERRED_MRT_TEX_CS_VERT, Shader::Deferred::MRT_TEX_CS_VERT_CREATE_INFO},
    {SHADER::DEFERRED_MRT_TEX_FRAG, Shader::Deferred::MRT_TEX_FRAG_CREATE_INFO},
    {SHADER::DEFERRED_MRT_COLOR_CS_VERT, Shader::Deferred::MRT_COLOR_CS_VERT_CREATE_INFO},
    {SHADER::DEFERRED_MRT_COLOR_FRAG, Shader::Deferred::MRT_COLOR_FRAG_CREATE_INFO},
    {SHADER::DEFERRED_MRT_POINT_FRAG, Shader::Deferred::MRT_POINT_FRAG_CREATE_INFO},
    {SHADER::DEFERRED_MRT_COLOR_RFL_RFR_FRAG, Shader::Deferred::MRT_COLOR_REF_REF_FRAG_CREATE_INFO},
    {SHADER::DEFERRED_MRT_SKYBOX_FRAG, Shader::Deferred::MRT_SKYBOX_FRAG_CREATE_INFO},
    {SHADER::DEFERRED_SSAO_FRAG, Shader::Deferred::SSAO_FRAG_CREATE_INFO},
    // SHADOW
    {SHADER::SHADOW_COLOR_VERT, Shader::Shadow::COLOR_VERT_CREATE_INFO},
    {SHADER::SHADOW_TEX_VERT, Shader::Shadow::TEX_VERT_CREATE_INFO},
    {SHADER::SHADOW_CUBE_GEOM, Shader::Shadow::CUBE_GEOM_CREATE_INFO},
    {SHADER::SHADOW_FRAG, Shader::Shadow::FRAG_CREATE_INFO},
    // TESSELLATION
    {SHADER::TESS_COLOR_VERT, Shader::Tessellation::COLOR_VERT_CREATE_INFO},
    {SHADER::BEZIER_4_TESC, Shader::Tessellation::BEZIER_4_TESC_CREATE_INFO},
    {SHADER::BEZIER_4_TESE, Shader::Tessellation::BEZIER_4_TESE_CREATE_INFO},
    {SHADER::PHONG_TRI_COLOR_TESC, Shader::Tessellation::PHONG_TRI_COLOR_TESC_CREATE_INFO},
    {SHADER::PHONG_TRI_COLOR_TESE, Shader::Tessellation::PHONG_TRI_COLOR_TESE_CREATE_INFO},
    // GEOMETRY
    {SHADER::WIREFRAME_GEOM, Shader::Geometry::WIREFRAME_CREATE_INFO},
    {SHADER::SILHOUETTE_GEOM, Shader::Geometry::SILHOUETTE_CREATE_INFO},
    // PARTICLE
    {SHADER::PRTCL_WAVE_VERT, Shader::Particle::WAVE_VERT_CREATE_INFO},
    {SHADER::PRTCL_FOUNTAIN_VERT, Shader::Particle::FOUNTAIN_VERT_CREATE_INFO},
    {SHADER::PRTCL_FOUNTAIN_DEFERRED_MRT_FRAG, Shader::Particle::FOUNTAIN_FRAG_DEFERRED_MRT_CREATE_INFO},
    {SHADER::PRTCL_EULER_COMP, Shader::Particle::EULER_CREATE_INFO},
    {SHADER::PRTCL_FOUNTAIN_EULER_VERT, Shader::Particle::FOUNTAIN_EULER_VERT_CREATE_INFO},
    {SHADER::PRTCL_SHDW_FOUNTAIN_EULER_VERT, Shader::Particle::SHDW_FOUNTAIN_EULER_VERT_CREATE_INFO},
    {SHADER::PRTCL_ATTR_COMP, Shader::Particle::ATTR_COMP_CREATE_INFO},
    {SHADER::PRTCL_ATTR_VERT, Shader::Particle::ATTR_VERT_CREATE_INFO},
    {SHADER::PRTCL_CLOTH_COMP, Shader::Particle::CLOTH_COMP_CREATE_INFO},
    {SHADER::PRTCL_CLOTH_NORM_COMP, Shader::Particle::CLOTH_NORM_COMP_CREATE_INFO},
    {SHADER::PRTCL_CLOTH_VERT, Shader::Particle::CLOTH_VERT_CREATE_INFO},
    // WATER
    {SHADER::HFF_HGHT_COMP, Shader::HFF_COMP_CREATE_INFO},
    {SHADER::HFF_NORM_COMP, Shader::HFF_NORM_COMP_CREATE_INFO},
    {SHADER::HFF_VERT, Shader::HFF_VERT_CREATE_INFO},
    {SHADER::HFF_CLMN_VERT, Shader::HFF_CLMN_VERT_CREATE_INFO},
    // FFT
    {SHADER::FFT_ONE_COMP, Shader::FFT_ONE_COMP_CREATE_INFO},
    // OCEAN
    {SHADER::OCEAN_DISP_COMP, Shader::Ocean::DISP_COMP_CREATE_INFO},
    {SHADER::OCEAN_FFT_COMP, Shader::Ocean::FFT_COMP_CREATE_INFO},
    {SHADER::OCEAN_VERT_INPUT_COMP, Shader::Ocean::VERT_INPUT_COMP_CREATE_INFO},
    {SHADER::OCEAN_VERT, Shader::Ocean::VERT_CREATE_INFO},
    {SHADER::OCEAN_DEFERRED_MRT_FRAG, Shader::Ocean::DEFERRED_MRT_FRAG_CREATE_INFO},
    // CDLOD
    {SHADER::CDLOD_VERT, Shader::Cdlod::VERT_CREATE_INFO},
    {SHADER::CDLOD_TEX_VERT, Shader::Cdlod::VERT_TEX_CREATE_INFO},
};

const std::map<SHADER_LINK, Shader::Link::CreateInfo> LINK_ALL = {
    // DEFAULT
    {SHADER_LINK::COLOR_FRAG, Shader::Link::COLOR_FRAG_CREATE_INFO},
    {SHADER_LINK::TEX_FRAG, Shader::Link::TEX_FRAG_CREATE_INFO},
    {SHADER_LINK::UTILITY_VERT, Shader::Link::UTILITY_VERT_CREATE_INFO},
    {SHADER_LINK::UTILITY_FRAG, Shader::Link::UTILITY_FRAG_CREATE_INFO},
    {SHADER_LINK::BLINN_PHONG, Shader::Link::BLINN_PHONG_CREATE_INFO},
    {SHADER_LINK::DEFAULT_MATERIAL, Shader::Link::DEFAULT_MATERIAL_CREATE_INFO},
    {SHADER_LINK::HELPERS, Shader::Link::HELPERS_CREATE_INFO},
    // PBR
    {SHADER_LINK::PBR_FRAG, Shader::Link::PBR::FRAG_CREATE_INFO},
    {SHADER_LINK::PBR_MATERIAL, Shader::Link::PBR::MATERIAL_CREATE_INFO},
    // GEOMETRY
    {SHADER_LINK::GEOMETRY_FRAG, Shader::Link::Geometry::WIREFRAME_CREATE_INFO},
    // PARTICLE
    {SHADER_LINK::PRTCL_FOUNTAIN, Shader::Link::Particle::FOUNTAIN_CREATE_INFO},
    // CDLOD
    {SHADER_LINK::CDLOD, Shader::Link::Cdlod::CREATE_INFO},
};

const std::map<SHADER, std::set<SHADER_LINK>> LINK_MAP = {
    // This map is used to recompile the shaders on the fly.
    // VERTEX
    {SHADER::COLOR_VERT,
     {
         SHADER_LINK::UTILITY_VERT,
     }},
    {SHADER::TEX_VERT,
     {
         SHADER_LINK::UTILITY_VERT,
     }},
    {SHADER::CUBE_VERT,
     {
         SHADER_LINK::DEFAULT_MATERIAL,
     }},
    // FRAGMENT
    {SHADER::COLOR_FRAG,
     {
         SHADER_LINK::COLOR_FRAG,
         SHADER_LINK::UTILITY_FRAG,
         SHADER_LINK::BLINN_PHONG,
         SHADER_LINK::DEFAULT_MATERIAL,
     }},
    {SHADER::LINE_FRAG,
     {
         SHADER_LINK::UTILITY_FRAG,
     }},
    {SHADER::TEX_FRAG,
     {
         SHADER_LINK::TEX_FRAG,
         SHADER_LINK::UTILITY_FRAG,
         SHADER_LINK::BLINN_PHONG,
         SHADER_LINK::DEFAULT_MATERIAL,
     }},
    {SHADER::CUBE_FRAG,
     {
         SHADER_LINK::DEFAULT_MATERIAL,
         SHADER_LINK::UTILITY_FRAG,
     }},
    {SHADER::PBR_COLOR_FRAG,
     {
         SHADER_LINK::COLOR_FRAG,
         SHADER_LINK::PBR_FRAG,
         SHADER_LINK::PBR_MATERIAL,
     }},
    {SHADER::PBR_TEX_FRAG,
     {
         SHADER_LINK::TEX_FRAG,
         SHADER_LINK::PBR_FRAG,
         SHADER_LINK::PBR_MATERIAL,
     }},
    {SHADER::PARALLAX_SIMPLE_FRAG,
     {
         SHADER_LINK::DEFAULT_MATERIAL,
     }},
    {SHADER::PARALLAX_STEEP_FRAG,
     {
         SHADER_LINK::DEFAULT_MATERIAL,
     }},
    {SHADER::DEFERRED_MRT_TEX_FRAG,
     {
         SHADER_LINK::TEX_FRAG,
         SHADER_LINK::DEFAULT_MATERIAL,
     }},
    {SHADER::DEFERRED_MRT_COLOR_FRAG,
     {
         SHADER_LINK::COLOR_FRAG,
         SHADER_LINK::DEFAULT_MATERIAL,
         SHADER_LINK::GEOMETRY_FRAG,
     }},
    {SHADER::DEFERRED_MRT_POINT_FRAG,
     {
         SHADER_LINK::DEFAULT_MATERIAL,
     }},
    {SHADER::PRTCL_FOUNTAIN_VERT,
     {
         SHADER_LINK::DEFAULT_MATERIAL,
         SHADER_LINK::PRTCL_FOUNTAIN,
     }},
    {SHADER::PRTCL_FOUNTAIN_DEFERRED_MRT_FRAG,
     {
         SHADER_LINK::DEFAULT_MATERIAL,
     }},
    {SHADER::PRTCL_EULER_COMP,
     {
         SHADER_LINK::PRTCL_FOUNTAIN,
     }},
    {SHADER::PRTCL_FOUNTAIN_EULER_VERT,
     {
         SHADER_LINK::DEFAULT_MATERIAL,
         SHADER_LINK::PRTCL_FOUNTAIN,
     }},
    {SHADER::PRTCL_SHDW_FOUNTAIN_EULER_VERT,
     {
         SHADER_LINK::DEFAULT_MATERIAL,
     }},
    {SHADER::PRTCL_ATTR_VERT,
     {
         SHADER_LINK::DEFAULT_MATERIAL,
     }},
    {SHADER::CDLOD_VERT,
     {
         SHADER_LINK::CDLOD,
     }},
    {SHADER::CDLOD_TEX_VERT,
     {
         SHADER_LINK::CDLOD,
     }},
};

}  // namespace Shader