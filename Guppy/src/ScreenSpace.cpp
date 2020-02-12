/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

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

DATA::DATA() : weights{}, edgeThreshold(0.5f), luminanceThreshold(1.7f) {
    // TODO: Sample for 5 weights used a sigma of 8. I should really just figure out exactly what the
    // math is doing here, so I can easily determine sigma and how many wieights are necessary based
    // on the cicumstance.
    // float sum, sigma2 = 8.0f;
    float sum, sigma2 = 25.0f;
    // Compute and sum the weights
    weights[0] = gauss(0, sigma2);
    sum = weights[0];
    for (int i = 1; i < 10; i++) {
        weights[i] = gauss(float(i), sigma2);
        sum += 2 * weights[i];
    }

    for (int i = 0; i < 10; i++) {
        weights[i] /= sum;
    }
}

Default::Default(const Buffer::Info&& info, DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Descriptor::Base(UNIFORM::SCREEN_SPACE_DEFAULT),
      Buffer::DataItem<DATA>(pData) {
    dirty = true;
}

}  // namespace ScreenSpace
}  // namespace Uniform

namespace Sampler {
namespace ScreenSpace {

const CreateInfo DEFAULT_2D_ARRAY_CREATE_INFO = {
    "Screen Space 2D Color Sampler",  //
    {{{::Sampler::USAGE::COLOR}}},
    vk::ImageViewType::e2D,  //
    BAD_EXTENT_3D,
    {true, true},
    {},
    SAMPLER::CLAMP_TO_BORDER,
    vk::ImageUsageFlagBits::eSampled,
    {{false, false}, 1},
};

const CreateInfo COMPUTE_2D_ARRAY_CREATE_INFO = {
    "Screen Space Compute 2D Color Sampler",
    {{{::Sampler::USAGE::COLOR}}},
    vk::ImageViewType::e2D,
    BAD_EXTENT_3D,
    {true, true},
    {},
    SAMPLER::CLAMP_TO_BORDER,
    vk::ImageUsageFlagBits::eStorage,
    {{false, false}, 1},
};

// LOG
const Sampler::CreateInfo HDR_LOG_SAMPLER_CREATE_INFO = {
    "Screen Space HDR Log 2D Sampler",
    {{{::Sampler::USAGE::COLOR}}},
    vk::ImageViewType::e2D,
    BAD_EXTENT_3D,
    {false, true},
    {},
    SAMPLER::CLAMP_TO_BORDER,
    (vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc),
    {{false, false}, 1},
    vk::Format::eR32Sfloat,
    Sampler::CHANNELS::_1,
};

// BLIT
const Sampler::CreateInfo HDR_LOG_BLIT_A_SAMPLER_CREATE_INFO = {
    "Screen Space HDR Log Blit A 2D Sampler",  //
    {{{::Sampler::USAGE::COLOR}}},
    vk::ImageViewType::e2D,
    BAD_EXTENT_3D,
    {false, true, 0.5f},
    {},
    SAMPLER::CLAMP_TO_EDGE,
    (vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst),
    {{false, true}, 1},
    vk::Format::eR32Sfloat,
};
const Sampler::CreateInfo HDR_LOG_BLIT_B_SAMPLER_CREATE_INFO = {
    "Screen Space HDR Log Blit B 2D Sampler",  //
    {{{::Sampler::USAGE::COLOR}}},
    vk::ImageViewType::e2D,
    BAD_EXTENT_3D,
    {false, true, 0.5f},
    {},
    SAMPLER::DEFAULT,
    (vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst),
    {{false, true}, 1},
    vk::Format::eR32Sfloat,
};

// BLUR
const Sampler::CreateInfo BLUR_2D_SAMPLER_CREATE_INFO = {
    "Screen Space Blur 2D Array Color Sampler",
    {{{::Sampler::USAGE::COLOR}}},
    vk::ImageViewType::e2D,
    BAD_EXTENT_3D,
    {false, true},
    {},
    SAMPLER::CLAMP_TO_BORDER,
    (vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment),
    {{false, false}, 1},
    vk::Format::eR32G32B32A32Sfloat,  // vk::Format::eR8G8B8A8Unorm,
};

}  // namespace ScreenSpace
}  // namespace Sampler

