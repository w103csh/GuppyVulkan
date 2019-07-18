
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

struct LayerInfo {
    USAGE type;
    std::string path;
    std::vector<combineInfo> combineInfos;
};

struct CreateInfo {
    std::string name = "";
    std::vector<LayerInfo> layerInfos;
    VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    VkImageCreateFlags imageFlags = 0;
    SAMPLER type = SAMPLER::DEFAULT;
    VkExtent2D extent = BAD_EXTENT_2D;
    bool makeMipmaps = true;
    bool usesSwapchain = false;
    VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    CHANNELS channels = CHANNELS::_4;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
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
