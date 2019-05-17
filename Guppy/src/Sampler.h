#ifndef SAMPLER_H
#define SAMPLER_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"

typedef unsigned char stbi_uc;  // <stbi_image.h>

namespace Sampler {

typedef enum CHANNELS {
    _1 = 1,
    _2 = 2,
    _3 = 3,
    _4 = 4,
} CHANNELS;

/*	FlagBits mask for CHANNELS:
        _1 -> 0x01
        _2 -> 0x02
        _3 -> 0x04
        _4 -> 0x08
*/
static FlagBits getChannelMask(const CHANNELS &c) { return 1 << (c - 1); }

struct CreateInfo {
    CHANNELS channels;
    VkFormat format;
    uint32_t binding;
};

class Base {
   public:
    Base(const std::string &name, CreateInfo createInfo);

    inline uint32_t layerCount() const { return static_cast<uint32_t>(pPixels.size()); }
    inline VkDeviceSize layerSize() const {
        return static_cast<VkDeviceSize>(width) *   //
               static_cast<VkDeviceSize>(height) *  //
               static_cast<VkDeviceSize>(NUM_CHANNELS);
    }
    inline VkDeviceSize size() const { return layerSize() * static_cast<VkDeviceSize>(layerCount()); }
    void copyData(void *&pData, size_t &offset) const;
    void destory(const VkDevice &dev);

    const uint32_t BINDING;
    const VkFormat FORMAT;
    const std::string NAME;
    const FlagBits NUM_CHANNELS;
    uint32_t width, height, mipLevels;
    VkImage image;
    VkDeviceMemory memory;
    VkImageView view;
    VkSampler sampler;
    std::vector<stbi_uc *> pPixels;
    VkDescriptorImageInfo imgDescInfo;
};

}  // namespace Sampler

#endif  // !SAMPLER_H