namespace Texture {
namespace ScreenSpace {

const CreateInfo DEFAULT_2D_TEXTURE_CREATE_INFO = {
    std::string(DEFAULT_2D_TEXTURE_ID),
    {Sampler::ScreenSpace::DEFAULT_2D_ARRAY_CREATE_INFO},
    false,
    true,
};

const CreateInfo COMPUTE_2D_TEXTURE_CREATE_INFO = {
    std::string(COMPUTE_2D_TEXTURE_ID),  //
    {Sampler::ScreenSpace::COMPUTE_2D_ARRAY_CREATE_INFO},
    false,
    true,
    STORAGE_IMAGE::DONT_CARE,
};

// LOG
const Texture::CreateInfo HDR_LOG_2D_TEXTURE_CREATE_INFO = {
    std::string(HDR_LOG_2D_TEXTURE_ID),
    {Sampler::ScreenSpace::HDR_LOG_SAMPLER_CREATE_INFO},
    false,
    true,
};

// BLIT A
const Texture::CreateInfo HDR_LOG_BLIT_A_2D_TEXTURE_CREATE_INFO = {
    std::string(HDR_LOG_BLIT_A_2D_TEXTURE_ID),
    {Sampler::ScreenSpace::HDR_LOG_BLIT_A_SAMPLER_CREATE_INFO},
    false,
    true,
};
// BLIT B
const Texture::CreateInfo HDR_LOG_BLIT_B_2D_TEXTURE_CREATE_INFO = {
    std::string(HDR_LOG_BLIT_B_2D_TEXTURE_ID),
    {Sampler::ScreenSpace::HDR_LOG_BLIT_B_SAMPLER_CREATE_INFO},
    false,
    true,
};

// BLUR A
const Texture::CreateInfo BLUR_A_2D_TEXTURE_CREATE_INFO = {
    std::string(BLUR_A_2D_TEXTURE_ID),
    {Sampler::ScreenSpace::BLUR_2D_SAMPLER_CREATE_INFO},
    false,
    true,
};
// BLUR B
const Texture::CreateInfo BLUR_B_2D_TEXTURE_CREATE_INFO = {
    std::string(BLUR_B_2D_TEXTURE_ID),
    {Sampler::ScreenSpace::BLUR_2D_SAMPLER_CREATE_INFO},
    false,
    true,
};

}  // namespace ScreenSpace
}  // namespace Texture

namespace Descriptor {
namespace Set {
namespace ScreenSpace {

const CreateInfo DEFAULT_UNIFORM_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_SCREEN_SPACE_DEFAULT,
    "_DS_UNI_SCR_DEF",
    {
        {{0, 0}, {UNIFORM::SCREEN_SPACE_DEFAULT}},
    },
};

const CreateInfo DEFAULT_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_DEFAULT,
    "_DS_SMP_SCR_DEF",
    {
        {{0, 0}, {COMBINED_SAMPLER::PIPELINE, ::RenderPass::DEFAULT_2D_TEXTURE_ID}},
        //{{1, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::ScreenSpace::DEFAULT_2D_TEXTURE_ID}},
    },
};

const CreateInfo HDR_BLIT_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_HDR_LOG_BLIT,
    "_DS_SMP_SCR_HDR_LOG_BLIT",
    {
        {{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::ScreenSpace::HDR_LOG_BLIT_A_2D_TEXTURE_ID}},
    },
};

const CreateInfo BLUR_A_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_BLUR_A,
    "_DS_SMP_SCR_BLUR_A",
    {
        {{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::ScreenSpace::BLUR_A_2D_TEXTURE_ID}},
    },
};
const CreateInfo BLUR_B_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_BLUR_B,
    "_DS_SMP_SCR_BLUR_B",
    {
        {{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::ScreenSpace::BLUR_B_2D_TEXTURE_ID}},
    },
};

const CreateInfo STORAGE_COMPUTE_POST_PROCESS_CREATE_INFO = {
    DESCRIPTOR_SET::STORAGE_SCREEN_SPACE_COMPUTE_POST_PROCESS,
    "_DS_STR_SCR_CMP_PSTPRC",
    {
        {{0, 0}, {STORAGE_BUFFER::POST_PROCESS}},
    },
};
const CreateInfo STORAGE_IMAGE_COMPUTE_DEFAULT_CREATE_INFO = {
    DESCRIPTOR_SET::STORAGE_IMAGE_SCREEN_SPACE_COMPUTE_DEFAULT,
    "_DS_STRIMG_SCR_CMP_DEF",
    {
        {{0, 0}, {STORAGE_IMAGE::PIPELINE, Texture::ScreenSpace::COMPUTE_2D_TEXTURE_ID}},
    },
};

}  // namespace ScreenSpace
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
namespace ScreenSpace {

const CreateInfo VERT_CREATE_INFO = {
    SHADER::SCREEN_SPACE_VERT,
    "Screen Space Vertex Shader",
    "screen.space.vert.glsl",
    vk::ShaderStageFlagBits::eVertex,
};

const CreateInfo FRAG_CREATE_INFO = {
    SHADER::SCREEN_SPACE_FRAG,
    "Screen Space Fragment Shader",
    "screen.space.frag.glsl",
    vk::ShaderStageFlagBits::eFragment,
    {
        SHADER_LINK::DEFAULT_MATERIAL,
    },
};

const CreateInfo FRAG_HDR_LOG_CREATE_INFO = {
    SHADER::SCREEN_SPACE_HDR_LOG_FRAG,
    "Screen Space HDR Log Fragment Shader",
    "screen.space.hdr.log.frag.glsl",
    vk::ShaderStageFlagBits::eFragment,
};

const CreateInfo FRAG_BRIGHT_CREATE_INFO = {
    SHADER::SCREEN_SPACE_BRIGHT_FRAG,
    "Screen Space Bright Fragment Shader",
    "screen.space.bright.frag.glsl",
    vk::ShaderStageFlagBits::eFragment,
};

const CreateInfo FRAG_BLUR_CREATE_INFO = {
    SHADER::SCREEN_SPACE_BLUR_FRAG,
    "Screen Space Blur Fragment Shader",
    "screen.space.blur.frag.glsl",
    vk::ShaderStageFlagBits::eFragment,
};

const CreateInfo COMP_CREATE_INFO = {
    SHADER::SCREEN_SPACE_COMP,
    "Screen Space Compute Shader",
    "comp.hdr.glsl",
    vk::ShaderStageFlagBits::eCompute,
};

}  // namespace ScreenSpace
}  // namespace Shader

