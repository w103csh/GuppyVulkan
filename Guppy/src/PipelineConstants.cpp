
#include "PipelineConstants.h"

#include "ConstantsAll.h"
#include "Enum.h"

namespace Pipeline {

const std::vector<PIPELINE> ALL = {
    PIPELINE::TRI_LIST_COLOR,                //
    PIPELINE::LINE,                          //
    PIPELINE::TRI_LIST_TEX,                  //
    PIPELINE::CUBE,                          //
    PIPELINE::PBR_COLOR,                     //
    PIPELINE::PBR_TEX,                       //
    PIPELINE::BP_TEX_CULL_NONE,              //
    PIPELINE::PARALLAX_SIMPLE,               //
    PIPELINE::PARALLAX_STEEP,                //
    PIPELINE::SCREEN_SPACE_DEFAULT,          //
    PIPELINE::SCREEN_SPACE_COMPUTE_DEFAULT,  //
};

const std::map<VERTEX, std::set<PIPELINE>> VERTEX_MAP = {
    {
        VERTEX::COLOR,
        {

            PIPELINE::TRI_LIST_COLOR,
            PIPELINE::LINE,
            PIPELINE::PBR_COLOR,
            PIPELINE::CUBE,
            PIPELINE::SCREEN_SPACE_DEFAULT,
        },
    },
    {
        VERTEX::TEXTURE,
        {
            PIPELINE::TRI_LIST_TEX,
            PIPELINE::PBR_TEX,
            PIPELINE::BP_TEX_CULL_NONE,
            PIPELINE::PARALLAX_SIMPLE,
            PIPELINE::PARALLAX_STEEP,
        },
    },
    {
        VERTEX::DONT_CARE,
        {
            PIPELINE::SCREEN_SPACE_COMPUTE_DEFAULT,
        },
    },
};

// DEFAULT
namespace Default {

// TRIANGLE LIST COLOR
const Pipeline::CreateInfo TRI_LIST_COLOR_CREATE_INFO = {
    PIPELINE::TRI_LIST_COLOR,
    "Default Triangle List Color",
    {SHADER::COLOR_VERT, SHADER::COLOR_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_DEFAULT,
        // DESCRIPTOR_SET::PROJECTOR_DEFAULT,
    },
    // Descriptor::OffsetsMap::Type{
    //    {UNIFORM::FOG_DEFAULT, {1}},
    //    //{UNIFORM::LIGHT_POSITIONAL_DEFAULT, {2}},
    //},
};

// LINE
const Pipeline::CreateInfo LINE_CREATE_INFO = {
    PIPELINE::LINE,
    "Default Line",
    {SHADER::COLOR_VERT, SHADER::LINE_FRAG},
    {DESCRIPTOR_SET::UNIFORM_DEFAULT},
};

// TRIANGLE LIST TEXTURE
const Pipeline::CreateInfo TRI_LIST_TEX_CREATE_INFO = {
    PIPELINE::TRI_LIST_TEX,
    "Default Triangle List Texture",
    {SHADER::TEX_VERT, SHADER::TEX_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_DEFAULT,  //
        DESCRIPTOR_SET::SAMPLER_DEFAULT,
        // DESCRIPTOR_SET::PROJECTOR_DEFAULT,
    },
};

// CUBE
const Pipeline::CreateInfo CUBE_CREATE_INFO = {
    PIPELINE::CUBE,
    "Cube Pipeline",
    {SHADER::CUBE_VERT, SHADER::CUBE_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_DEFAULT,
        DESCRIPTOR_SET::SAMPLER_CUBE_DEFAULT,
    },
};

}  // namespace Default

// BLINN PHONG
namespace BP {

// TEXTURE CULL NONE
const Pipeline::CreateInfo TEX_CULL_NONE_CREATE_INFO = {
    PIPELINE::BP_TEX_CULL_NONE,
    "Blinn Phong Texture Cull None",
    {SHADER::TEX_VERT, SHADER::TEX_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_DEFAULT,  //
        DESCRIPTOR_SET::SAMPLER_DEFAULT,
        // DESCRIPTOR_SET::PROJECTOR_DEFAULT,
    },
};

}  // namespace BP

}  // namespace Pipeline