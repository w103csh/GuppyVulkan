/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "FFT.h"

#include <complex>
#include <cmath>
#include <string>
#include <vulkan/vulkan.hpp>

#include <Common/Helpers.h>

std::vector<int16_t> FFT::MakeBitReversalOffsets(const uint32_t N) {
    assert(N < INT16_MAX && helpers::isPowerOfTwo(N));

    std::vector<int16_t> offsets(N);
    int16_t log_2 = static_cast<int16_t>(log2(N));

    const auto reverse = [&log_2](int16_t i) -> int16_t {
        int16_t res = 0;
        for (int j = 0; j < log_2; j++) {
            res = (res << 1) + (i & 1);
            i >>= 1;
        }
        return res;
    };

    for (int16_t i = 0; i < static_cast<int16_t>(N); i++) offsets[i] = reverse(i);

    return offsets;
}

std::vector<float> FFT::MakeTwiddleFactors(uint32_t N) {
    assert(N < INT16_MAX && helpers::isPowerOfTwo(N));

    std::vector<std::complex<float>> ts;
    int16_t log_2 = static_cast<int16_t>(log2(N));

    std::complex<float> w, wm;
    for (int s = 1; s <= log_2; ++s) {
        int m = 1 << s;
        int m2 = m >> 1;
        w = 1.0f;
        wm = std::polar<float>(1.0f, glm::pi<float>() / m2);
        for (int j = 0; j < m2; ++j) {
            ts.push_back(w);
            w *= wm;
        }
    }

    std::vector<float> twiddleFactors;
    for (const auto& t : ts) {
        twiddleFactors.emplace_back(t.real());
        twiddleFactors.emplace_back(t.imag());
    }

    return twiddleFactors;
}

// TEXUTRE
namespace Texture {
namespace FFT {

CreateInfo MakeTestTex() {
    uint32_t w = 4, h = 4;
    assert(helpers::isPowerOfTwo(w) && helpers::isPowerOfTwo(h));

    Sampler::CreateInfo sampInfo = {
        std::string(TEST_ID) + " Sampler",
        {{{::Sampler::USAGE::HEIGHT}, {::Sampler::USAGE::HEIGHT}}, true, true},
        vk::ImageViewType::e2DArray,
        {w, h, 1},
        {},
        {},
        SAMPLER::DEFAULT,
        vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
        {{false, false}, 1},
        vk::Format::eR32G32Sfloat,
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
    vk::ShaderStageFlagBits::eCompute,
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
    {{DESCRIPTOR_SET::FFT_DEFAULT, vk::ShaderStageFlagBits::eCompute}},
};
OneComponent::OneComponent(Handler& handler) : Compute(handler, &FFT_ONE_COMP_CREATE_INFO) {}

}  // namespace FFT
}  // namespace Pipeline