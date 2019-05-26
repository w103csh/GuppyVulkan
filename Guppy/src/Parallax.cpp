
#include "Parallax.h"

#include "Instance.h"
#include "PipelineHandler.h"
#include "Vertex.h"

Descriptor::Set::Parallax::Uniform::Uniform()
    : Set::Base{
          DESCRIPTOR_SET::UNIFORM_PARALLAX,
          "DSMI_UNI_PLX",
          {
              {{0, 0}, {DESCRIPTOR::CAMERA_PERSPECTIVE_DEFAULT, {0}, ""}},
              {{1, 0}, {DESCRIPTOR::MATERIAL_DEFAULT, {0}, ""}},
              {{3, 0}, {DESCRIPTOR::LIGHT_POSITIONAL_DEFAULT, {OFFSET_ALL}, ""}},
          },
      } {}

Descriptor::Set::Parallax::Sampler::Sampler()
    : Set::Base{
          DESCRIPTOR_SET::SAMPLER_PARALLAX,
          "DSMI_SMP_PLX",
          {
              {{0, 0}, {DESCRIPTOR::SAMPLER_MATERIAL_COMBINED, {0}, ""}},
              {{1, 0}, {DESCRIPTOR::SAMPLER_MATERIAL_COMBINED, {0}, ""}},
          },
      } {}

namespace Shader {
namespace Parallax {
const CreateInfo VERT_CREATE_INFO = {
    SHADER::PARALLAX_VERT,
    "Parallax Vertex Shader",
    "parallax.vert.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,
};
const CreateInfo SIMPLE_CREATE_INFO = {
    SHADER::PARALLAX_SIMPLE_FRAG,       //
    "Parallax Simple Fragment Shader",  //
    "parallax.frag.glsl",
    VK_SHADER_STAGE_FRAGMENT_BIT,
    {SHADER_LINK::DEFAULT_MATERIAL},
};
const CreateInfo STEEP_CREATE_INFO = {
    SHADER::PARALLAX_STEEP_FRAG,       //
    "Parallax Steep Fragment Shader",  //
    "steep-parallax.frag.glsl",        //
    VK_SHADER_STAGE_FRAGMENT_BIT,      //
    {SHADER_LINK::DEFAULT_MATERIAL},
};
}  // namespace Parallax
}  // namespace Shader

Pipeline::Parallax::Simple::Simple(Pipeline::Handler& handler)
    : Base{
          handler,
          PIPELINE::PARALLAX_SIMPLE,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          "Parallax Simple Pipeline",
          {SHADER::PARALLAX_VERT, SHADER::PARALLAX_SIMPLE_FRAG},
          {},
          {
              DESCRIPTOR_SET::UNIFORM_PARALLAX,
              DESCRIPTOR_SET::SAMPLER_PARALLAX,
          },
      } {};

Pipeline::Parallax::Steep::Steep(Pipeline::Handler& handler)
    : Base{
          handler,
          PIPELINE::PARALLAX_STEEP,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          "Parallax Steep Pipeline",
          {SHADER::PARALLAX_VERT, SHADER::PARALLAX_STEEP_FRAG},
          {},
          {
              DESCRIPTOR_SET::UNIFORM_PARALLAX,
              DESCRIPTOR_SET::SAMPLER_PARALLAX,
          },
      } {};
