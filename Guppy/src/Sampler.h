#ifndef SAMPLER_H
#define SAMPLER_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Helpers.h"

class Shell;
typedef unsigned char stbi_uc;  // <stbi_image.h>

namespace Sampler {

typedef enum USE {
    COLOR = 0x00000001,  // DIFFUSE
    // THROUGH 0x00000008
    NORMAL = 0x00000010,
    // THROUGH 0x00000080
    SPECULAR = 0x00000100,
    // THROUGH 0x00000800
    ALPHA = 0x00001000,
    // THROUGH 0x00000800
    HEIGHT = 0x00010000,
    // THROUGH 0x00000800
} USE;

// Note: the values here are used for byte offsets,
// and other things that rely on them.
typedef enum CHANNELS {
    _1 = 1,
    _2 = 2,
    _3 = 3,
    _4 = 4,
} CHANNELS;

// { path, number of channels, combine offset }
typedef std::tuple<std::string, CHANNELS, uint8_t> combineInfo;

struct LayerInfo {
    USE type;
    std::string path;
    std::vector<combineInfo> combineInfos;
};

struct CreateInfo {
    std::string name = "";
    std::vector<LayerInfo> layerInfos;
    VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    VkImageCreateFlags imageFlags = 0;
    SAMPLER type = SAMPLER::DEFAULT;
    bool makeMipmaps = true;
    CHANNELS channels = CHANNELS::_4;
    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
};

class Base {
   public:  // TODO: use access modifiers
    Base(const Shell &shell, CreateInfo *pCreateInfo);

    void determineImageTypes();

    inline VkDeviceSize layerSize() const {
        return static_cast<VkDeviceSize>(width) *   //
               static_cast<VkDeviceSize>(height) *  //
               static_cast<VkDeviceSize>(NUM_CHANNELS);
    }
    inline VkDeviceSize size() const { return layerSize() * static_cast<VkDeviceSize>(arrayLayers); }

    void copyData(void *&pData, size_t &offset) const;
    inline void cleanup() { pPixels.clear(); }

    VkImageCreateInfo getImageCreateInfo();

    void destroy(const VkDevice &dev);

    const VkFormat FORMAT;
    const VkImageCreateFlags IMAGE_FLAGS;
    const std::string NAME;
    const FlagBits NUM_CHANNELS;
    const VkImageTiling TILING;
    const SAMPLER TYPE;

    FlagBits flags;  // TODO: remove this instead of passing a dynamic list to shaders?
    uint32_t width, height, mipLevels, arrayLayers;
    VkImageType imageType;
    VkImage image;
    VkDeviceMemory memory;
    VkImageViewType imageViewType;
    VkImageView view;
    VkSampler sampler;
    std::vector<stbi_uc *> pPixels;
    VkDescriptorImageInfo imgDescInfo;
};

// FUNCTIONS

/*	FlagBits mask for CHANNELS:
        _1 -> 0x01
        _2 -> 0x02
        _3 -> 0x04
        _4 -> 0x08
*/
inline FlagBits GetChannelMask(const CHANNELS &c) { return 1 << (c - 1); }

VkSamplerCreateInfo GetVkSamplerCreateInfo(const Sampler::Base &sampler);

LayerInfo GetDef4Comb3And1LayerInfo(const Sampler::USE &&type, const std::string &path, const std::string &fileName,
                                    const std::string &combineFileName = "");

}  // namespace Sampler

#endif  // !SAMPLER_H