#ifndef SAMPLER_H
#define SAMPLER_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "ConstantsAll.h"
#include "Helpers.h"

class Shell;

namespace Sampler {

class Base {
   public:  // TODO: use access modifiers
    Base(const CreateInfo *pCreateInfo, bool hasData);

    void determineImageTypes();

    inline VkDeviceSize layerSize() const {
        return static_cast<VkDeviceSize>(imgCreateInfo.extent.width) *   //
               static_cast<VkDeviceSize>(imgCreateInfo.extent.height) *  //
               static_cast<VkDeviceSize>(BYTES_PER_CHANNEL) *            //
               static_cast<VkDeviceSize>(NUM_CHANNELS);
    }
    inline VkDeviceSize size() const { return layerSize() * static_cast<VkDeviceSize>(imgCreateInfo.arrayLayers); }

    void copyData(void *&pData, size_t &offset) const;
    inline void cleanup() { pPixels.clear(); }

    void destroy(const VkDevice &dev);

    // TODO: Get rid of all these creation info members

    const std::string NAME;
    const uint8_t BYTES_PER_CHANNEL;
    const FlagBits NUM_CHANNELS;
    const SAMPLER TYPE;

    FlagBits flags;  // TODO: remove this instead of passing a dynamic list to shaders?

    VkImageCreateInfo imgCreateInfo;
    SwapchainInfo swpchnInfo;
    MipmapInfo mipmapInfo;

    float aspect;
    VkImageViewType imageViewType;

    VkImage image;
    VkDeviceMemory memory;

    std::vector<stbi_uc *> pPixels;

    layerResourceMap layerResourceMap;
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