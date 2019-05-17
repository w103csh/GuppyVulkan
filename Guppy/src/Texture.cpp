
#include "Texture.h"

Texture::Base::Base(const uint32_t&& offset, CreateInfo* pCreateInfo)
    : NAME(helpers::getFileName(pCreateInfo->colorPath)),
      OFFSET(offset),
      status(STATUS::PENDING),
      makeMipmaps(pCreateInfo->makeMipmaps),
      flags(0),
      aspect(EMPTY_ASPECT),
      pLdgRes(nullptr),
      colorPath(pCreateInfo->colorPath),
      normalPath(pCreateInfo->normalPath),
      specularPath(pCreateInfo->specularPath),
      alphaPath(pCreateInfo->alphaPath),
      heightPath(pCreateInfo->heightPath),
      colorChannels(pCreateInfo->colorChannels),
      normalChannels(pCreateInfo->normalChannels),
      specularChannels(pCreateInfo->specularChannels),
      alphaChannels(pCreateInfo->alphaChannels),
      heightChannels(pCreateInfo->heightChannels),
      sampCh1(NAME, {Sampler::CHANNELS::_1, VK_FORMAT_R8_UNORM, 2}),
      sampCh2(NAME, {Sampler::CHANNELS::_2, VK_FORMAT_R8G8_UNORM, 3}),
      sampCh3(NAME, {Sampler::CHANNELS::_3, VK_FORMAT_R5G6B5_UNORM_PACK16, 1}),
      sampCh4(NAME, {Sampler::CHANNELS::_4, VK_FORMAT_R8G8B8A8_UNORM, 0})
//
{
    // Set texture flags
    if (!colorPath.empty()) flags |= Texture::TYPE::COLOR * Sampler::getChannelMask(pCreateInfo->colorChannels);
    if (!normalPath.empty()) flags |= Texture::TYPE::NORMAL * Sampler::getChannelMask(pCreateInfo->normalChannels);
    if (!specularPath.empty()) flags |= Texture::TYPE::SPECULAR * Sampler::getChannelMask(pCreateInfo->specularChannels);
    if (!alphaPath.empty()) flags |= Texture::TYPE::ALPHA * Sampler::getChannelMask(pCreateInfo->alphaChannels);
    if (!heightPath.empty()) flags |= Texture::TYPE::HEIGHT * Sampler::getChannelMask(pCreateInfo->heightChannels);
    assert(flags);
}

bool Texture::Base::hasSampler(const uint32_t& binding) const {
    bool hasSampler = false;
    switch (binding) {
        case 0:
            hasSampler = sampCh4.imgDescInfo.sampler != VK_NULL_HANDLE;
            break;
        case 1:
            hasSampler = sampCh1.imgDescInfo.sampler != VK_NULL_HANDLE;
            break;
        case 2:
            hasSampler = sampCh2.imgDescInfo.sampler != VK_NULL_HANDLE;
            break;
        case 3:
            hasSampler = sampCh3.imgDescInfo.sampler != VK_NULL_HANDLE;
            break;
        default:
            assert(false);
            break;
    }
    return hasSampler;
}

void Texture::Base::setWriteInfo(VkWriteDescriptorSet& write) const {
    write.descriptorCount = 1;
    switch (write.dstBinding) {
        case 0:
            write.pImageInfo = &sampCh4.imgDescInfo;
            break;
        case 1:
            write.pImageInfo = &sampCh1.imgDescInfo;
            break;
        case 2:
            write.pImageInfo = &sampCh2.imgDescInfo;
            break;
        case 3:
            write.pImageInfo = &sampCh3.imgDescInfo;
            break;
        default:
            assert(false);
            break;
    }
}
