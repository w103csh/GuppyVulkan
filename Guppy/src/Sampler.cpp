/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Sampler.h"

#include <sstream>
#include <stb_image.h>
#include <variant>

#include <Common/Helpers.h>

#include "Shell.h"

namespace {

void validateInfo(const Sampler::CreateInfo* pCreateInfo) {
    assert(pCreateInfo);
    assert(!pCreateInfo->layersInfo.infos.empty());
    assert(pCreateInfo->numberOfChannels < UINT8_MAX);
    uint8_t maxChannels = static_cast<uint8_t>(pCreateInfo->numberOfChannels);
    uint8_t offset = 0;
    for (const auto& layerInfo : pCreateInfo->layersInfo.infos) {
        for (const auto& combineInfo : layerInfo.combineInfos) {
            assert(std::get<1>(combineInfo) < UINT8_MAX);
            offset = static_cast<uint8_t>(std::get<1>(combineInfo)) + std::get<2>(combineInfo);
            assert(offset <= maxChannels);
        }
    }
}

int getReqComp(const Sampler::CHANNELS& type) {
    switch (type) {
        case Sampler::CHANNELS::_1:
            return STBI_grey;
            break;
        case Sampler::CHANNELS::_2:
            return STBI_grey_alpha;
            break;
        case Sampler::CHANNELS::_3:
            return STBI_rgb;
            break;
        case Sampler::CHANNELS::_4:
            return STBI_rgb_alpha;
            break;
        default:
            assert(false);
            return STBI_default;
    }
}

void checkFailure(const Shell& shell, const std::string path, const stbi_uc* pPixels) {
    return;  // TODO: something that works and isn't annoying.
    std::stringstream ss;
    if (auto failure_reason = stbi_failure_reason()) {
        ss << "Error: \"" << failure_reason << "\" in file \"" << path << std::endl;
        shell.log(Shell::LogPriority::LOG_ERR, ss.str().c_str());
    }
    if (pPixels == NULL) {  // TODO: this doesn't work
        ss << "failed to load " << path << std::endl;
        shell.log(Shell::LogPriority::LOG_ERR, ss.str().c_str());
        throw std::runtime_error(ss.str());
    }
}

void validateChannels(const Shell& shell, Sampler::Base& sampler, const std::string path, int c) {
    if (c < static_cast<int>(sampler.NUM_CHANNELS)) {
        std::stringstream ss;
        ss << "Image at path \"" << path << "\" loaded for " << sampler.NUM_CHANNELS << " channels but only has " << c
           << " channels.";
        shell.log(Shell::LogPriority::LOG_WARN, ss.str().c_str());
    }
}

void validateDimensions(const Shell& shell, Sampler::Base& sampler, const std::string path, int w, int h) {
    std::stringstream ss;
    if (w != sampler.imgCreateInfo.extent.width) ss << "invalid " << path << " (width)! " << std::endl;
    if (w != sampler.imgCreateInfo.extent.height) ss << "invalid " << path << " (height)! " << std::endl;
    auto errMsg = ss.str();
    if (!errMsg.empty()) {
        shell.log(Shell::LogPriority::LOG_ERR, errMsg.c_str());
        assert(false);
        throw std::runtime_error(errMsg);
    }
}

}  // namespace

