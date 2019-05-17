
#include "TextureHandler.h"

#include <algorithm>
#include <sstream>
#include <stb_image.h>

#include "CommandHandler.h"
#include "LoadingHandler.h"
#include "Shell.h"
#include "ShaderHandler.h"

namespace {

void check_failure(const std::string path) {
    if (true) return;
    if (auto failure_reason = stbi_failure_reason()) {
        std::stringstream ss;
        ss << "Error: \"" << failure_reason << "\" in file \"" << path << std::endl;
        std::cout << ss.str();
    }
}

// TODO: get rid of all this garbage, and make it properly dynamic
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
Sampler::Base& getSampler(const std::shared_ptr<Texture::Base> pTexture, const Sampler::CHANNELS& type) {
    switch (type) {
        case Sampler::CHANNELS::_1:
            return pTexture->sampCh1;
            break;
        case Sampler::CHANNELS::_2:
            return pTexture->sampCh2;
            break;
        case Sampler::CHANNELS::_3:
            return pTexture->sampCh3;
            break;
        case Sampler::CHANNELS::_4:
            return pTexture->sampCh4;
            break;
        default:
            assert(false);
            throw std::runtime_error("Unhandled sampler channels");
    }
}

}  // namespace

Texture::Handler::Handler(Game* pGame) : Game::Handler(pGame) {}

void Texture::Handler::init() {
    reset();

    // DEFAULT TEXTURES
    Texture::CreateInfo info;

    info = {};
    info.colorPath = STATUE_TEX_PATH;
    addTexture(&info);

    info = {};
    info.colorPath = CHALET_TEX_PATH;
    addTexture(&info);

    info = {};
    info.colorPath = VULKAN_TEX_PATH;
    addTexture(&info);

    info = {};
    info.colorPath = MED_H_DIFF_TEX_PATH;
    info.normalPath = MED_H_NORM_TEX_PATH;
    info.specularPath = MED_H_SPEC_TEX_PATH;
    addTexture(&info);

    info = {};
    info.colorPath = HARDWOOD_FLOOR_TEX_PATH;
    addTexture(&info);

    info = {};
    info.colorPath = WOOD_007_DIFF_TEX_PATH;
    info.normalPath = WOOD_007_NORM_TEX_PATH;
    addTexture(&info);

    info = {};
    info.colorPath = MYBRICK_DIFF_TEX_PATH;
    info.normalPath = MYBRICK_NORM_TEX_PATH;
    info.heightPath = MYBRICK_HGHT_TEX_PATH;
    addTexture(&info);
}

void Texture::Handler::reset() {
    const auto& dev = shell().context().dev;
    for (auto& pTex : pTextures_) {
        pTex->sampCh1.destory(dev);
        pTex->sampCh2.destory(dev);
        pTex->sampCh3.destory(dev);
        pTex->sampCh4.destory(dev);
    }
    pTextures_.clear();
}

// This should be the only avenue for texture creation.
void Texture::Handler::addTexture(Texture::CreateInfo* pCreateInfo) {
    // TODO: do this here?
    std::replace(pCreateInfo->colorPath.begin(), pCreateInfo->colorPath.end(), '\\', '/');
    std::replace(pCreateInfo->normalPath.begin(), pCreateInfo->normalPath.end(), '\\', '/');
    std::replace(pCreateInfo->specularPath.begin(), pCreateInfo->specularPath.end(), '\\', '/');
    std::replace(pCreateInfo->alphaPath.begin(), pCreateInfo->alphaPath.end(), '\\', '/');
    std::replace(pCreateInfo->heightPath.begin(), pCreateInfo->heightPath.end(), '\\', '/');

    pTextures_.emplace_back(std::make_shared<Texture::Base>(static_cast<uint32_t>(pTextures_.size()), pCreateInfo));

    bool nonAsyncTest = false;
    if (nonAsyncTest) {
        auto result = load(pTextures_.back());
    } else {
        // Load the texture data...
        auto fut = loadTexture(pTextures_.back());
        // Store the loading future.
        texFutures_.emplace_back(std::move(fut));
    }
}

const std::shared_ptr<Texture::Base> Texture::Handler::getTextureByPath(std::string path) const {
    auto it = std::find_if(pTextures_.begin(), pTextures_.end(), [&path](auto& pTex) { return pTex->colorPath == path; });
    return it == pTextures_.end() ? nullptr : (*it);
}

