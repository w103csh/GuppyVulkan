/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "TextureConstants.h"

#include <vulkan/vulkan.hpp>

#include "Random.h"
#include "Texture.h"

namespace Texture {

// STATUE
const CreateInfo STATUE_CREATE_INFO = {
    std::string(STATUE_ID),
    {Sampler::STATUE_CREATE_INFO},
};

// VULKAN LOGO
const CreateInfo VULKAN_CREATE_INFO = {
    std::string(VULKAN_ID),
    {Sampler::VULKAN_CREATE_INFO},
};

// HARDWOOD
const CreateInfo HARDWOOD_CREATE_INFO = {
    std::string(HARDWOOD_ID),
    {Sampler::HARDWOOD_CREATE_INFO},
};

// NEON BLUE TUX GUPPY
const CreateInfo NEON_BLUE_TUX_GUPPY_CREATE_INFO = {
    std::string(NEON_BLUE_TUX_GUPPY_ID),
    {Sampler::NEON_BLUE_TUX_GUPPY_CREATE_INFO},
};

// BLUEWATER
const CreateInfo BLUEWATER_CREATE_INFO = {
    std::string(BLUEWATER_ID),
    {Sampler::BLUEWATER_CREATE_INFO},
};

// FIRE
const CreateInfo FIRE_CREATE_INFO = {
    std::string(FIRE_ID),
    {Sampler::FIRE_CREATE_INFO},
};

// SMOKE
const CreateInfo SMOKE_CREATE_INFO = {
    std::string(SMOKE_ID),
    {Sampler::SMOKE_CREATE_INFO},
};

// STAR
const CreateInfo STAR_CREATE_INFO = {
    std::string(STAR_ID),
    {Sampler::STAR_CREATE_INFO},
};

// FABRIC BROWN
const CreateInfo FABRIC_BROWN_CREATE_INFO = {
    std::string(FABRIC_BROWN_ID),
    {Sampler::FABRIC_BROWN_CREATE_INFO},
};

// BRIGHT MOON
const CreateInfo BRIGHT_MOON_CREATE_INFO = {
    std::string(BRIGHT_MOON_ID),
    {Sampler::BRIGHT_MOON_CREATE_INFO},
};

// CIRCLES
const CreateInfo CIRCLES_CREATE_INFO = {
    std::string(CIRCLES_ID),
    {Sampler::CIRCLES_CREATE_INFO},
};

// MEDIEVAL HOUSE
const CreateInfo MEDIEVAL_HOUSE_CREATE_INFO = {
    std::string(MEDIEVAL_HOUSE_ID),
    {Sampler::MEDIEVAL_HOUSE_CREATE_INFO},
};

// WOOD
const CreateInfo WOOD_CREATE_INFO = {
    std::string(WOOD_ID),
    {Sampler::WOOD_CREATE_INFO},
};

// MYBRICK
const CreateInfo MYBRICK_CREATE_INFO = {
    std::string(MYBRICK_ID),
    {
        Sampler::MYBRICK_COLOR_CREATE_INFO,
        Sampler::MYBRICK_NORMAL_CREATE_INFO,
    },
};

// PISA HDR
const CreateInfo PISA_HDR_CREATE_INFO = {
    std::string(PISA_HDR_ID),
    {Sampler::PISA_HDR_CREATE_INFO},
};

// PISA HDR
const CreateInfo SKYBOX_CREATE_INFO = {
    std::string(SKYBOX_ID),
    {Sampler::SKYBOX_CREATE_INFO},
};

CreateInfo MakeRandom1dTex(const std::string_view& textureId, const std::string_view& samplerId, const uint32_t bufSize) {
    float* pData = (float*)malloc(bufSize * sizeof(float));

    for (uint32_t i = 0; i < bufSize; i++) {
        pData[i] = Random::inst().nextFloatZeroToOne();
    }

    Sampler::CreateInfo sampInfo = {
        std::string(samplerId),
        {{{::Sampler::USAGE::RANDOM}}},
        vk::ImageViewType::e1D,
        {bufSize, 1, 1},
        {},
        {},
        SAMPLER::DEFAULT_NEAREST,
        vk::ImageUsageFlagBits::eSampled,
        {{false, false}, 1},
        vk::Format::eR32Sfloat,
        Sampler::CHANNELS::_1,
        sizeof(float),
    };

    sampInfo.layersInfo.infos.front().pPixel = (stbi_uc*)pData;

    return {std::string(textureId), {sampInfo}, false};
}

CreateInfo MakeCubeMapTex(const std::string_view id, const SAMPLER type, const uint32_t size, const uint32_t numMaps) {
    assert(numMaps);

    Sampler::CreateInfo sampInfo = {
        std::string(id) + " Sampler",
        {{}, true, true},
        numMaps > 1 ? vk::ImageViewType::eCubeArray : vk::ImageViewType::eCube,
        {size, size, 1},
        {},
        vk::ImageCreateFlagBits::eCubeCompatible,
        type,
        vk::ImageUsageFlagBits::eSampled,
    };

    Sampler::LayerInfo layerInfo = {};
    DESCRIPTOR descType = COMBINED_SAMPLER::DONT_CARE;
    switch (type) {
        case SAMPLER::CLAMP_TO_BORDER_DEPTH:
        case SAMPLER::CLAMP_TO_BORDER_DEPTH_PCF: {
            sampInfo.imageViewType = vk::ImageViewType::eCubeArray;  // force this always?
            sampInfo.usage |= vk::ImageUsageFlagBits::eDepthStencilAttachment;
            layerInfo.type = Sampler::USAGE::DEPTH;
            descType = COMBINED_SAMPLER::PIPELINE_DEPTH;
        } break;
        case SAMPLER::DEFAULT:
        case SAMPLER::CUBE:
        case SAMPLER::CLAMP_TO_BORDER:
        case SAMPLER::CLAMP_TO_EDGE:
        case SAMPLER::DEFAULT_NEAREST: {
            sampInfo.usage |= vk::ImageUsageFlagBits::eColorAttachment;
            layerInfo.type = Sampler::USAGE::COLOR;
        } break;
        default: {
            assert(false);
            exit(EXIT_FAILURE);
        } break;
    }
    sampInfo.layersInfo.infos.assign(static_cast<size_t>(numMaps) * 6, layerInfo);

    return {std::string(id), {sampInfo}, false, false, descType};
}

}  // namespace Texture
