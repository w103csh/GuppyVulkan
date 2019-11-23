
#ifndef SAMPLER_CONSTANTS_H
#define SAMPLER_CONSTANTS_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Types.h"

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

uint32_t GetMipLevels(const VkExtent3D& extent);

struct LayerInfo {
    USAGE type;
    std::string path;
    bool makeImageView = false;  // Flag for this layer
    bool makeSampler = false;    // Flag for this layer
    std::vector<combineInfo> combineInfos;
    stbi_uc* pPixel = nullptr;
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
    VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    // Leave as bad only if the texture needs to pick up its extent
    // from a loaded image file, or the swapchain images.
    VkExtent3D extent = BAD_EXTENT_3D;
    SwapchainInfo swpchnInfo = {};
    VkImageCreateFlags imageFlags = 0;
    SAMPLER type = SAMPLER::DEFAULT;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    MipmapCreateInfo mipmapCreateInfo = {};
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    CHANNELS numberOfChannels = CHANNELS::_4;
    uint8_t bytesPerChannel = 1;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
};

constexpr uint32_t IMAGE_ARRAY_LAYERS_ALL = UINT32_MAX;
struct LayerResource {
    bool hasSampler = false;
    VkImageView view = VK_NULL_HANDLE;
    VkSampler sampler = VK_NULL_HANDLE;
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
extern const CreateInfo MEDIEVAL_HOUSE_CREATE_INFO;
extern const CreateInfo WOOD_CREATE_INFO;
extern const CreateInfo MYBRICK_COLOR_CREATE_INFO;
extern const CreateInfo MYBRICK_NORMAL_CREATE_INFO;
extern const CreateInfo PISA_HDR_CREATE_INFO;
extern const CreateInfo SKYBOX_CREATE_INFO;

}  // namespace Sampler

#endif  // !SAMPLER_CONSTANTS_H
