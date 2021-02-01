/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Parallax.h"

#include <vulkan/vulkan.hpp>

#include "Instance.h"
#include "PipelineHandler.h"
#include "Vertex.h"

namespace Descriptor {
namespace Set {
namespace Parallax {
const CreateInfo UNIFORM_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_PARALLAX,
    "_DS_UNI_PLX",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_DEFAULT}},
        {{3, 0}, {UNIFORM::LIGHT_POSITIONAL_DEFAULT}},
    },
};
const CreateInfo SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_PARALLAX,
    "_DS_SMP_PLX",
    {
        {{0, 0}, {COMBINED_SAMPLER::MATERIAL, "0"}},
        {{1, 0}, {COMBINED_SAMPLER::MATERIAL, "1"}},
    },
};
}  // namespace Parallax
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
namespace Parallax {
const CreateInfo VERT_CREATE_INFO = {
    SHADER::PARALLAX_VERT,
    "Parallax Vertex Shader",
    "parallax.vert.glsl",
    vk::ShaderStageFlagBits::eVertex,
};
const CreateInfo SIMPLE_CREATE_INFO = {
    SHADER::PARALLAX_SIMPLE_FRAG,       //
    "Parallax Simple Fragment Shader",  //
    "parallax.frag.glsl",
    vk::ShaderStageFlagBits::eFragment,
    {SHADER_LINK::DEFAULT_MATERIAL},
};
const CreateInfo STEEP_CREATE_INFO = {
    SHADER::PARALLAX_STEEP_FRAG,         //
    "Parallax Steep Fragment Shader",    //
    "steep-parallax.frag.glsl",          //
    vk::ShaderStageFlagBits::eFragment,  //
    {SHADER_LINK::DEFAULT_MATERIAL},
};
}  // namespace Parallax
}  // namespace Shader

namespace Pipeline {
namespace Parallax {

const Pipeline::CreateInfo SIMPLE_CREATE_INFO = {
    GRAPHICS::PARALLAX_SIMPLE,
    "Parallax Simple Pipeline",
    {SHADER::PARALLAX_VERT, SHADER::PARALLAX_SIMPLE_FRAG},
    {
        {DESCRIPTOR_SET::UNIFORM_PARALLAX, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)},
        {DESCRIPTOR_SET::SAMPLER_PARALLAX, vk::ShaderStageFlagBits::eFragment},
    },
};

const Pipeline::CreateInfo STEEP_CREATE_INFO = {
    GRAPHICS::PARALLAX_STEEP,
    "Parallax Steep Pipeline",
    {SHADER::PARALLAX_VERT, SHADER::PARALLAX_STEEP_FRAG},
    {
        {DESCRIPTOR_SET::UNIFORM_PARALLAX, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)},
        {DESCRIPTOR_SET::SAMPLER_PARALLAX, vk::ShaderStageFlagBits::eFragment},
    },
};

}  // namespace Parallax
}  // namespace Pipeline
