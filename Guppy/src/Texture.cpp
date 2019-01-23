
#include <algorithm>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION

#include "CmdBufHandler.h"
#include "Shell.h"
#include "Texture.h"

#define STB_FORMAT VK_FORMAT_R8G8B8A8_UNORM

// thread sync
void Texture::createTexture(const VkDevice& dev, const bool makeMipmaps, std::shared_ptr<Texture::Data> pTexture) {
    pTexture->pLdgRes = LoadingResourceHandler::createLoadingResources();

    auto& tex = (*pTexture);
    uint32_t layerCount = getArrayLayerCount(tex);

    createImage(dev, tex, layerCount);

    if (makeMipmaps) {
        generateMipmaps(tex, layerCount);
    } else {
        // TODO: DO SOMETHING .. this was not tested
    }
    createImageView(dev, tex, layerCount);
    createSampler(dev, tex);
    createDescInfo(tex);

    LoadingResourceHandler::loadSubmit(std::move(pTexture->pLdgRes));

    pTexture->status = STATUS::READY;
}

void Texture::createImage(const VkDevice& dev, Data& tex, uint32_t layerCount) {
    VkDeviceSize imageSize = tex.width * tex.height * 4;  // 4 components from STBI_rgb_alpha

    BufferResource stgRes = {};
    auto memReqsSize = helpers::createBuffer(dev, (imageSize * layerCount), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                             stgRes.buffer, stgRes.memory);

    void* pData;
    size_t offset = 0, size = static_cast<size_t>(imageSize);
    vkMapMemory(dev, stgRes.memory, 0, memReqsSize, 0, &pData);

    if (tex.flags & FLAG::DIFFUSE) {
        memcpy(static_cast<char*>(pData) + offset, tex.pixels, size);
        offset += size;
    }
    if (tex.flags & FLAG::NORMAL) {
        memcpy(static_cast<char*>(pData) + offset, tex.normPixels, size);
        offset += size;  // TODO: same size for all?
    }
    if (tex.flags & FLAG::SPECULAR) {
        memcpy(static_cast<char*>(pData) + offset, tex.specPixels, size);
        offset += size;  // TODO: same size for all?
    }

    vkUnmapMemory(dev, stgRes.memory);

    stbi_image_free(tex.pixels);
    stbi_image_free(tex.normPixels);
    stbi_image_free(tex.specPixels);

    // Using CmdBufHandler::getUniqueQueueFamilies(true, false, true) here might not be wise... To work
    // right it relies on the the two command buffers being created with the same data.
    helpers::createImage(dev, CmdBufHandler::getUniqueQueueFamilies(true, false, true), VK_SAMPLE_COUNT_1_BIT, STB_FORMAT,
                         VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, static_cast<uint32_t>(tex.width),
                         static_cast<uint32_t>(tex.height), tex.mipLevels, layerCount, tex.image, tex.memory);

    helpers::transitionImageLayout(tex.pLdgRes->transferCmd, tex.image, STB_FORMAT, VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, tex.mipLevels, layerCount);

    helpers::copyBufferToImage(tex.pLdgRes->graphicsCmd, tex.width, tex.height, layerCount, stgRes.buffer, tex.image);

    tex.pLdgRes->stgResources.push_back(stgRes);
}

void Texture::generateMipmaps(const Data& tex, uint32_t layerCount) {
    // This was the way before mip maps
    // transitionImageLayout(
    //    srcQueueFamilyIndexFinal,
    //    dstQueueFamilyIndexFinal,
    //    m_mipLevels,
    //    image,
    //    VK_FORMAT_R8G8B8A8_UNORM,
    //    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    //    m_transferCommandPool
    //);

    // transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = tex.image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = tex.width;
    int32_t mipHeight = tex.height;

    for (uint32_t i = 1; i < tex.mipLevels; i++) {
        // CREATE MIP LEVEL

        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(tex.pLdgRes->graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit = {};
        // source
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = layerCount;
        // destination
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = layerCount;

        vkCmdBlitImage(tex.pLdgRes->graphicsCmd, tex.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, tex.image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        // TRANSITION TO SHADER READY

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(tex.pLdgRes->graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);

        // This is a bit wonky methinks (non-sqaure is the case for this)
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // This is not handled in the loop so one more!!!! The last level is never never
    // blitted from.

    barrier.subresourceRange.baseMipLevel = tex.mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(tex.pLdgRes->graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);
}

void Texture::createImageView(const VkDevice& dev, Data& tex, uint32_t layerCount) {
    helpers::createImageView(dev, tex.image, tex.mipLevels, STB_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT,
                             VK_IMAGE_VIEW_TYPE_2D_ARRAY, layerCount, tex.view);
}

void Texture::createSampler(const VkDevice& dev, Data& tex) {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;  // TODO: OPTION (FEATURE BASED)
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;  // test this out for fun
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0;  // static_cast<float>(m_mipLevels / 2); // Optional
    samplerInfo.maxLod = static_cast<float>(tex.mipLevels);
    samplerInfo.mipLodBias = 0;  // Optional

    vk::assert_success(vkCreateSampler(dev, &samplerInfo, nullptr, &tex.sampler));

    // Name some objects for debugging
    ext::DebugMarkerSetObjectName(dev, (uint64_t)tex.sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT,
                                  (tex.name + " sampler").c_str());
}

void Texture::createDescInfo(Data& tex) {
    tex.imgDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    tex.imgDescInfo.imageView = tex.view;
    tex.imgDescInfo.sampler = tex.sampler;
}

uint32_t Texture::getArrayLayerCount(const Data& tex) {
    uint32_t count = 0;
    if (tex.flags & FLAG::DIFFUSE) count++;
    if (tex.flags & FLAG::NORMAL) count++;
    if (tex.flags & FLAG::SPECULAR) count++;
    return count;
}
