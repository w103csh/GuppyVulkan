/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Shadow.h"

#include "Random.h"

namespace {
constexpr uint32_t MAP_WIDTH = 1024;  // 540;
constexpr uint32_t MAP_HEIGHT = MAP_WIDTH;
constexpr uint32_t SAMPLES_U = 4;
constexpr uint32_t SAMPLES_V = 8;
constexpr uint32_t OFFSET_WIDTH = 8;
constexpr uint32_t OFFSET_HEIGHT = 8;
constexpr uint32_t OFFSET_DEPTH = SAMPLES_U * SAMPLES_V / 2;
constexpr float OFFSET_RADIUS = 7.0f / static_cast<float>(MAP_WIDTH);
}  // namespace

namespace Sampler {
namespace Shadow {

const CreateInfo MAP_2D_ARRAY_CREATE_INFO = {
    "Shadow 2D Array Map Sampler",
    {
        {
            {::Sampler::USAGE::DEPTH, "", true, true},
            //{::Sampler::USAGE::NORMAL, "", true, false},
        },
        true,
        true,
    },
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
    {MAP_WIDTH, MAP_HEIGHT, 1},
    {},
    0,
    // SAMPLER::CLAMP_TO_BORDER_DEPTH,
    SAMPLER::CLAMP_TO_BORDER_DEPTH_PCF,
    (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
};

}  // namespace Shadow
}  // namespace Sampler

namespace Texture {
namespace Shadow {

const CreateInfo MAP_2D_ARRAY_CREATE_INFO = {
    std::string(MAP_2D_ARRAY_ID),  //
    {Sampler::Shadow::MAP_2D_ARRAY_CREATE_INFO},
    false,
    false,
    COMBINED_SAMPLER::PIPELINE_DEPTH,
};

CreateInfo MakeOffsetTex() {
    uint32_t samples = SAMPLES_U * SAMPLES_V;
    uint32_t bufSize = OFFSET_WIDTH * OFFSET_HEIGHT * samples * 2;
    float* pData = (float*)malloc(bufSize * sizeof(float));

    for (uint32_t i = 0; i < OFFSET_WIDTH; i++) {
        for (uint32_t j = 0; j < OFFSET_HEIGHT; j++) {
            for (uint32_t k = 0; k < samples; k += 2) {
                int x1, y1, x2, y2;
                x1 = k % (SAMPLES_U);
                y1 = (samples - 1 - k) / SAMPLES_U;
                x2 = (k + 1) % SAMPLES_U;
                y2 = (samples - 1 - k - 1) / SAMPLES_U;

                glm::vec4 v;
                // Center on grid and jitter
                v.x = (x1 + 0.5f) + Random::inst().nextFloatNegZeroPoint5ToPosZeroPoint5();
                v.y = (y1 + 0.5f) + Random::inst().nextFloatNegZeroPoint5ToPosZeroPoint5();
                v.z = (x2 + 0.5f) + Random::inst().nextFloatNegZeroPoint5ToPosZeroPoint5();
                v.w = (y2 + 0.5f) + Random::inst().nextFloatNegZeroPoint5ToPosZeroPoint5();

                // Scale between 0 and 1
                v.x /= SAMPLES_U;
                v.y /= SAMPLES_V;
                v.z /= SAMPLES_U;
                v.w /= SAMPLES_V;

                // Warp to disk
                int cell = ((k / 2) * OFFSET_WIDTH * OFFSET_HEIGHT + j * OFFSET_WIDTH + i) * 4;
                pData[cell + 0] = sqrtf(v.y) * cosf(glm::two_pi<float>() * v.x);
                pData[cell + 1] = sqrtf(v.y) * sinf(glm::two_pi<float>() * v.x);
                pData[cell + 2] = sqrtf(v.w) * cosf(glm::two_pi<float>() * v.z);
                pData[cell + 3] = sqrtf(v.w) * sinf(glm::two_pi<float>() * v.z);
            }
        }
    }

    Sampler::CreateInfo sampInfo = {
        "Shadow Offset 2D Sampler",
        {{{::Sampler::USAGE::DONT_CARE}}},
        VK_IMAGE_VIEW_TYPE_3D,
        {OFFSET_WIDTH, OFFSET_HEIGHT, samples / 2},
        {},
        0,
        SAMPLER::DEFAULT_NEAREST,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        {{false, false}, 1},
        VK_FORMAT_R32G32B32A32_SFLOAT,
        Sampler::CHANNELS::_4,
        sizeof(float),
    };

    sampInfo.layersInfo.infos.front().pPixel = (stbi_uc*)pData;

    return {std::string(OFFSET_2D_ID), {sampInfo}, false};
}

}  // namespace Shadow
}  // namespace Texture

namespace Uniform {
namespace Shadow {

Base::Base(const Buffer::Info&& info, DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM::SHADOW_DATA),
      Buffer::DataItem<DATA>(pData) {
    pData_->data[0] = static_cast<float>(OFFSET_WIDTH);
    pData_->data[1] = static_cast<float>(OFFSET_HEIGHT);
    pData_->data[2] = static_cast<float>(OFFSET_DEPTH);
    pData_->data[3] = OFFSET_RADIUS;
    dirty = true;
}

}  // namespace Shadow
}  // namespace Uniform

namespace Light {
namespace Shadow {

Positional::Positional(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM::LIGHT_POSITIONAL_SHADOW),
      Buffer::PerFramebufferDataItem<DATA>(pData),
      proj_(pCreateInfo->proj) {
    update(pCreateInfo->mainCameraSpaceToWorldSpace);
}

void Positional::update(const glm::mat4& m, const uint32_t frameIndex) {
    data_.proj = proj_ * m;
    setData(frameIndex);
}

}  // namespace Shadow
}  // namespace Light

namespace Descriptor {
namespace Set {
namespace Shadow {

const CreateInfo UNIFORM_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_SHADOW,
    "_DS_UNI_SHDW",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
    },
};

const CreateInfo SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_SHADOW,
    "_DS_SMP_SHDW",
    {{{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Shadow::MAP_2D_ARRAY_ID}}},
};

const CreateInfo SAMPLER_OFFSET_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_SHADOW_OFFSET,
    "_DS_SMP_SHDW_OFF",
    {{{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Shadow::OFFSET_2D_ID}}},
};

}  // namespace Shadow
}  // namespace Set
}  // namespace Descriptor

