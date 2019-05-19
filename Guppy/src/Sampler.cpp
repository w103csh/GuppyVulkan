
#include "Sampler.h"

#include <sstream>
#include <stb_image.h>

#include "Shell.h"

namespace {

void validateInfo(Sampler::CreateInfo* pCreateInfo) {
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
    if (true) return;
    std::stringstream ss;
    if (auto failure_reason = stbi_failure_reason()) {
        ss << "Error: \"" << failure_reason << "\" in file \"" << path << std::endl;
        shell.log(Shell::LOG_ERR, ss.str().c_str());
    }
    if (!pPixels) {
        ss << "failed to load " << path << std::endl;
        shell.log(Shell::LOG_ERR, ss.str().c_str());
        throw std::runtime_error(ss.str());
    }
}

void validateChannels(const Shell& shell, Sampler::Base* pSampler, const std::string path, int c) {
    if (c < static_cast<int>(pSampler->NUM_CHANNELS)) {
        std::stringstream ss;
        ss << std::endl
           << "Image at path \"" << path << "\" loaded for " << pSampler->NUM_CHANNELS << " channels but only has " << c
           << " channels." << std::endl;
        shell.log(Shell::LOG_WARN, ss.str().c_str());
    }
}

void validateDimensions(const Shell& shell, Sampler::Base* pSampler, const std::string path, int w, int h) {
    std::stringstream ss;
    if (w != pSampler->width) ss << "invalid " << path << " (width)! " << std::endl;
    if (w != pSampler->height) ss << "invalid " << path << " (height)! " << std::endl;
    auto errMsg = ss.str();
    if (!errMsg.empty()) {
        shell.log(Shell::LOG_ERR, errMsg.c_str());
        assert(false);
        throw std::runtime_error(errMsg);
    }
}

}  // namespace

Sampler::LayerInfo Sampler::GetDef4Comb3And1LayerInfo(const Sampler::TYPE&& type, const std::string& path,
                                                      const std::string& fileName, const std::string& combineFileName) {
    LayerInfo layerInfo = {type, path + fileName};
    if (!combineFileName.empty()) {
        layerInfo.combineInfos.push_back({path + combineFileName, CHANNELS::_1, 3});
    }
    return layerInfo;
}

