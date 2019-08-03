
#include "Sampler.h"

#include <sstream>
#include <stb_image.h>
#include <variant>

#include "Helpers.h"
#include "Shell.h"

namespace {

void validateInfo(const Sampler::CreateInfo* pCreateInfo) {
    assert(pCreateInfo);
    assert(!pCreateInfo->layerInfos.empty());
    assert(pCreateInfo->channels < UINT8_MAX);
    uint8_t maxChannels = static_cast<uint8_t>(pCreateInfo->channels);
    uint8_t offset = 0;
    for (const auto& layerInfo : pCreateInfo->layerInfos) {
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
        shell.log(Shell::LOG_ERR, ss.str().c_str());
    }
    if (pPixels == NULL) {  // TODO: this doesn't work
        ss << "failed to load " << path << std::endl;
        shell.log(Shell::LOG_ERR, ss.str().c_str());
        throw std::runtime_error(ss.str());
    }
}

void validateChannels(const Shell& shell, Sampler::Base& sampler, const std::string path, int c) {
    if (c < static_cast<int>(sampler.NUM_CHANNELS)) {
        std::stringstream ss;
        ss << std::endl
           << "Image at path \"" << path << "\" loaded for " << sampler.NUM_CHANNELS << " channels but only has " << c
           << " channels." << std::endl;
        shell.log(Shell::LOG_WARN, ss.str().c_str());
    }
}

void validateDimensions(const Shell& shell, Sampler::Base& sampler, const std::string path, int w, int h) {
    std::stringstream ss;
    if (w != sampler.extent.width) ss << "invalid " << path << " (width)! " << std::endl;
    if (w != sampler.extent.height) ss << "invalid " << path << " (height)! " << std::endl;
    auto errMsg = ss.str();
    if (!errMsg.empty()) {
        shell.log(Shell::LOG_ERR, errMsg.c_str());
        assert(false);
        throw std::runtime_error(errMsg);
    }
}

}  // namespace

Sampler::Base::Base(const CreateInfo* pCreateInfo, bool hasData)
    : IMAGE_FLAGS(pCreateInfo->imageFlags),
      NAME(pCreateInfo->name),
      NUM_CHANNELS(pCreateInfo->channels),
      TILING(pCreateInfo->tiling),
      TYPE(pCreateInfo->type),
      flags(0),
      swpchnInfo(pCreateInfo->swpchnInfo),
      mipmapInfo(pCreateInfo->mipmapInfo),
      format(pCreateInfo->format),
      samples(VK_SAMPLE_COUNT_1_BIT),
      usage(pCreateInfo->usage),
      extent(pCreateInfo->extent),
      aspect(BAD_ASPECT),
      arrayLayers(pCreateInfo->layerInfos.size()),
      image(VK_NULL_HANDLE),
      memory(VK_NULL_HANDLE),
      imageViewType(pCreateInfo->imageViewType),
      view(VK_NULL_HANDLE),
      sampler(VK_NULL_HANDLE),
      imgInfo{}  //
{
    if (swpchnInfo.usesSwapchain()) {
        if (swpchnInfo.usesExtent) assert(helpers::compExtent2D(extent, BAD_EXTENT_2D));
        if (swpchnInfo.usesFormat) format = VK_FORMAT_UNDEFINED;
    } else {
        assert(format != VK_FORMAT_UNDEFINED);
        if (!helpers::compExtent2D(extent, BAD_EXTENT_2D)) assert(!hasData);
    }
}