void Texture::Handler::update() {
    // Check texture futures
    if (!texFutures_.empty()) {
        auto itFut = texFutures_.begin();

        while (itFut != texFutures_.end()) {
            auto& fut = (*itFut);

            // Check the status but don't wait...
            auto status = fut.wait_for(std::chrono::seconds(0));

            if (status == std::future_status::ready) {
                // Finish creating the texture
                createTexture(fut.get());

                // Remove the future from the list if all goes well.
                itFut = texFutures_.erase(itFut);

            } else {
                ++itFut;
            }
        }
    }
}

std::shared_ptr<Texture::Base> Texture::Handler::load(std::shared_ptr<Texture::Base>& pTexture) {
    // TODO: get rid of all this garbage, and make it properly dynamic

    // HEIGHT
    /*	This needs to be first in any potential sampler array because it affects the texture
            coordinate of subsequent samples.
            Note: If the bitmap is one channel and there is a normal map I should combine the normal
            map, and height map using the 4th channel for the height.
    */
    loadDataAndValidate(pTexture->heightPath, getReqComp(pTexture->heightChannels), pTexture,
                        getSampler(pTexture, pTexture->heightChannels));
    // COLOR (diffuse)
    loadDataAndValidate(pTexture->colorPath, getReqComp(pTexture->colorChannels), pTexture,
                        getSampler(pTexture, pTexture->colorChannels));
    // NORMAL
    loadDataAndValidate(pTexture->normalPath, getReqComp(pTexture->normalChannels), pTexture,
                        getSampler(pTexture, pTexture->normalChannels));
    // SPECULAR
    loadDataAndValidate(pTexture->specularPath, getReqComp(pTexture->specularChannels), pTexture,
                        getSampler(pTexture, pTexture->specularChannels));
    // ALPHA
    /*	Note: If the bitmap is one channel and there is a color map I should combine the color
                map, and alpha map using the 4th channel for the alpha.
    */
    loadDataAndValidate(pTexture->alphaPath, getReqComp(pTexture->alphaChannels), pTexture,
                        getSampler(pTexture, pTexture->alphaChannels));

    return pTexture;
}

void Texture::Handler::loadDataAndValidate(const std::string& path, int req_comp, std::shared_ptr<Texture::Base>& pTexture,
                                           Sampler::Base& texSampler) {
    if (path.empty()) return;

    int w, h, c;
    texSampler.pPixels.push_back(stbi_load(path.c_str(), &w, &h, &c, req_comp));
    check_failure(path);

    std::stringstream ss;
    if (!texSampler.pPixels.back()) {
        ss << "failed to load " << path << std::endl;
        assert(false);
        throw std::runtime_error(ss.str());
    }

    // Validate channels
    if (c < static_cast<int>(texSampler.NUM_CHANNELS)) {
        ss << std::endl
           << "Image at path \"" << path << "\" loaded for " << texSampler.NUM_CHANNELS << " channels but only has " << c
           << " channels." << std::endl;
        shell().log(Shell::LOG_WARN, ss.str().c_str());
        ss.str(std::string());
    }

    // Validate width
    if (texSampler.width == 0)
        texSampler.width = w;
    else if (w != texSampler.width)
        ss << "invalid " << path << " (width)! " << std::endl;

    // Validate height
    if (texSampler.height == 0)
        texSampler.height = h;
    else if (w != texSampler.height)
        ss << "invalid " << path << " (height)! " << std::endl;

    // Validate aspect
    float aspect = static_cast<float>(w) / static_cast<float>(h);
    if (pTexture->aspect == Texture::EMPTY_ASPECT)
        pTexture->aspect = aspect;
    else if (!helpers::almost_equal(aspect, pTexture->aspect, 1))
        ss << "invalid " << path << " (aspect)! " << std::endl;

    auto errMsg = ss.str();
    if (!errMsg.empty()) {
        assert(false);
        throw std::runtime_error(errMsg);
    }

    texSampler.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texSampler.width, texSampler.height)))) + 1;
}

std::future<std::shared_ptr<Texture::Base>> Texture::Handler::loadTexture(std::shared_ptr<Texture::Base>& pTexture) {
    return std::async(std::launch::async, &Texture::Handler::load, this, pTexture);
}

// thread sync
void Texture::Handler::createTexture(std::shared_ptr<Texture::Base> pTexture) {
    pTexture->pLdgRes = loadingHandler().createLoadingResources();

    createTextureSampler(pTexture, pTexture->sampCh1);
    createTextureSampler(pTexture, pTexture->sampCh2);
    createTextureSampler(pTexture, pTexture->sampCh3);
    createTextureSampler(pTexture, pTexture->sampCh4);

    loadingHandler().loadSubmit(std::move(pTexture->pLdgRes));

    pTexture->status = STATUS::READY;
}