Sampler::Base::Base(const Shell& shell, CreateInfo* pCreateInfo)
    : FORMAT(pCreateInfo->format),
      NAME(pCreateInfo->name),
      NUM_CHANNELS(pCreateInfo->channels),
      TILING(pCreateInfo->tiling),
      flags(0),
      width(0),
      height(0),
      mipLevels(1),
      arrayLayers(pCreateInfo->layerInfos.size()),
      image(VK_NULL_HANDLE),
      memory(VK_NULL_HANDLE),
      imageViewType(pCreateInfo->imageViewType),
      view(VK_NULL_HANDLE),
      sampler(VK_NULL_HANDLE),
      imgDescInfo{} {
    // VALIDATE INFO
    validateInfo(pCreateInfo);

    bool isFirstLayer = true;
    int w, h, c, req_comp = getReqComp(pCreateInfo->channels);

    for (auto& layerInfo : pCreateInfo->layerInfos) {
        // Convert windows path delimiters.
        std::replace(layerInfo.path.begin(), layerInfo.path.end(), '\\', '/');

        // Load data...
        pPixels.push_back(stbi_load(layerInfo.path.c_str(), &w, &h, &c, req_comp));
        checkFailure(shell, layerInfo.path, pPixels.back());

        validateChannels(shell, this, layerInfo.path, c);

        if (isFirstLayer) {
            width = static_cast<uint32_t>(w);
            height = static_cast<uint32_t>(h);
            if (pCreateInfo->makeMipmaps)
                mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
            // validate
            assert(width > 0);
            assert(height > 0);
            assert(mipLevels > 0);
            isFirstLayer = false;
        } else {
            validateDimensions(shell, this, layerInfo.path, w, h);
        }

        flags |= layerInfo.type * GetChannelMask(pCreateInfo->channels);

        for (const auto& combineInfo : layerInfo.combineInfos) {
            int req_comp_combine = getReqComp(std::get<1>(combineInfo));

            // Load combine data...
            auto pCombinePixels = stbi_load(std::get<0>(combineInfo).c_str(), &w, &h, &c, req_comp_combine);
            checkFailure(shell, layerInfo.path, pCombinePixels);

            validateChannels(shell, this, layerInfo.path, c);
            validateDimensions(shell, this, layerInfo.path, w, h);

            // Mix combined data in.
            VkDeviceSize size = static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height);
            for (VkDeviceSize i = 0; i < size; i++) {
                // combineInfo { path, number of channels, combine offset }
                auto offset = (i * NUM_CHANNELS) + std::get<2>(combineInfo);
                auto combineOffset = i * std::get<1>(combineInfo);
                std::memcpy(pPixels.back() + offset, pCombinePixels + combineOffset, std::get<1>(combineInfo));
            }
        }
    }
    assert(arrayLayers == pPixels.size());
    determineImageTypes();
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
        if (width >= 1 && height == 1 && arrayLayers == 1) {
            imageViewType = VK_IMAGE_VIEW_TYPE_1D;
            imageType = VK_IMAGE_TYPE_1D;
        } else if (width >= 1 && height == 1 && arrayLayers > 1) {
            imageViewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            imageType = VK_IMAGE_TYPE_1D;
        } else if (width >= 1 && height >= 1 && arrayLayers == 1) {
            imageViewType = VK_IMAGE_VIEW_TYPE_2D;
            imageType = VK_IMAGE_TYPE_2D;
        } else if (width >= 1 && width == height && arrayLayers == 6) {
            imageViewType = VK_IMAGE_VIEW_TYPE_CUBE_ARRAY;
            imageType = VK_IMAGE_TYPE_2D;
        } else if (width >= 1 && height >= 1 && arrayLayers > 1) {
            imageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            imageType = VK_IMAGE_TYPE_2D;
        }
    } else {
        // If imageViewType is set then validate it.
        switch (imageViewType) {
            case VK_IMAGE_VIEW_TYPE_1D:
                assert(width >= 1 && height == 1 && arrayLayers >= 1);
                imageType = VK_IMAGE_TYPE_1D;
                break;
            case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
                assert(width >= 1 && height == 1 && arrayLayers >= 1);
                imageType = VK_IMAGE_TYPE_1D;
                break;
            case VK_IMAGE_VIEW_TYPE_2D:
                assert(width >= 1 && height >= 1 && arrayLayers >= 1);
                imageType = VK_IMAGE_TYPE_2D;
                break;
            case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
                assert(width >= 1 && height >= 1 && arrayLayers >= 1);
                imageType = VK_IMAGE_TYPE_2D;
                break;
            case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
                assert(width >= 1 && width == height && arrayLayers >= 6);
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

void Sampler::Base::destroy(const VkDevice& dev) {
    vkDestroySampler(dev, sampler, nullptr);
    vkDestroyImageView(dev, view, nullptr);
    vkDestroyImage(dev, image, nullptr);
    vkFreeMemory(dev, memory, nullptr);
}

VkImageCreateInfo Sampler::Base::getImageCreateInfo() {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags = 0;  // TODO: what should this be. There is a 2D array bit.
    imageInfo.imageType = imageType;
    imageInfo.format = FORMAT;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.arrayLayers = arrayLayers;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;  // TODO: can this be something else?
    imageInfo.tiling = TILING;
    /* VK_IMAGE_USAGE_SAMPLED_BIT specifies that the image can be used to create a
     *	VkImageView suitable for occupying a VkDescriptorSet slot either of
     *	type VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE or VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
     */
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.mipLevels = mipLevels;
    imageInfo.extent.depth = 1;  // !
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    return imageInfo;
}