Sampler::Base::Base(const CreateInfo* pCreateInfo, bool needsData)
    : NAME(pCreateInfo->name),
      BYTES_PER_CHANNEL(pCreateInfo->bytesPerChannel),
      NUM_CHANNELS(pCreateInfo->numberOfChannels),
      TYPE(pCreateInfo->type),
      flags(0),
      imgCreateInfo(),
      swpchnInfo(pCreateInfo->swpchnInfo),
      mipmapInfo(pCreateInfo->mipmapCreateInfo.info),
      aspect(BAD_ASPECT),
      imageViewType(pCreateInfo->imageViewType),
      image(),
      memory(),
      imgInfo{}  //
{
    // Image create info
    imgCreateInfo.flags = pCreateInfo->imageFlags;
    imgCreateInfo.imageType = vk::ImageType::e2D;
    imgCreateInfo.format = pCreateInfo->format;
    imgCreateInfo.extent = pCreateInfo->extent;
    imgCreateInfo.mipLevels = pCreateInfo->mipmapCreateInfo.mipLevels;
    imgCreateInfo.arrayLayers = pCreateInfo->layersInfo.infos.size();
    imgCreateInfo.samples = vk::SampleCountFlagBits::e1;
    imgCreateInfo.tiling = pCreateInfo->tiling;
    imgCreateInfo.usage = pCreateInfo->usage;
    imgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imgCreateInfo.queueFamilyIndexCount = 0;
    imgCreateInfo.pQueueFamilyIndices = nullptr;
    imgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

    // Swapchain info settings
    if (swpchnInfo.usesSwapchain()) {
        if (swpchnInfo.usesExtent) assert(helpers::compExtent3D(imgCreateInfo.extent, BAD_EXTENT_3D));
        if (swpchnInfo.usesFormat) imgCreateInfo.format = vk::Format::eUndefined;
    } else {
        assert(imgCreateInfo.format != vk::Format::eUndefined);
        if (!helpers::compExtent3D(imgCreateInfo.extent, BAD_EXTENT_3D)) assert(!needsData);
    }

    // Setup layer info structures
    if (pCreateInfo->layersInfo.makeSampler) assert(pCreateInfo->layersInfo.makeImageView);
    if (pCreateInfo->layersInfo.makeImageView) {
        auto insertPair = layerResourceMap.insert({IMAGE_ARRAY_LAYERS_ALL, {pCreateInfo->layersInfo.makeSampler}});
        assert(insertPair.second);
    }
    for (uint32_t layer = 0; layer < pCreateInfo->layersInfo.infos.size(); layer++) {
        const auto& layerInfo = pCreateInfo->layersInfo.infos[layer];
        if (layerInfo.makeSampler) assert(layerInfo.makeImageView);
        if (layerInfo.makeImageView) {
            auto insertPair = layerResourceMap.insert({layer, {layerInfo.makeSampler}});
            assert(insertPair.second);
        }
    }
}

