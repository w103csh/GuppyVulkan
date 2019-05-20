
#include "ShaderConstants.h"

const std::vector<SHADER> SHADER_ALL = {
    // DEFAULT
    SHADER::COLOR_VERT,  //
    SHADER::COLOR_FRAG,  //
    SHADER::LINE_FRAG,   //
    SHADER::TEX_VERT,    //
    SHADER::TEX_FRAG,    //
    SHADER::CUBE,        //
    // PBR
    SHADER::PBR_COLOR_FRAG,  //
    SHADER::PBR_TEX_FRAG,    //
    // PARALLAX
    SHADER::PARALLAX_VERT,         //
    SHADER::PARALLAX_SIMPLE_FRAG,  //
    SHADER::PARALLAX_STEEP_FRAG,   //
};

const std::vector<SHADER_LINK> SHADER_LINK_ALL = {
    // These need to be in same order as the enum
    SHADER_LINK::COLOR_FRAG,        //
    SHADER_LINK::TEX_FRAG,          //
    SHADER_LINK::BLINN_PHONG_FRAG,  //
    SHADER_LINK::DEFAULT_MATERIAL,  //
    SHADER_LINK::PBR_FRAG,          //
    SHADER_LINK::PBR_MATERIAL,      //
};

const std::map<SHADER, std::set<SHADER_LINK>> SHADER_LINK_MAP = {
    // TODO: why is this necessary? it seems like it shouldn't be.
    {SHADER::COLOR_VERT,
     {
         SHADER_LINK::DEFAULT_MATERIAL,
     }},
    {SHADER::COLOR_FRAG,
     {
         SHADER_LINK::COLOR_FRAG,
         SHADER_LINK::BLINN_PHONG_FRAG,
         SHADER_LINK::DEFAULT_MATERIAL,
     }},
    {SHADER::TEX_FRAG,
     {
         SHADER_LINK::TEX_FRAG,
         SHADER_LINK::BLINN_PHONG_FRAG,
         SHADER_LINK::DEFAULT_MATERIAL,
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
};