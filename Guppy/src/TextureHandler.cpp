
#include <algorithm>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION

#include "TextureHandler.h"

#include "CommandHandler.h"
#include "LoadingHandler.h"
#include "Shell.h"
#include "Sampler.h"
#include "ShaderHandler.h"

#define STB_FORMAT VK_FORMAT_R8G8B8A8_UNORM

Texture::Handler::Handler(Game* pGame) : Game::Handler(pGame) {}

void Texture::Handler::init() {
    reset();

    // Default textures
    addTexture(STATUE_TEX_PATH);
    addTexture(CHALET_TEX_PATH);
    addTexture(VULKAN_TEX_PATH);
    addTexture(MED_H_DIFF_TEX_PATH, MED_H_NORM_TEX_PATH, MED_H_SPEC_TEX_PATH);
    addTexture(HARDWOOD_FLOOR_TEX_PATH);
    addTexture(WOOD_007_DIFF_TEX_PATH, WOOD_007_NORM_TEX_PATH);
}

void Texture::Handler::reset() {
    for (auto& pTex : pTextures_) {
        vkDestroySampler(shell().context().dev, pTex->sampler, nullptr);
        vkDestroyImageView(shell().context().dev, pTex->view, nullptr);
        vkDestroyImage(shell().context().dev, pTex->image, nullptr);
        vkFreeMemory(shell().context().dev, pTex->memory, nullptr);
    }
    pTextures_.clear();
}

// This should be the only avenue for texture creation.
void Texture::Handler::addTexture(std::string path, std::string normPath, std::string specPath) {
    for (auto& tex : pTextures_) {
        if (tex->path == path) shell().log(Shell::LOG_WARN, "Texture with same path was already loaded.");
        if (!tex->normPath.empty() && tex->normPath == normPath)
            shell().log(Shell::LOG_WARN, "Texture with same normal path was already loaded.");
        if (!tex->specPath.empty() && tex->specPath == specPath)
            shell().log(Shell::LOG_WARN, "Texture with same spectral path was already loaded.");
    }

    // If this asserts read the comment where TEXTURE_LIMIT is declared.
    assert(pTextures_.size() < TEXTURE_LIMIT);

    // Make the texture.
    pTextures_.emplace_back(std::make_shared<Texture::DATA>(pTextures_.size(), path, normPath, specPath));
    // Load the texture data...
    auto fut = loadTexture(shell().context().dev, true, pTextures_.back());
    // Store the loading future.
    texFutures_.emplace_back(std::move(fut));
}

const std::shared_ptr<Texture::DATA> Texture::Handler::getTextureByPath(std::string path) const {
    auto it = std::find_if(pTextures_.begin(), pTextures_.end(), [&path](auto& pTex) { return pTex->path == path; });
    return it == pTextures_.end() ? nullptr : (*it);
}

void Texture::Handler::update() {
    // Check texture futures
    if (!texFutures_.empty()) {
        bool wasUpdated = false;
        auto itFut = texFutures_.begin();

        while (itFut != texFutures_.end()) {
            auto& fut = (*itFut);

            // Check the status but don't wait...
            auto status = fut.wait_for(std::chrono::seconds(0));

            if (status == std::future_status::ready) {
                // Finish creating the texture
                createTexture(true, fut.get());

                // Remove the future from the list if all goes well.
                itFut = texFutures_.erase(itFut);
                wasUpdated = true;

            } else {
                ++itFut;
            }
        }

        // if (wasUpdated) Shader::Handler::updateDefaultDynamicUniform();
    }
}

VkDescriptorSetLayoutBinding Texture::Handler::getDescriptorLayoutBinding(const DESCRIPTOR& type) const {
    assert(false);
    // switch (type) {
    //    case DESCRIPTOR::DEFAULT_SAMPLER:
    //        return getDefaultSamplerBindings();
    //    default:
    //        throw std::runtime_error("descriptor type not handled!");
    //}
    return {};
}

VkWriteDescriptorSet Texture::Handler::getDescriptorWrite(const DESCRIPTOR& type) const {
    assert(false);
    // switch (type) {
    //    case DESCRIPTOR::DEFAULT_SAMPLER:
    //        return getDefaultWrite();
    //    default:
    //        throw std::runtime_error("descriptor type not handled!");
    //}
    return {};
}

VkCopyDescriptorSet Texture::Handler::getDescriptorCopy(const DESCRIPTOR& type) const {
    assert(false);
    // switch (type) {
    //    case DESCRIPTOR::DEFAULT_SAMPLER:
    //        return getDefaultCopy();
    //    default:
    //        throw std::runtime_error("descriptor type not handled!");
    //}
    return {};
}

