/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef SAMPLER_CONSTANTS_H
#define SAMPLER_CONSTANTS_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

#include <Common/Types.h>

#include "Constants.h"

using stbi_uc = unsigned char;  // <stbi_image.h>

enum class SAMPLER {
    //
    DEFAULT = 0,
    CUBE,
    CLAMP_TO_BORDER,
    CLAMP_TO_EDGE,
    CLAMP_TO_BORDER_DEPTH,
    CLAMP_TO_BORDER_DEPTH_PCF,
    DEFAULT_NEAREST,
};

namespace Sampler {

// clang-format off
using USAGE = enum : FlagBits {
    COLOR =         0x00000001,  // DIFFUSE
    HDR_LOG =       0x00000002,
    POSITION =      0x00000004,
    AMBIENT =       0x00000008,
    NORMAL =        0x00000010,
    DEPTH =         0x00000020,
    SPECULAR =      0x00000100,
    DONT_CARE =     0x00000200,
    FLAGS =         0x00000400,
    RANDOM =        0x00000400,
    ALPHA =         0x00001000,
    HEIGHT =        0x00010000,
};

// Note: the values here are used for byte offsets,
// and other things that rely on them.
using CHANNELS = enum : uint8_t {
    _1 = 1,
    _2 = 2,
    _3 = 3,
    _4 = 4,
};
// clang-format on

// { path, number of channels, combine offset }
using combineInfo = std::tuple<std::string, CHANNELS, uint8_t>;

uint32_t GetMipLevels(const vk::Extent3D& extent);

struct LayerInfo {
    USAGE type;
    std::string path;
    bool makeImageView = false;  // Flag for this layer
    bool makeSampler = false;    // Flag for this layer
    std::vector<combineInfo> combineInfos;
    void* pPixel = nullptr;
};

struct LayersInfo {
    bool hasData() const {
        for (const auto& info : infos)
            if (info.pPixel != nullptr) return true;
        return false;
    }
    std::vector<LayerInfo> infos;
    bool makeImageView = true;  // Flag for all layers
    bool makeSampler = true;    // Flag for all layers
};

// If usesFormat or usesExtent is true it will cause the texture
// to be destoryed and remade with the swapchain.
struct SwapchainInfo {
    constexpr bool usesSwapchain() const { return usesExtent || usesFormat; }
    bool usesFormat = false;
    bool usesExtent = false;
    float extentFactor = 1.0f;
    // TODO: This only kind of makes sense here...
    bool usesSamples = false;
};

struct MipmapInfo {
    bool generateMipmaps = true;
    bool usesExtent = true;
};

struct MipmapCreateInfo {
    MipmapInfo info;
    uint32_t mipLevels = 1;
};

struct CreateInfo {
    std::string name = "";
    LayersInfo layersInfo = {};
    vk::ImageViewType imageViewType = vk::ImageViewType::e2D;
    // Leave as bad only if the texture needs to pick up its extent
    // from a loaded image file, or the swapchain images.
    vk::Extent3D extent = BAD_EXTENT_3D;
    SwapchainInfo swpchnInfo = {};
    vk::ImageCreateFlags imageFlags;
    SAMPLER type = SAMPLER::DEFAULT;
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled;
    MipmapCreateInfo mipmapCreateInfo = {};
    vk::Format format = vk::Format::eR8G8B8A8Unorm;
    CHANNELS numberOfChannels = CHANNELS::_4;
    uint8_t bytesPerChannel = 1;
    vk::ImageTiling tiling = vk::ImageTiling::eOptimal;
};

constexpr uint32_t IMAGE_ARRAY_LAYERS_ALL = UINT32_MAX;
struct LayerResource {
    bool hasSampler = false;
    vk::ImageView view;
    vk::Sampler sampler;
};
using layerResourceMap = std::map<uint32_t, LayerResource>;

// CREATE INFOS

extern const CreateInfo STATUE_CREATE_INFO;
extern const CreateInfo VULKAN_CREATE_INFO;
extern const CreateInfo HARDWOOD_CREATE_INFO;
extern const CreateInfo NEON_BLUE_TUX_GUPPY_CREATE_INFO;
extern const CreateInfo BLUEWATER_CREATE_INFO;
extern const CreateInfo FIRE_CREATE_INFO;
extern const CreateInfo SMOKE_CREATE_INFO;
extern const CreateInfo STAR_CREATE_INFO;
extern const CreateInfo FABRIC_BROWN_CREATE_INFO;
extern const CreateInfo BRIGHT_MOON_CREATE_INFO;
extern const CreateInfo MEDIEVAL_HOUSE_CREATE_INFO;
extern const CreateInfo WOOD_CREATE_INFO;
extern const CreateInfo MYBRICK_COLOR_CREATE_INFO;
extern const CreateInfo MYBRICK_NORMAL_CREATE_INFO;
extern const CreateInfo PISA_HDR_CREATE_INFO;
extern const CreateInfo SKYBOX_CREATE_INFO;

}  // namespace Sampler

#endif  // !SAMPLER_CONSTANTS_H