void Sampler::Base::determineImageTypes() {
    /* There is a table of valid type usage here:
     *	https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkImageViewCreateInfo.html.
     *
     *	I did not test any of the 1D types. The conditions here were done hastily,
     *	and are not well tested. I only did a few.
     */

    const auto& arrayLayers = imgCreateInfo.arrayLayers;
    const auto& extent = imgCreateInfo.extent;
    const auto& samples = imgCreateInfo.samples;
    const auto& flags = imgCreateInfo.flags;

    if (imageViewType == vk::ImageViewType{}) {
        // If imageViewType is not set then determine one.
        if (extent.width >= 1 && extent.height == 1 && arrayLayers == 1) {
            imageViewType = vk::ImageViewType::e1D;
            imgCreateInfo.imageType = vk::ImageType::e1D;
        } else if (extent.width >= 1 && extent.height == 1 && arrayLayers > 1) {
            imageViewType = vk::ImageViewType::e1DArray;
            imgCreateInfo.imageType = vk::ImageType::e1D;
        } else if (extent.width >= 1 && extent.height >= 1 && arrayLayers == 1) {
            imageViewType = vk::ImageViewType::e2D;
            imgCreateInfo.imageType = vk::ImageType::e2D;
        } else if (extent.width >= 1 && extent.width == extent.height && arrayLayers == 6) {
            imageViewType = vk::ImageViewType::eCube;
            imgCreateInfo.imageType = vk::ImageType::e2D;
        } else if (extent.width >= 1 && extent.height >= 1 && arrayLayers > 1) {
            imageViewType = vk::ImageViewType::e2DArray;
            imgCreateInfo.imageType = vk::ImageType::e2D;
        }
    } else {
        // If imageViewType is set then validate it.
        switch (imageViewType) {
            case vk::ImageViewType::e1D:
                assert(extent.width >= 1 && extent.height == 1 && extent.depth == 1 && arrayLayers >= 1);
                imgCreateInfo.imageType = vk::ImageType::e1D;
                break;
            case vk::ImageViewType::e1DArray:
                assert(extent.width >= 1 && extent.height == 1 && extent.depth == 1 && arrayLayers >= 1);
                imgCreateInfo.imageType = vk::ImageType::e1D;
                break;
            case vk::ImageViewType::e2D:
                // Note: There are versions of 2D that can have a depth >= 1, but there are other
                // conditions and would result in vk::ImageType::e3D.
                assert(extent.width >= 1 && extent.height >= 1 && extent.depth == 1 && arrayLayers >= 1);
                imgCreateInfo.imageType = vk::ImageType::e2D;
                break;
            case vk::ImageViewType::e2DArray:
                // Note: There are versions of 2D_ARRAY that can have a depth >= 1, but there are
                // other conditions and would result in vk::ImageType::e3D.
                assert(extent.width >= 1 && extent.height >= 1 && extent.depth == 1 && arrayLayers >= 1);
                imgCreateInfo.imageType = vk::ImageType::e2D;
                break;
            case vk::ImageViewType::eCube:
                assert(extent.width >= 1 && extent.width == extent.height && extent.depth == 1 && arrayLayers >= 6);
                imgCreateInfo.imageType = vk::ImageType::e2D;
                break;
            case vk::ImageViewType::e3D:
                assert(extent.width >= 1 && extent.height >= 1 && extent.depth >= 1 && arrayLayers == 1 &&
                       samples == vk::SampleCountFlagBits::e1);
                imgCreateInfo.imageType = vk::ImageType::e3D;
                break;
            case vk::ImageViewType::eCubeArray:
                assert(extent.width >= 1 && extent.height == extent.width && extent.depth == 1 &&
                       (arrayLayers > 0 && arrayLayers % 6 == 0) && samples == vk::SampleCountFlagBits::e1 &&
                       (flags & vk::ImageCreateFlagBits::eCubeCompatible));
                imgCreateInfo.imageType = vk::ImageType::e2D;
                break;
            default:
                assert(false && "Unhandled image view type");
                break;
        }
    }
}

void Sampler::Base::copyData(void*& pData, size_t& offset) const {
    auto size = layerSize();
    for (auto& p : pPixels) {
        std::memcpy(static_cast<char*>(pData) + offset, p, size);
        offset += size;
        stbi_image_free(p);
    }
}

void Sampler::Base::destroy(const Context& ctx) {
    for (auto& [layer, layerResource] : layerResourceMap) {
        if (layerResource.sampler) ctx.dev.destroySampler(layerResource.sampler, ctx.pAllocator);
        ctx.dev.destroyImageView(layerResource.view, ctx.pAllocator);
    }
    ctx.dev.destroyImage(image, ctx.pAllocator);
    ctx.dev.freeMemory(memory, ctx.pAllocator);
}

// FUNCTIONS

