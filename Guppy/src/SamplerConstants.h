
#ifndef SAMPLER_CONSTANTS_H
#define SAMPLER_CONSTANTS_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

#include "Constants.h"

enum class SAMPLER {
    //
    DEFAULT = 0,
    CUBE,
    CLAMP_TO_BORDER,
    CLAMP_TO_EDGE,
};

namespace Sampler {

// clang-format off
using USAGE = enum {
    COLOR =         0x00000001,  // DIFFUSE
    HDR_LOG =       0x00000002,
    NORMAL =        0x00000010,
    SPECULAR =      0x00000100,
    ALPHA =         0x00001000,
    HEIGHT =        0x00010000,
};
// Note: the values here are used for byte offsets,
// and other things that rely on them.
using CHANNELS = enum {
    _1 = 1,
    _2 = 2,
    _3 = 3,
    _4 = 4,
};
// clang-format on

// { path, number of channels, combine offset }
using combineInfo = std::tuple<std::string, CHANNELS, uint8_t>;

uint32_t GetMipLevels(const VkExtent2D& extent);

struct LayerInfo {
    USAGE type;
    std::string path;
    std::vector<combineInfo> combineInfos;
};

// If usesFormat or usesExtent is true it will cause the texture
// to be destoryed and remade with the swapchain.
struct SwapchainInfo {
    constexpr bool usesSwapchain() const { return usesExtent || usesFormat; }
    bool usesFormat = false;
    bool usesExtent = false;
    float extentFactor = 1.0f;
};

struct MipmapInfo {
    bool generateMipmaps = true;
    bool usesExtent = true;
    uint32_t mipLevels = 1;
};

struct CreateInfo {
    std::string name = "";
    std::vector<LayerInfo> layerInfos;
    VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    // Leave as bad only if the texture needs to pick up its extent
    // from a loaded image file, or the swapchain images.
    VkExtent2D extent = BAD_EXTENT_2D;
    SwapchainInfo swpchnInfo = {};
    VkImageCreateFlags imageFlags = 0;
    SAMPLER type = SAMPLER::DEFAULT;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    MipmapInfo mipmapInfo = {};
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    CHANNELS channels = CHANNELS::_4;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
};

// CREATE INFOS

extern const CreateInfo STATUE_CREATE_INFO;
extern const CreateInfo VULKAN_CREATE_INFO;
extern const CreateInfo HARDWOOD_CREATE_INFO;
extern const CreateInfo MEDIEVAL_HOUSE_CREATE_INFO;
extern const CreateInfo WOOD_CREATE_INFO;
extern const CreateInfo MYBRICK_COLOR_CREATE_INFO;
extern const CreateInfo MYBRICK_NORMAL_CREATE_INFO;
extern const CreateInfo PISA_HDR_CREATE_INFO;
extern const CreateInfo SKYBOX_CREATE_INFO;

}  // namespace Sampler

#endif  // !SAMPLER_CONSTANTS_H
