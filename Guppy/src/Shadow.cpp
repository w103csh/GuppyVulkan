/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Shadow.h"

#include "Camera.h"
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
        //{{::Sampler::USAGE::DEPTH, "", true, true}},
        {{::Sampler::USAGE::DEPTH}},
        true,
        true,
    },
    vk::ImageViewType::e2DArray,
    {MAP_WIDTH, MAP_HEIGHT, 1},
    {},
    {},
    // SAMPLER::CLAMP_TO_BORDER_DEPTH,
    // SAMPLER::CLAMP_TO_BORDER_DEPTH_PCF,
    // SAMPLER::CLAMP_TO_BORDER,
    SAMPLER::CLAMP_TO_EDGE,
    (vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eSampled),
    {},
    vk::Format::eD32Sfloat,
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
        vk::ImageViewType::e3D,
        {OFFSET_WIDTH, OFFSET_HEIGHT, samples / 2},
        {},
        {},
        SAMPLER::DEFAULT_NEAREST,
        vk::ImageUsageFlagBits::eSampled,
        {{false, false}, 1},
        vk::Format::eR32G32B32A32Sfloat,
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

namespace Positional {

Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM::LIGHT_POSITIONAL_SHADOW),
      Buffer::PerFramebufferDataItem<DATA>(pData),
      proj_(pCreateInfo->proj) {
    update(pCreateInfo->mainCameraSpaceToWorldSpace);
}

void Base::update(const glm::mat4& m, const uint32_t frameIndex) {
    data_.proj = proj_ * m;
    setData(frameIndex);
}

}  // namespace Positional

namespace Cube {

Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM::LIGHT_CUBE_SHADOW),
      Buffer::PerFramebufferDataItem<DATA>(pData),
      near_(pCreateInfo->n) {
    data_.flags = pCreateInfo->flags;
    data_.L = pCreateInfo->L;
    data_.La = pCreateInfo->La;
    data_.data0.w = pCreateInfo->f;
    proj_ = glm::perspective(glm::half_pi<float>(), 1.0f, near_, data_.data0.w);
    setPosition(glm::vec3(pCreateInfo->position));
}

void Base::setPosition(const glm::vec3 position, const uint32_t frameIndex) {
    // position
    data_.data0.x = position.x;
    data_.data0.y = position.y;
    data_.data0.z = position.z;
    // views
    setViews();
    // view projections
    for (uint32_t i = 0; i < LAYERS; i++) {
        data_.viewProjs[i] = proj_ * views_[i];  // * model_; Use the model?
    }
    setData(frameIndex);
}

void Base::update(glm::vec3&& position, const uint32_t frameIndex) {
    data_.cameraPosition = position;
    setData(frameIndex);
}

void Base::setViews() {
    const glm::vec3 position = glm::vec3(data_.data0);
    // cube map view transforms
    views_[0] = glm::lookAt(position, position + CARDINAL_X, -CARDINAL_Y);
    views_[1] = glm::lookAt(position, position - CARDINAL_X, -CARDINAL_Y);
    views_[2] = glm::lookAt(position, position + CARDINAL_Y, CARDINAL_Z);
    views_[3] = glm::lookAt(position, position - CARDINAL_Y, -CARDINAL_Z);
    views_[4] = glm::lookAt(position, position + CARDINAL_Z, -CARDINAL_Y);
    views_[5] = glm::lookAt(position, position - CARDINAL_Z, -CARDINAL_Y);
}

}  // namespace Cube

}  // namespace Shadow
}  // namespace Light