void Texture::Handler::createTextureSampler(const std::shared_ptr<Texture::Base> pTexture, Sampler::Base& texSampler) {
    if (texSampler.layerCount() == 0) return;

    createImage(pTexture, texSampler);
    if (pTexture->makeMipmaps) {
        generateMipmaps(pTexture, texSampler);
    } else {
        // TODO: DO SOMETHING.. this was not tested
    }
    createImageView(shell().context().dev, texSampler);
    createSampler(shell().context().dev, texSampler);
    createDescInfo(texSampler);
}

void Texture::Handler::createImage(const std::shared_ptr<Texture::Base> pTexture, Sampler::Base& texSampler) {
    BufferResource stgRes = {};
    VkDeviceSize memorySize = texSampler.size();
    auto memReqsSize = helpers::createBuffer(shell().context().dev, memorySize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                             shell().context().mem_props, stgRes.buffer, stgRes.memory);

    void* pData;
    size_t offset = 0;
    vk::assert_success(vkMapMemory(shell().context().dev, stgRes.memory, 0, memReqsSize, 0, &pData));
    // Copy data to memory
    texSampler.copyData(pData, offset);

    vkUnmapMemory(shell().context().dev, stgRes.memory);

    // Using CmdBufHandler::getUniqueQueueFamilies(true, false, true) here might not be wise... To work
    // right it relies on the the two command buffers being created with the same data.
    helpers::createImage(shell().context().dev, shell().context().mem_props,
                         commandHandler().getUniqueQueueFamilies(true, false, true), VK_SAMPLE_COUNT_1_BIT,
                         texSampler.FORMAT, VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, static_cast<uint32_t>(texSampler.width),
                         static_cast<uint32_t>(texSampler.height), texSampler.mipLevels, texSampler.layerCount(),
                         texSampler.image, texSampler.memory);

    helpers::transitionImageLayout(pTexture->pLdgRes->transferCmd, texSampler.image, texSampler.FORMAT,
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, texSampler.mipLevels,
                                   texSampler.layerCount());

    helpers::copyBufferToImage(pTexture->pLdgRes->graphicsCmd, texSampler.width, texSampler.height, texSampler.layerCount(),
                               stgRes.buffer, texSampler.image);

    pTexture->pLdgRes->stgResources.push_back(stgRes);
}

void Texture::Handler::generateMipmaps(const std::shared_ptr<Texture::Base> pTexture, const Sampler::Base& texSampler) {
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
    barrier.image = texSampler.image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = texSampler.layerCount();
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texSampler.width;
    int32_t mipHeight = texSampler.height;

    for (uint32_t i = 1; i < texSampler.mipLevels; i++) {
        // CREATE MIP LEVEL

        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(pTexture->pLdgRes->graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit = {};
        // source
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = texSampler.layerCount();
        // destination
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = texSampler.layerCount();

        vkCmdBlitImage(pTexture->pLdgRes->graphicsCmd, texSampler.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       texSampler.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        // TRANSITION TO SHADER READY

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(pTexture->pLdgRes->graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        // This is a bit wonky methinks (non-sqaure is the case for this)
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // This is not handled in the loop so one more!!!! The last level is never never
    // blitted from.

    barrier.subresourceRange.baseMipLevel = texSampler.mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(pTexture->pLdgRes->graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}

void Texture::Handler::createImageView(const VkDevice& dev, Sampler::Base& texSampler) {
    helpers::createImageView(dev, texSampler.image, texSampler.mipLevels, texSampler.FORMAT, VK_IMAGE_ASPECT_COLOR_BIT,
                             VK_IMAGE_VIEW_TYPE_2D_ARRAY, texSampler.layerCount(), texSampler.view);
}

void Texture::Handler::createSampler(const VkDevice& dev, Sampler::Base& texSampler) {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // TODO: make some of these or all of these values dynamic
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
    samplerInfo.maxLod = static_cast<float>(texSampler.mipLevels);
    samplerInfo.mipLodBias = 0;  // Optional

    vk::assert_success(vkCreateSampler(dev, &samplerInfo, nullptr, &texSampler.sampler));

    // Name some objects for debugging
    ext::DebugMarkerSetObjectName(dev, (uint64_t)texSampler.sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT,
                                  (texSampler.NAME + " sampler").c_str());
}

void Texture::Handler::createDescInfo(Sampler::Base& texSampler) {
    texSampler.imgDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    texSampler.imgDescInfo.imageView = texSampler.view;
    texSampler.imgDescInfo.sampler = texSampler.sampler;
}
