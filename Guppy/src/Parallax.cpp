
#include "Parallax.h"

#include "Instance.h"
#include "PipelineHandler.h"
#include "Vertex.h"

Descriptor::Set::Parallax::Uniform::Uniform()
    : Set::Base{DESCRIPTOR_SET::UNIFORM_PARALLAX,  //
                {
                    {{0, 0, 0}, {DESCRIPTOR::CAMERA_PERSPECTIVE_DEFAULT, {0}}},
                    {{0, 1, 0}, {DESCRIPTOR::MATERIAL_DEFAULT, {0}}},
                    {{0, 3, 0}, {DESCRIPTOR::LIGHT_POSITIONAL_DEFAULT, {OFFSET_ALL}}},  //
                }} {}

Shader::Parallax::Vertex::Vertex(Shader::Handler& handler)
    : Base{handler,                     //
           SHADER::PARALLAX_VERT,       //
           "parallax.vert.glsl",        //
           VK_SHADER_STAGE_VERTEX_BIT,  //
           "Parallax Vertex Shader"} {}

Shader::Parallax::SimpleFragment::SimpleFragment(Shader::Handler& handler)
    : Base{handler,                       //
           SHADER::PARALLAX_SIMPLE_FRAG,  //
           "parallax.frag.glsl",          //
           VK_SHADER_STAGE_FRAGMENT_BIT,  //
           "Parallax Simple Fragment Shader"} {}

Shader::Parallax::SteepFragment::SteepFragment(Shader::Handler& handler)
    : Base{handler,                       //
           SHADER::PARALLAX_STEEP_FRAG,   //
           "steep-parallax.frag.glsl",    //
           VK_SHADER_STAGE_FRAGMENT_BIT,  //
           "Parallax Steep Fragment Shader"} {}

Pipeline::Parallax::Simple::Simple(Pipeline::Handler& handler)
    : Base{
          handler,
          PIPELINE::PARALLAX_SIMPLE,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          "Parallax Simple Pipeline",
          {SHADER::PARALLAX_VERT, SHADER::PARALLAX_SIMPLE_FRAG},
          {},
          {DESCRIPTOR_SET::UNIFORM_PARALLAX, DESCRIPTOR_SET::SAMPLER_DEFAULT}  //
      } {};

void Pipeline::Parallax::Simple::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    GetDefaultTextureInputAssemblyInfoResources(createInfoRes);
}

Pipeline::Parallax::Steep::Steep(Pipeline::Handler& handler)
    : Base{
          handler,
          PIPELINE::PARALLAX_STEEP,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          "Parallax Steep Pipeline",
          {SHADER::PARALLAX_VERT, SHADER::PARALLAX_STEEP_FRAG},
          {},
          {DESCRIPTOR_SET::UNIFORM_PARALLAX, DESCRIPTOR_SET::SAMPLER_DEFAULT}  //
      } {};

void Pipeline::Parallax::Steep::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    GetDefaultTextureInputAssemblyInfoResources(createInfoRes);
}