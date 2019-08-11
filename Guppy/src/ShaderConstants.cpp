
#include "ShaderConstants.h"

#include "Deferred.h"
#include "PBR.h"
#include "Parallax.h"
#include "ScreenSpace.h"

//#define MULTILINE(...) #__VA_ARGS__

namespace Shader {

// SHADER INFO

const CreateInfo COLOR_VERT_CREATE_INFO = {
    SHADER::COLOR_VERT,             //
    "Default Color Vertex Shader",  //
    "color.vert",                   //
    VK_SHADER_STAGE_VERTEX_BIT,     //
    {SHADER_LINK::UTILITY_VERT},
};

const CreateInfo COLOR_FRAG_CREATE_INFO = {
    SHADER::COLOR_FRAG,
    "Default Color Fragment Shader",
    "color.frag",
    VK_SHADER_STAGE_FRAGMENT_BIT,
    {
        SHADER_LINK::COLOR_FRAG,
        SHADER_LINK::UTILITY_FRAG,
        SHADER_LINK::BLINN_PHONG,
        SHADER_LINK::DEFAULT_MATERIAL,
    },
};

const CreateInfo LINE_FRAG_CREATE_INFO = {
    SHADER::LINE_FRAG,               //
    "Default Line Fragment Shader",  //
    "line.frag",                     //
    VK_SHADER_STAGE_FRAGMENT_BIT,    //
    {SHADER_LINK::UTILITY_FRAG},
};

const CreateInfo TEX_VERT_CREATE_INFO = {
    SHADER::TEX_VERT,                 //
    "Default Texture Vertex Shader",  //
    "texture.vert",                   //
    VK_SHADER_STAGE_VERTEX_BIT,       //
    {SHADER_LINK::UTILITY_VERT},
};

const CreateInfo TEX_FRAG_CREATE_INFO = {
    SHADER::TEX_FRAG,
    "Default Texture Fragment Shader",
    "texture.frag",
    VK_SHADER_STAGE_FRAGMENT_BIT,
    {
        SHADER_LINK::TEX_FRAG,
        SHADER_LINK::UTILITY_FRAG,
        SHADER_LINK::BLINN_PHONG,
        SHADER_LINK::DEFAULT_MATERIAL,
    },
};

const CreateInfo CUBE_VERT_CREATE_INFO = {
    SHADER::CUBE_VERT,           //
    "Cube Vertex Shader",        //
    "cube.vert",                 //
    VK_SHADER_STAGE_VERTEX_BIT,  //
    {SHADER_LINK::DEFAULT_MATERIAL},
};

const CreateInfo CUBE_FRAG_CREATE_INFO = {
    SHADER::CUBE_FRAG,             //
    "Cube Fragment Shader",        //
    "cube.frag",                   //
    VK_SHADER_STAGE_FRAGMENT_BIT,  //
    {
        SHADER_LINK::DEFAULT_MATERIAL,
        SHADER_LINK::UTILITY_FRAG,
    },
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

}  // namespace Link

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
    {SHADER::COLOR_FRAG, Shader::COLOR_FRAG_CREATE_INFO},
    {SHADER::LINE_FRAG, Shader::LINE_FRAG_CREATE_INFO},
    {SHADER::TEX_VERT, Shader::TEX_VERT_CREATE_INFO},
    {SHADER::TEX_FRAG, Shader::TEX_FRAG_CREATE_INFO},
    {SHADER::CUBE_VERT, Shader::CUBE_VERT_CREATE_INFO},
    {SHADER::CUBE_FRAG, Shader::CUBE_FRAG_CREATE_INFO},
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
    {SHADER::DEFERRED_MRT_VERT, Shader::Deferred::MRT_VERT_CREATE_INFO},
    {SHADER::DEFERRED_MRT_FRAG, Shader::Deferred::MRT_FRAG_CREATE_INFO},
};

const std::map<SHADER_LINK, Shader::Link::CreateInfo> LINK_ALL = {
    // DEFAULT
    {SHADER_LINK::COLOR_FRAG, Shader::Link::COLOR_FRAG_CREATE_INFO},
    {SHADER_LINK::TEX_FRAG, Shader::Link::TEX_FRAG_CREATE_INFO},
    {SHADER_LINK::UTILITY_VERT, Shader::Link::UTILITY_VERT_CREATE_INFO},
    {SHADER_LINK::UTILITY_FRAG, Shader::Link::UTILITY_FRAG_CREATE_INFO},
    {SHADER_LINK::BLINN_PHONG, Shader::Link::BLINN_PHONG_CREATE_INFO},
    {SHADER_LINK::DEFAULT_MATERIAL, Shader::Link::DEFAULT_MATERIAL_CREATE_INFO},
    // PBR
    {SHADER_LINK::PBR_FRAG, Shader::Link::PBR::FRAG_CREATE_INFO},
    {SHADER_LINK::PBR_MATERIAL, Shader::Link::PBR::MATERIAL_CREATE_INFO},
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
};

}  // namespace Shader