void Sampler::Base::determineImageTypes() {
    /* There is a table of valid type usage here:
     *	https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkImageViewCreateInfo.html.
     *
     *	I did not test any of the 1D types. The conditions here were done hastily,
     *	and are not well tested. I only did a few.
     */

    if (imageViewType == VK_IMAGE_VIEW_TYPE_MAX_ENUM) {
        // If imageViewType is not set then determine one.
        if (extent.width >= 1 && extent.height == 1 && arrayLayers == 1) {
            imageViewType = VK_IMAGE_VIEW_TYPE_1D;
            imageType = VK_IMAGE_TYPE_1D;
        } else if (extent.width >= 1 && extent.height == 1 && arrayLayers > 1) {
            imageViewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            imageType = VK_IMAGE_TYPE_1D;
        } else if (extent.width >= 1 && extent.height >= 1 && arrayLayers == 1) {
            imageViewType = VK_IMAGE_VIEW_TYPE_2D;
            imageType = VK_IMAGE_TYPE_2D;
        } else if (extent.width >= 1 && extent.width == extent.height && arrayLayers == 6) {
            imageViewType = VK_IMAGE_VIEW_TYPE_CUBE;
            imageType = VK_IMAGE_TYPE_2D;
        } else if (extent.width >= 1 && extent.height >= 1 && arrayLayers > 1) {
            imageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            imageType = VK_IMAGE_TYPE_2D;
        }
    } else {
        // If imageViewType is set then validate it.
        switch (imageViewType) {
            case VK_IMAGE_VIEW_TYPE_1D:
                assert(extent.width >= 1 && extent.height == 1 && arrayLayers >= 1);
                imageType = VK_IMAGE_TYPE_1D;
                break;
            case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
                assert(extent.width >= 1 && extent.height == 1 && arrayLayers >= 1);
                imageType = VK_IMAGE_TYPE_1D;
                break;
            case VK_IMAGE_VIEW_TYPE_2D:
                assert(extent.width >= 1 && extent.height >= 1 && arrayLayers >= 1);
                imageType = VK_IMAGE_TYPE_2D;
                break;
            case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
                assert(extent.width >= 1 && extent.height >= 1 && arrayLayers >= 1);
                imageType = VK_IMAGE_TYPE_2D;
                break;
            case VK_IMAGE_VIEW_TYPE_CUBE:
                assert(extent.width >= 1 && extent.width == extent.height && arrayLayers >= 6);
                imageType = VK_IMAGE_TYPE_2D;
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

VkImageCreateInfo Sampler::Base::getImageCreateInfo() {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = IMAGE_FLAGS;
    imageInfo.imageType = imageType;
    imageInfo.format = format;
    imageInfo.extent.width = extent.width;
    imageInfo.extent.height = extent.height;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.samples = samples;
    imageInfo.tiling = TILING;
    /* VK_IMAGE_USAGE_SAMPLED_BIT specifies that the image can be used to create a
     *	VkImageView suitable for occupying a VkDescriptorSet slot either of
     *	type VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE or VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
     */
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.mipLevels = mipmapInfo.mipLevels;
    imageInfo.extent.depth = 1;  // !
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    return imageInfo;
}

void Sampler::Base::destroy(const VkDevice& dev) {
    vkDestroySampler(dev, sampler, nullptr);
    vkDestroyImageView(dev, view, nullptr);
    vkDestroyImage(dev, image, nullptr);
    vkFreeMemory(dev, memory, nullptr);
}

// FUNCTIONS

Sampler::Base Sampler::make(const Shell& shell, const CreateInfo* pCreateInfo, const bool hasData) {
    Sampler::Base sampler(pCreateInfo, hasData);

    for (const auto& layerInfo : pCreateInfo->layerInfos) {
        sampler.flags |= layerInfo.type * GetChannelMask(pCreateInfo->channels);
    }

    // VALIDATE INFO
    validateInfo(pCreateInfo);

    if (!hasData) {
        // Texture has no data to load.
        if (!sampler.swpchnInfo.usesSwapchain()) {
            // aspect
            sampler.aspect = static_cast<float>(sampler.extent.width) / static_cast<float>(sampler.extent.height);
        }
    } else {
        // Texture has data to load so load it and validate.
        bool isFirstLayer = true;
        int w, h, c, req_comp = getReqComp(pCreateInfo->channels);

        for (auto& layerInfo : pCreateInfo->layerInfos) {
            // LOAD FROM FILE
            std::string path = layerInfo.path;
            std::replace(path.begin(), path.end(), '\\', '/');  // Convert windows path delimiters.

            // Load data...
            sampler.pPixels.push_back(stbi_load(path.c_str(), &w, &h, &c, req_comp));
            // checkFailure(shell, layerInfo.path, p);
            assert(sampler.pPixels.back() != nullptr);  // TODO: get this to work inside checkFailure.

            validateChannels(shell, sampler, layerInfo.path, c);

            if (isFirstLayer) {
                // width
                sampler.extent.width = static_cast<uint32_t>(w);
                assert(sampler.extent.width > 0);
                // height
                sampler.extent.height = static_cast<uint32_t>(h);
                assert(sampler.extent.height > 0);
                // aspect
                sampler.aspect = static_cast<float>(sampler.extent.width) / static_cast<float>(sampler.extent.height);
                // mip levels
                if (sampler.mipmapInfo.usesExtent) sampler.mipmapInfo.mipLevels = GetMipLevels(sampler.extent);
                assert(sampler.mipmapInfo.mipLevels > 0);

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
                VkDeviceSize size =
                    static_cast<VkDeviceSize>(sampler.extent.width) * static_cast<VkDeviceSize>(sampler.extent.height);
                for (VkDeviceSize i = 0; i < size; i++) {
                    // combineInfo { path, number of channels, combine offset }
                    auto offset = (i * sampler.NUM_CHANNELS) + std::get<2>(combineInfo);
                    auto combineOffset = i * std::get<1>(combineInfo);
                    std::memcpy(sampler.pPixels.back() + offset, pCombinePixels + combineOffset, std::get<1>(combineInfo));
                }
            }
        }

        assert(sampler.arrayLayers == sampler.pPixels.size());
    }

    sampler.determineImageTypes();
    return sampler;
}

VkSamplerCreateInfo Sampler::GetVkSamplerCreateInfo(const Sampler::Base& sampler) {
    VkSamplerCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    info.magFilter = VK_FILTER_LINEAR;
    info.minFilter = VK_FILTER_LINEAR;
    info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    info.mipLodBias = 0;              // Optional
    info.anisotropyEnable = VK_TRUE;  // TODO: OPTION (FEATURE BASED)
    info.maxAnisotropy = 16;
    info.compareEnable = VK_FALSE;
    info.compareOp = VK_COMPARE_OP_ALWAYS;
    info.minLod = 0;  // static_cast<float>(m_mipLevels / 2); // Optional
    info.maxLod = static_cast<float>(sampler.mipmapInfo.mipLevels);
    info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    info.unnormalizedCoordinates = VK_FALSE;  // test this out for fun

    switch (sampler.TYPE) {
        case SAMPLER::CUBE:
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.anisotropyEnable = VK_FALSE;  // VK_TRUE;
            // info.maxAnisotropy = 16;
            if (sampler.mipmapInfo.mipLevels > 0) {
                info.mipLodBias = 0;  // Optional
                info.minLod = 0;      // static_cast<float>(m_mipLevels / 2); // Optional
                info.maxLod = static_cast<float>(sampler.mipmapInfo.mipLevels);
            }
            break;
        case SAMPLER::CLAMP_TO_BORDER:
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            info.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            break;
        case SAMPLER::CLAMP_TO_EDGE:
            info.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            info.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
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
