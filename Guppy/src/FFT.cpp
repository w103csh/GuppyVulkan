/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "FFT.h"

#include <string>
#include <vulkan/vulkan.h>

#include "Helpers.h"

// TEXUTRE
namespace Texture {
namespace FFT {

CreateInfo MakeTestTex() {
    uint32_t w = 4, h = 4;
    assert(helpers::isPowerOfTwo(w) && helpers::isPowerOfTwo(h));

    Sampler::CreateInfo sampInfo = {
        std::string(TEST_ID) + " Sampler",
        {{{::Sampler::USAGE::HEIGHT}, {::Sampler::USAGE::HEIGHT}}, true, true},
        VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        {w, h, 1},
        {},
        0,
        SAMPLER::DEFAULT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        {{false, false}, 1},
        VK_FORMAT_R32G32_SFLOAT,
        Sampler::CHANNELS::_2,
        sizeof(float),
    };

    sampInfo.layersInfo.infos.front().pPixel = new float[32]{
        //
        127.0f, 0.0f, 46.0f,  0.0f, 255.0f, 0.0f, 241.0f, 0.0f,  //
        176.0f, 0.0f, 179.0f, 0.0f, 70.0f,  0.0f, 241.0f, 0.0f,  //
        183.0f, 0.0f, 5.0f,   0.0f, 190.0f, 0.0f, 243.0f, 0.0f,  //
        196.0f, 0.0f, 157.0f, 0.0f, 136.0f, 0.0f, 94.0f,  0.0f,
    };
    sampInfo.layersInfo.infos.back().pPixel = new float[32]{};

    return {std::string(TEST_ID), {sampInfo}, false, false, STORAGE_IMAGE::DONT_CARE};
}

}  // namespace FFT
}  // namespace Texture

// SHADER
namespace Shader {
const CreateInfo FFT_ONE_COMP_CREATE_INFO = {
    SHADER::FFT_ONE_COMP,  //
    "Fast Fourier Transform One Component Compute Shader",
    "comp.fft.one.glsl",
    VK_SHADER_STAGE_COMPUTE_BIT,
    {SHADER_LINK::HELPERS},
};
}  // namespace Shader

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
const CreateInfo FFT_DEFAULT_CREATE_INFO = {
    DESCRIPTOR_SET::FFT_DEFAULT,
    "_DS_FFT",
    {{{0, 0}, {STORAGE_IMAGE::PIPELINE, Texture::FFT::TEST_ID}}},
};
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {
namespace FFT {

// ONE COMPONENT (COMPUTE)
const CreateInfo FFT_ONE_COMP_CREATE_INFO = {
    COMPUTE::FFT_ONE,
    "Fast Fourier Transform One Component Compute Pipeline",
    {SHADER::FFT_ONE_COMP},
    {DESCRIPTOR_SET::FFT_DEFAULT},
};
OneComponent::OneComponent(Handler& handler) : Compute(handler, &FFT_ONE_COMP_CREATE_INFO) {}

}  // namespace FFT
}  // namespace Pipeline