std::future<std::shared_ptr<Texture::DATA>> Texture::Handler::loadTexture(const VkDevice& dev, const bool makeMipmaps,
                                                                          std::shared_ptr<Texture::DATA>& pTexture) {
    return std::async(std::launch::async, [pTexture]() {
        int width, height, channels;

        // Diffuse map (default)
        // TODO: make this dynamic like the others...
        pTexture->pixels = stbi_load(pTexture->path.c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (!pTexture->pixels) {
            throw std::runtime_error("failed to load texture map!");
        }

        pTexture->width = static_cast<uint32_t>(width);
        pTexture->height = static_cast<uint32_t>(height);
        pTexture->channels = static_cast<uint32_t>(channels);
        pTexture->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(pTexture->width, pTexture->height)))) + 1;
        pTexture->aspect = static_cast<float>(width) / static_cast<float>(height);

        // Normal map
        if (!pTexture->normPath.empty()) {
            pTexture->normPixels = stbi_load(pTexture->normPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

            if (!pTexture->normPixels) {
                throw std::runtime_error("failed to load normal map!");
            }

            std::stringstream ss;
            if (width != pTexture->width) ss << "invalid normal map (width)! ";
            if (height != pTexture->height) ss << "invalid normal map (height)! ";
            if (channels != pTexture->channels) ss << "invalid normal map (channels)! ";  // TODO: not sure about this
            if (!ss.str().empty()) {
                throw std::runtime_error(ss.str());
            }
        }

        // Spectral map
        if (!pTexture->specPath.empty()) {
            pTexture->specPixels = stbi_load(pTexture->specPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

            if (!pTexture->specPixels) {
                throw std::runtime_error("failed to load spectral map!");
            }

            std::stringstream ss;
            if (width != pTexture->width) ss << "invalid spectral map (width)! ";
            if (height != pTexture->height) ss << "invalid spectral map (height)! ";
            if (channels != pTexture->channels) ss << "invalid spectral map (channels)! ";  // TODO: not sure about this
            if (!ss.str().empty()) {
                throw std::runtime_error(ss.str());
            }
        }

        return std::move(pTexture);
    });
}

// thread sync
void Texture::Handler::createTexture(const bool makeMipmaps, std::shared_ptr<Texture::DATA> pTexture) {
    pTexture->pLdgRes = loadingHandler().createLoadingResources();

    auto& tex = (*pTexture);
    uint32_t layerCount = getArrayLayerCount(tex);

    createImage(tex, layerCount);

    if (makeMipmaps) {
        generateMipmaps(tex, layerCount);
    } else {
        // TODO: DO SOMETHING .. this was not tested
    }
    createImageView(shell().context().dev, tex, layerCount);
    createSampler(shell().context().dev, tex);
    createDescInfo(tex);

    loadingHandler().loadSubmit(std::move(pTexture->pLdgRes));

    pTexture->status = STATUS::READY;
}

void Texture::Handler::createImage(DATA& tex, uint32_t layerCount) {
    VkDeviceSize imageSize = tex.width * tex.height * 4;  // 4 components from STBI_rgb_alpha

    BufferResource stgRes = {};
    auto memReqsSize =
        helpers::createBuffer(shell().context().dev, (imageSize * layerCount), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              shell().context().mem_props, stgRes.buffer, stgRes.memory);

    void* pData;
    size_t offset = 0, size = static_cast<size_t>(imageSize);
    vk::assert_success(vkMapMemory(shell().context().dev, stgRes.memory, 0, memReqsSize, 0, &pData));

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

    vkUnmapMemory(shell().context().dev, stgRes.memory);

    stbi_image_free(tex.pixels);
    stbi_image_free(tex.normPixels);
    stbi_image_free(tex.specPixels);

    // Using CmdBufHandler::getUniqueQueueFamilies(true, false, true) here might not be wise... To work
    // right it relies on the the two command buffers being created with the same data.
    helpers::createImage(shell().context().dev, shell().context().mem_props,
                         commandHandler().getUniqueQueueFamilies(true, false, true), VK_SAMPLE_COUNT_1_BIT, STB_FORMAT,
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

void Texture::Handler::generateMipmaps(const Texture::DATA& tex, uint32_t layerCount) {
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

void Texture::Handler::createImageView(const VkDevice& dev, Texture::DATA& tex, uint32_t layerCount) {
    helpers::createImageView(dev, tex.image, tex.mipLevels, STB_FORMAT, VK_IMAGE_ASPECT_COLOR_BIT,
                             VK_IMAGE_VIEW_TYPE_2D_ARRAY, layerCount, tex.view);
}

void Texture::Handler::createSampler(const VkDevice& dev, Texture::DATA& tex) {
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

void Texture::Handler::createDescInfo(Texture::DATA& tex) {
    tex.imgDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    tex.imgDescInfo.imageView = tex.view;
    tex.imgDescInfo.sampler = tex.sampler;
}

uint32_t Texture::Handler::getArrayLayerCount(const Texture::DATA& tex) {
    uint32_t count = 0;
    if (tex.flags & FLAG::DIFFUSE) count++;
    if (tex.flags & FLAG::NORMAL) count++;
    if (tex.flags & FLAG::SPECULAR) count++;
    return count;
}