Sampler::Base Sampler::make(const Shell& shell, const CreateInfo* pCreateInfo, const bool needsData) {
    Sampler::Base sampler(pCreateInfo, needsData);

    for (const auto& layerInfo : pCreateInfo->layersInfo.infos) {
        sampler.flags |= layerInfo.type * GetChannelMask(pCreateInfo->numberOfChannels);
    }

    // VALIDATE INFO
    validateInfo(pCreateInfo);

    if (!needsData && !pCreateInfo->layersInfo.hasData()) {
        // Texture has no data to load.
        if (!sampler.swpchnInfo.usesSwapchain()) {
            // aspect
            sampler.aspect = static_cast<float>(sampler.imgCreateInfo.extent.width) /
                             static_cast<float>(sampler.imgCreateInfo.extent.height);
        }
    } else {
        // Texture has data to load so load it and validate.
        bool isFirstLayer = true;
        int w, h, d, c, req_comp = getReqComp(pCreateInfo->numberOfChannels);

        for (auto& layerInfo : pCreateInfo->layersInfo.infos) {
            if (layerInfo.pPixel == nullptr) {
                // LOAD FROM FILE
                std::string path = layerInfo.path;
                std::replace(path.begin(), path.end(), '\\', '/');  // Convert windows path delimiters.

                // Load data...
                sampler.pPixels.push_back(stbi_load(path.c_str(), &w, &h, &c, req_comp));
                // checkFailure(shell, layerInfo.path, p);

                d = 1;  // Depth is always 1 if loading from file ???
            } else {
                w = pCreateInfo->extent.width;
                h = pCreateInfo->extent.height;
                d = pCreateInfo->extent.depth;
                c = pCreateInfo->numberOfChannels;
                // Pixel data is already known.
                sampler.pPixels.push_back(layerInfo.pPixel);
            }
            assert(sampler.pPixels.back() != nullptr);  // TODO: get this to work inside checkFailure.

            validateChannels(shell, sampler, layerInfo.path, c);

            if (isFirstLayer) {
                // width
                sampler.imgCreateInfo.extent.width = static_cast<uint32_t>(w);
                assert(sampler.imgCreateInfo.extent.width > 0);
                // height
                sampler.imgCreateInfo.extent.height = static_cast<uint32_t>(h);
                assert(sampler.imgCreateInfo.extent.height > 0);
                // depth
                sampler.imgCreateInfo.extent.depth = static_cast<uint32_t>(d);
                assert(sampler.imgCreateInfo.extent.depth > 0);
                // aspect
                sampler.aspect = static_cast<float>(sampler.imgCreateInfo.extent.width) /
                                 static_cast<float>(sampler.imgCreateInfo.extent.height);
                // mip levels
                if (sampler.mipmapInfo.usesExtent)
                    sampler.imgCreateInfo.mipLevels = GetMipLevels(sampler.imgCreateInfo.extent);
                assert(sampler.imgCreateInfo.mipLevels > 0);

                isFirstLayer = false;
            } else {
                validateDimensions(shell, sampler, layerInfo.path, w, h);
            }

            /* TOOD: uncomment this and remove constructor code when this can
             *  handle dataless samplers. Or remove both if moving toward macro
             *  replace for sampler types/count.
             */
            // sampler.flags |= layerInfo.type * GetChannelMask(pCreateInfo->channels);

            for (const auto& combineInfo : layerInfo.combineInfos) {
                int req_comp_combine = getReqComp(std::get<1>(combineInfo));

                // Load combine data...
                auto pCombinePixels = stbi_load(std::get<0>(combineInfo).c_str(), &w, &h, &c, req_comp_combine);
                // checkFailure(shell, layerInfo.path, pCombinePixels);

                validateChannels(shell, sampler, layerInfo.path, c);
                validateDimensions(shell, sampler, layerInfo.path, w, h);

                // Mix combined data in.
                vk::DeviceSize size = static_cast<vk::DeviceSize>(sampler.imgCreateInfo.extent.width) *
                                      static_cast<vk::DeviceSize>(sampler.imgCreateInfo.extent.height);
                for (vk::DeviceSize i = 0; i < size; i++) {
                    // combineInfo { path, number of channels, combine offset }
                    auto offset = (i * sampler.NUM_CHANNELS) + std::get<2>(combineInfo);
                    auto combineOffset = i * std::get<1>(combineInfo);
                    std::memcpy(((stbi_uc*)sampler.pPixels.back()) + offset, pCombinePixels + combineOffset,
                                std::get<1>(combineInfo));
                }
            }
        }

        assert(sampler.imgCreateInfo.arrayLayers == sampler.pPixels.size());
    }

    sampler.determineImageTypes();
    return sampler;
}