namespace Descriptor {
namespace Set {
namespace Shadow {

const CreateInfo CAMERA_BASIC_ONLY_CREATE_INFO = {
    DESCRIPTOR_SET::CAMERA_BASIC_ONLY,
    "_DS_CAM_BSC_ONLY",
    {{{0, 0}, {UNIFORM_DYNAMIC::CAMERA_PERSPECTIVE_BASIC}}},
};

const CreateInfo CUBE_UNIFORM_ONLY_CREATE_INFO = {
    DESCRIPTOR_SET::SHADOW_CUBE_UNIFORM_ONLY,
    "_DS_SHDW_CUBE_UNI_ONLY",
    {{{0, 0}, {UNIFORM::LIGHT_CUBE_SHADOW}}},
};

const CreateInfo CUBE_ALL_CREATE_INFO = {
    DESCRIPTOR_SET::SHADOW_CUBE_ALL,
    "_DS_SHDW_CUBE_ALL",
    {
        {{0, 0}, {COMBINED_SAMPLER::PIPELINE_DEPTH, Texture::Shadow::MAP_CUBE_ARRAY_ID}},
        {{1, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Shadow::OFFSET_2D_ID}},
        {{2, 0}, {UNIFORM::LIGHT_CUBE_SHADOW}},
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
    createInfoRes.rasterizationStateInfo = vk::PipelineRasterizationStateCreateInfo{};
    createInfoRes.rasterizationStateInfo.lineWidth = 1.0f;
    createInfoRes.rasterizationStateInfo.polygonMode = vk::PolygonMode::eFill;
    createInfoRes.rasterizationStateInfo.cullMode = vk::CullModeFlagBits::eBack;
    /**
     * THIS SETTING IS SPECIFICALLY FOR RENDER TO CUBE MAP TEXTURE LOOKUP/CAMERA CREATION !!!
     *
     * Setting clockwise here allow a couple of things to happen for a specific reason. In order to the cube map view
     * matrices to play nicely with the GLSL texture lookup functions I had to switch from a left-handed coordinate system to
     * a right-handed one (I think...). But because all of my pipelines, and camera's and face winding were setup for a
     * left-handed system it was easier to just reverse the winding here as the final piece of making it all work correctly.
     * There might be a simpler solution, but I couldn't find one at the time.
     */
    createInfoRes.rasterizationStateInfo.frontFace = vk::FrontFace::eClockwise;
    /**
     * If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far
     *  planes are clamped to them as opposed to discarding them. This is useful in some special
     *  cases like shadow maps. Using this requires enabling a GPU feature.
     */
    createInfoRes.rasterizationStateInfo.depthClampEnable = VK_FALSE;
    createInfoRes.rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
    /**
     *I've seen literally no results with changing the constant factor. I probably just dont understand
     *  how it works. The slope factor on the other hand seemed to help tremendously. Ultimately the most
     *  success came from setting the cullMode to vk::CullMode::eFront which eliminated almost all of
     *  the "shadow acne".
     */
    createInfoRes.rasterizationStateInfo.depthBiasEnable = VK_FALSE;
    createInfoRes.rasterizationStateInfo.depthBiasConstantFactor = 0.0f;
    createInfoRes.rasterizationStateInfo.depthBiasClamp = 0.0f;
    createInfoRes.rasterizationStateInfo.depthBiasSlopeFactor = 0.0f;
}

namespace Shadow {

// COLOR
const Pipeline::CreateInfo COLOR_CREATE_INFO = {
    GRAPHICS::SHADOW_COLOR,
    "Shadow Color Pipeline",
    {
        SHADER::SHADOW_COLOR_VERT,
    },
    {{DESCRIPTOR_SET::CAMERA_BASIC_ONLY, vk::ShaderStageFlagBits::eVertex}},
};
Color::Color(Pipeline::Handler& handler) : Graphics(handler, &COLOR_CREATE_INFO) {}
void Color::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    GetShadowRasterizationStateInfoResources(createInfoRes);
}

// TEXTURE
const Pipeline::CreateInfo TEX_CREATE_INFO = {
    GRAPHICS::SHADOW_TEX,
    "Shadow Texture Pipeline",
    {
        SHADER::SHADOW_TEX_VERT,
    },
    {{DESCRIPTOR_SET::CAMERA_BASIC_ONLY, vk::ShaderStageFlagBits::eVertex}},
};
Texture::Texture(Pipeline::Handler& handler) : Graphics(handler, &TEX_CREATE_INFO) {}
void Texture::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    GetShadowRasterizationStateInfoResources(createInfoRes);
}

// COLOR CUBEMAP
const Pipeline::CreateInfo COLOR_CUBE_CREATE_INFO = {
    GRAPHICS::SHADOW_COLOR_CUBE,
    "Shadow Color Cubemap Pipeline",
    {
        SHADER::SHADOW_COLOR_CUBE_VERT,
        SHADER::SHADOW_CUBE_GEOM,
        SHADER::SHADOW_FRAG,
    },
    {{DESCRIPTOR_SET::SHADOW_CUBE_UNIFORM_ONLY, vk::ShaderStageFlagBits::eGeometry}},
};
ColorCube::ColorCube(Pipeline::Handler& handler) : Graphics(handler, &COLOR_CUBE_CREATE_INFO) {}
void ColorCube::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    GetShadowRasterizationStateInfoResources(createInfoRes);
}

// TEXTURE CUBEMAP
const Pipeline::CreateInfo TEX_CUBE_CREATE_INFO = {
    GRAPHICS::SHADOW_TEX_CUBE,
    "Shadow Texture Cubemap Pipeline",
    {
        SHADER::SHADOW_TEX_CUBE_VERT,
        SHADER::SHADOW_CUBE_GEOM,
        SHADER::SHADOW_FRAG,
    },
    {{DESCRIPTOR_SET::SHADOW_CUBE_UNIFORM_ONLY, vk::ShaderStageFlagBits::eGeometry}},
};
TextureCube::TextureCube(Pipeline::Handler& handler) : Graphics(handler, &TEX_CUBE_CREATE_INFO) {}
void TextureCube::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    GetShadowRasterizationStateInfoResources(createInfoRes);
}

}  // namespace Shadow
}  // namespace Pipeline
