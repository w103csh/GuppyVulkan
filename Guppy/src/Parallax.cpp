
#include "Parallax.h"

#include "Instance.h"
#include "PipelineHandler.h"
#include "Vertex.h"

Descriptor::Set::Parallax::Uniform::Uniform(Handler& handler)
    : Set::Base{
          handler,
          DESCRIPTOR_SET::UNIFORM_PARALLAX,
          "_DS_UNI_PLX",
          {
              {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
              {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_DEFAULT}},
              {{3, 0}, {UNIFORM::LIGHT_POSITIONAL_DEFAULT}},
          },
      } {}

Descriptor::Set::Parallax::Sampler::Sampler(Handler& handler)
    : Set::Base{
          handler,
          DESCRIPTOR_SET::SAMPLER_PARALLAX,
          "_DS_SMP_PLX",
          {
              {{0, 0}, {COMBINED_SAMPLER::MATERIAL, "0"}},
              {{1, 0}, {COMBINED_SAMPLER::MATERIAL, "1"}},
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

namespace Pipeline {
namespace Parallax {

const Pipeline::CreateInfo SIMPLE_CREATE_INFO = {
    PIPELINE::PARALLAX_SIMPLE,
    "Parallax Simple Pipeline",
    {SHADER::PARALLAX_VERT, SHADER::PARALLAX_SIMPLE_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_PARALLAX,
        DESCRIPTOR_SET::SAMPLER_PARALLAX,
    },
};

const Pipeline::CreateInfo STEEP_CREATE_INFO = {
    PIPELINE::PARALLAX_STEEP,
    "Parallax Steep Pipeline",
    {SHADER::PARALLAX_VERT, SHADER::PARALLAX_STEEP_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_PARALLAX,
        DESCRIPTOR_SET::SAMPLER_PARALLAX,
    },
};

}  // namespace Parallax
}  // namespace Pipeline