vk::SamplerCreateInfo Sampler::GetVulkanSamplerCreateInfo(const Sampler::Base& sampler) {
    vk::SamplerCreateInfo info = {};
    info.magFilter = vk::Filter::eLinear;
    info.minFilter = vk::Filter::eLinear;
    info.mipmapMode = vk::SamplerMipmapMode::eLinear;
    info.addressModeU = vk::SamplerAddressMode::eRepeat;
    info.addressModeV = vk::SamplerAddressMode::eRepeat;
    info.addressModeW = vk::SamplerAddressMode::eRepeat;
    info.mipLodBias = 0;              // Optional
    info.anisotropyEnable = VK_TRUE;  // TODO: OPTION (FEATURE BASED)
    info.maxAnisotropy = 16;
    info.compareEnable = VK_FALSE;
    info.compareOp = vk::CompareOp::eAlways;
    info.minLod = 0;  // static_cast<float>(m_mipLevels / 2); // Optional
    info.maxLod = static_cast<float>(sampler.imgCreateInfo.mipLevels);
    info.borderColor = vk::BorderColor::eIntOpaqueBlack;
    info.unnormalizedCoordinates = VK_FALSE;  // test this out for fun

    switch (sampler.TYPE) {
        case SAMPLER::DEFAULT_NEAREST:
            info.magFilter = vk::Filter::eNearest;
            info.minFilter = vk::Filter::eNearest;
            break;
        case SAMPLER::CUBE:
            info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
            info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
            info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
            info.anisotropyEnable = VK_FALSE;  // VK_TRUE;
            // info.maxAnisotropy = 16;
            if (sampler.imgCreateInfo.mipLevels > 0) {
                info.mipLodBias = 0;  // Optional
                info.minLod = 0;      // static_cast<float>(m_mipLevels / 2); // Optional
                info.maxLod = static_cast<float>(sampler.imgCreateInfo.mipLevels);
            }
            break;
        case SAMPLER::CLAMP_TO_BORDER:
            info.addressModeU = vk::SamplerAddressMode::eClampToBorder;
            info.addressModeV = vk::SamplerAddressMode::eClampToBorder;
            info.addressModeW = vk::SamplerAddressMode::eClampToBorder;
            info.borderColor = vk::BorderColor::eFloatTransparentBlack;
            break;
        case SAMPLER::CLAMP_TO_EDGE:
            info.addressModeU = vk::SamplerAddressMode::eClampToEdge;
            info.addressModeV = vk::SamplerAddressMode::eClampToEdge;
            info.addressModeW = vk::SamplerAddressMode::eClampToEdge;
            break;
        case SAMPLER::CLAMP_TO_BORDER_DEPTH:
            info.magFilter = vk::Filter::eNearest;
            info.minFilter = vk::Filter::eNearest;
        case SAMPLER::CLAMP_TO_BORDER_DEPTH_PCF:
            info.magFilter = vk::Filter::eLinear;
            info.minFilter = vk::Filter::eLinear;
            info.addressModeU = vk::SamplerAddressMode::eClampToBorder;
            info.addressModeV = vk::SamplerAddressMode::eClampToBorder;
            info.addressModeW = vk::SamplerAddressMode::eClampToBorder;
            info.compareEnable = VK_TRUE;
            info.compareOp = vk::CompareOp::eLess;
            info.borderColor = vk::BorderColor::eFloatOpaqueWhite;
            break;
        case SAMPLER::DEFAULT:
            break;
        default:
            assert(false);
            throw std::runtime_error("Unhandled sampler type");
    }
    return info;
}

Sampler::LayerInfo Sampler::GetDef4Comb3And1LayerInfo(const Sampler::USAGE&& type, const std::string& path,
                                                      const std::string& fileName, const std::string& combineFileName) {
    LayerInfo layerInfo = {type, path + fileName};
    if (!combineFileName.empty()) {
        layerInfo.combineInfos.push_back({path + combineFileName, CHANNELS::_1, 3});
    }
    return layerInfo;
}
