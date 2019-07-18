#ifndef SAMPLER_H
#define SAMPLER_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "ConstantsAll.h"
#include "Helpers.h"

class Shell;
typedef unsigned char stbi_uc;  // <stbi_image.h>

namespace Sampler {

class Base {
   public:  // TODO: use access modifiers
    Base(const CreateInfo *pCreateInfo);

    void determineImageTypes();

    inline VkDeviceSize layerSize() const {
        return static_cast<VkDeviceSize>(extent.width) *   //
               static_cast<VkDeviceSize>(extent.height) *  //
               static_cast<VkDeviceSize>(NUM_CHANNELS);
    }
    inline VkDeviceSize size() const { return layerSize() * static_cast<VkDeviceSize>(arrayLayers); }

    void copyData(void *&pData, size_t &offset) const;
    inline void cleanup() { pPixels.clear(); }

    VkImageCreateInfo getImageCreateInfo();

    void destroy(const VkDevice &dev);

    // TODO: Get rid of all these creation info members

    const VkImageCreateFlags IMAGE_FLAGS;
    const std::string NAME;
    const FlagBits NUM_CHANNELS;
    const VkImageTiling TILING;
    const SAMPLER TYPE;

    FlagBits flags;  // TODO: remove this instead of passing a dynamic list to shaders?
    bool usesSwapchain;
    VkFormat format;
    VkSampleCountFlagBits samples;
    VkImageUsageFlags usage;
    VkExtent2D extent;
    float aspect;
    uint32_t mipLevels, arrayLayers;
    VkImageType imageType;
    VkImage image;
    VkDeviceMemory memory;
    VkImageViewType imageViewType;
    VkImageView view;
    VkSampler sampler;
    std::vector<stbi_uc *> pPixels;
    ImageInfo imgInfo;
};

// FUNCTIONS

Sampler::Base make(const Shell &shell, const CreateInfo *pCreateInfo, const bool hasData);

/*	FlagBits mask for CHANNELS:
        _1 -> 0x01
        _2 -> 0x02
        _3 -> 0x04
        _4 -> 0x08
*/
inline FlagBits GetChannelMask(const CHANNELS &c) { return 1 << (c - 1); }

VkSamplerCreateInfo GetVkSamplerCreateInfo(const Sampler::Base &sampler);

LayerInfo GetDef4Comb3And1LayerInfo(const Sampler::USAGE &&type, const std::string &path, const std::string &fileName,
                                    const std::string &combineFileName = "");

}  // namespace Sampler

#endif  // !SAMPLER_H