namespace Pipeline {

void GetShadowRasterizationStateInfoResources(Pipeline::CreateInfoResources& createInfoRes) {
    createInfoRes.rasterizationStateInfo = {};
    createInfoRes.rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    createInfoRes.rasterizationStateInfo.lineWidth = 1.0f;
    createInfoRes.rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    createInfoRes.rasterizationStateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;  // This worked great !!!
    createInfoRes.rasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    /* If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far
     *  planes are clamped to them as opposed to discarding them. This is useful in some special
     *  cases like shadow maps. Using this requires enabling a GPU feature.
     */
    createInfoRes.rasterizationStateInfo.depthClampEnable = VK_FALSE;
    createInfoRes.rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
    /* I've seen literally no results with changing the constant factor. I probably just dont understand
     *  how it works. The slope factor on the other hand seemed to help tremendously. Ultimately the most
     *  success came from setting the cullMode to VK_CULL_MODE_FRONT_BIT which eliminated almost all of
     *  the "shadow acne".
     */
    createInfoRes.rasterizationStateInfo.depthBiasEnable = VK_TRUE;
    createInfoRes.rasterizationStateInfo.depthBiasConstantFactor = 0.0f;
    createInfoRes.rasterizationStateInfo.depthBiasClamp = 0.0f;
    createInfoRes.rasterizationStateInfo.depthBiasSlopeFactor = 0.0f;
}

namespace Shadow {

// COLOR
const Pipeline::CreateInfo COLOR_CREATE_INFO = {
    GRAPHICS::SHADOW_COLOR,
    "Shadow Color Pipeline",
    {SHADER::SHADOW_COLOR_VERT, SHADER::SHADOW_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_SHADOW,
    },
};
Color::Color(Pipeline::Handler& handler) : Graphics(handler, &COLOR_CREATE_INFO) {}

void Color::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    GetShadowRasterizationStateInfoResources(createInfoRes);
}

// TEXTURE
const Pipeline::CreateInfo TEX_CREATE_INFO = {
    GRAPHICS::SHADOW_TEX,
    "Shadow Texture Pipeline",
    {SHADER::SHADOW_TEX_VERT, SHADER::SHADOW_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_SHADOW,
    },
};
Texture::Texture(Pipeline::Handler& handler) : Graphics(handler, &TEX_CREATE_INFO) {}

void Texture::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    GetShadowRasterizationStateInfoResources(createInfoRes);
}

}  // namespace Shadow
}  // namespace Pipeline
