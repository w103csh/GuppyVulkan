
#include "ScreenSpace.h"

#include <glm/glm.hpp>

namespace {

inline float gauss(float x, float sigma2) {
    double coeff = 1.0 / (glm::two_pi<double>() * sigma2);
    double expon = -(x * x) / (2.0 * sigma2);
    return (float)(coeff * exp(expon));
}

}  // namespace

namespace Uniform {
namespace ScreenSpace {
DATA::DATA() : weights{}, edgeThreshold(0.5f) {
    float sum, sigma2 = 8.0f;

    // Compute and sum the weights
    weights[0] = gauss(0, sigma2);
    sum = weights[0];
    for (int i = 1; i < 5; i++) {
        weights[i] = gauss(float(i), sigma2);
        sum += 2 * weights[i];
    }

    // Normalize the weights and set the uniform
    for (int i = 0; i < 5; i++) {
        weights[i] /= sum;
    }
    auto x = 1;
}
Default::Default(const Buffer::Info&& info, DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Uniform::Base(),                                       //
      Buffer::DataItem<DATA>(pData)                          //
{
    DIRTY = true;
}
}  // namespace ScreenSpace
}  // namespace Uniform

namespace Descriptor {
namespace Set {
namespace ScreenSpace {
DefaultUniform::DefaultUniform()
    : Set::Base{
          DESCRIPTOR_SET::UNIFORM_SCREEN_SPACE_DEFAULT,
          "_DS_UNI_SCR_DEF",
          {
              // TODO: This works because of coincidence. Either use the default uniform or
              // create a new material.
              {{0, 0}, {UNIFORM::SCREEN_SPACE_DEFAULT, ""}},
              {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_DEFAULT, ""}},
          },
      } {}
DefaultSampler::DefaultSampler()
    : Set::Base{
          DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_DEFAULT,
          "_DS_SMP_SCR_DEF",
          {
              {{0, 0}, {COMBINED_SAMPLER::PIPELINE, RenderPass::DEFAULT_2D_TEXTURE_ID}},
              //{{1, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::ScreenSpace::DEFAULT_2D_TEXTURE_ID}},
          },
      } {}
}  // namespace ScreenSpace
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
namespace ScreenSpace {
const CreateInfo VERT_CREATE_INFO = {
    SHADER::SCREEN_SPACE_VERT,
    "Screen Space Vertex Shader",
    "screen.space.vert.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,
};
const CreateInfo FRAG_CREATE_INFO = {
    SHADER::SCREEN_SPACE_FRAG,
    "Screen Space Fragment Shader",
    "screen.space.frag.glsl",
    VK_SHADER_STAGE_FRAGMENT_BIT,
    {
        SHADER_LINK::DEFAULT_MATERIAL,
    },
};
}  // namespace ScreenSpace
}  // namespace Shader

namespace Pipeline {
namespace ScreenSpace {
const Pipeline::CreateInfo DEFAULT_CREATE_INFO = {
    PIPELINE::SCREEN_SPACE_DEFAULT,
    "Screen Space Pipeline",
    {SHADER::SCREEN_SPACE_VERT, SHADER::SCREEN_SPACE_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_SCREEN_SPACE_DEFAULT,
        DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_DEFAULT,
    },
};
Default::Default(Pipeline::Handler& handler) : Base(handler, &DEFAULT_CREATE_INFO) {}
}  // namespace ScreenSpace
}  // namespace Pipeline

namespace Sampler {
namespace ScreenSpace {
const CreateInfo DEFAULT_2D_ARRAY_CREATE_INFO = {
    "Render Pass Screen Space 2D Color Sampler",  //
    {{::Sampler::USAGE::COLOR}},                  //
    VK_IMAGE_VIEW_TYPE_2D,                        //
    0,
    SAMPLER::CLAMP_TO_BORDER,
    BAD_EXTENT_2D,
    false,
};
}
}  // namespace Sampler

namespace Texture {
namespace ScreenSpace {
const CreateInfo DEFAULT_2D_TEXTURE_CREATE_INFO = {
    std::string(DEFAULT_2D_TEXTURE_ID),
    {Sampler::ScreenSpace::DEFAULT_2D_ARRAY_CREATE_INFO},
    false,
};
}  // namespace ScreenSpace
}  // namespace Texture

namespace RenderPass {
namespace ScreenSpace {
const CreateInfo CREATE_INFO = {
    RENDER_PASS::SCREEN_SPACE,
    "Screen Space Swapchain Render Pass",
    {
        PIPELINE::SCREEN_SPACE_DEFAULT,
    },
    FLAG::SWAPCHAIN,
};
// const CreateInfo SAMPLER_CREATE_INFO = {
//    RENDER_PASS::SCREEN_SPACE_SAMPLER,
//    "Screen Space Sampler Render Pass",
//    {
//        PIPELINE::SCREEN_SPACE_DEFAULT,
//    },
//    FLAG::SWAPCHAIN,
//    {std::string(Texture::ScreenSpace::DEFAULT_2D_TEXTURE_ID)},
//};
}  // namespace ScreenSpace
}  // namespace RenderPass
