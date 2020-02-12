/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef SAMPLER_H
#define SAMPLER_H

#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "ConstantsAll.h"
#include "Helpers.h"

class Shell;

namespace Sampler {

class Base {
   public:  // TODO: use access modifiers
    Base(const CreateInfo *pCreateInfo, bool hasData);

    void determineImageTypes();

    inline vk::DeviceSize layerSize() const {
        return static_cast<vk::DeviceSize>(imgCreateInfo.extent.width) *   //
               static_cast<vk::DeviceSize>(imgCreateInfo.extent.height) *  //
               static_cast<vk::DeviceSize>(BYTES_PER_CHANNEL) *            //
               static_cast<vk::DeviceSize>(NUM_CHANNELS);
    }
    inline vk::DeviceSize size() const { return layerSize() * static_cast<vk::DeviceSize>(imgCreateInfo.arrayLayers); }

    void copyData(void *&pData, size_t &offset) const;
    inline void cleanup() { pPixels.clear(); }

    void destroy(const vk::Device &dev);

    // TODO: Get rid of all these creation info members

    const std::string NAME;
    const uint8_t BYTES_PER_CHANNEL;
    const FlagBits NUM_CHANNELS;
    const SAMPLER TYPE;

    FlagBits flags;  // TODO: remove this instead of passing a dynamic list to shaders?

    vk::ImageCreateInfo imgCreateInfo;
    SwapchainInfo swpchnInfo;
    MipmapInfo mipmapInfo;

    float aspect;
    vk::ImageViewType imageViewType;

    vk::Image image;
    vk::DeviceMemory memory;

    std::vector<void *> pPixels;

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

vk::SamplerCreateInfo GetVulkanSamplerCreateInfo(const Sampler::Base &sampler);

LayerInfo GetDef4Comb3And1LayerInfo(const Sampler::USAGE &&type, const std::string &path, const std::string &fileName,
                                    const std::string &combineFileName = "");

}  // namespace Sampler

#endif  // !SAMPLER_H