namespace Pipeline {
namespace ScreenSpace {

// DEFAULT
const Pipeline::CreateInfo DEFAULT_CREATE_INFO = {
    GRAPHICS::SCREEN_SPACE_DEFAULT,
    "Screen Space Default Pipeline",
    {SHADER::SCREEN_SPACE_VERT, SHADER::SCREEN_SPACE_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_SCREEN_SPACE_DEFAULT,
        DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_DEFAULT,
        DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_HDR_LOG_BLIT,
        DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_BLUR_A,
    },
    {},
    {PUSH_CONSTANT::POST_PROCESS},
};
Default::Default(Pipeline::Handler& handler) : Graphics(handler, &DEFAULT_CREATE_INFO) {}

// HDR LOG
const Pipeline::CreateInfo HDR_LOG_CREATE_INFO = {
    GRAPHICS::SCREEN_SPACE_HDR_LOG,
    "Screen Space HDR Log Pipeline",
    {SHADER::SCREEN_SPACE_VERT, SHADER::SCREEN_SPACE_HDR_LOG_FRAG},
    {
        DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_DEFAULT,
    },
};
HdrLog::HdrLog(Pipeline::Handler& handler) : Graphics(handler, &HDR_LOG_CREATE_INFO) {}

// BRIGHT
const Pipeline::CreateInfo BRIGHT_CREATE_INFO = {
    GRAPHICS::SCREEN_SPACE_BRIGHT,
    "Screen Space Bright Pipeline",
    {SHADER::SCREEN_SPACE_VERT, SHADER::SCREEN_SPACE_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_SCREEN_SPACE_DEFAULT,
        DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_DEFAULT,
    },
    {},
    {PUSH_CONSTANT::POST_PROCESS},
};
Bright::Bright(Pipeline::Handler& handler) : Graphics(handler, &BRIGHT_CREATE_INFO) {}

// BLUR A
const Pipeline::CreateInfo BLUR_A_CREATE_INFO = {
    GRAPHICS::SCREEN_SPACE_BLUR_A,
    "Screen Space Blur A Pipeline",
    {SHADER::SCREEN_SPACE_VERT, SHADER::SCREEN_SPACE_BLUR_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_SCREEN_SPACE_DEFAULT,
        DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_BLUR_A,
    },
    {},
    {PUSH_CONSTANT::POST_PROCESS},
};
BlurA::BlurA(Pipeline::Handler& handler) : Graphics(handler, &BLUR_A_CREATE_INFO) {}

// BLUR B
const Pipeline::CreateInfo BLUR_B_CREATE_INFO = {
    GRAPHICS::SCREEN_SPACE_BLUR_B,
    "Screen Space Blur B Pipeline",
    {SHADER::SCREEN_SPACE_VERT, SHADER::SCREEN_SPACE_BLUR_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_SCREEN_SPACE_DEFAULT,
        DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_BLUR_B,
    },
    {},
    {PUSH_CONSTANT::POST_PROCESS},
};
BlurB::BlurB(Pipeline::Handler& handler) : Graphics(handler, &BLUR_B_CREATE_INFO) {}

// DEFAULT (COMPUTE)
const Pipeline::CreateInfo COMPUTE_DEFAULT_CREATE_INFO = {
    COMPUTE::SCREEN_SPACE_DEFAULT,
    "Screen Space Compute Default Pipeline",
    {SHADER::SCREEN_SPACE_COMP},
    {
        DESCRIPTOR_SET::STORAGE_SCREEN_SPACE_COMPUTE_POST_PROCESS,
        DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_DEFAULT,
        DESCRIPTOR_SET::SWAPCHAIN_IMAGE,
    },
    {},
    {PUSH_CONSTANT::POST_PROCESS},
};
ComputeDefault::ComputeDefault(Pipeline::Handler& handler) : Compute(handler, &COMPUTE_DEFAULT_CREATE_INFO) {}

}  // namespace ScreenSpace
}  // namespace Pipeline
