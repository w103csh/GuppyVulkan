
#include "TextureHandler.h"

#include <algorithm>
#include <stb_image.h>

#include "CommandHandler.h"
#include "Constants.h"
#include "LoadingHandler.h"
#include "Shell.h"
#include "ShaderHandler.h"

Texture::Handler::Handler(Game* pGame) : Game::Handler(pGame) {}

void Texture::Handler::init() {
    reset();

    if (false) return;

    make(&Texture::STATUE_CREATE_INFO);
    make(&Texture::VULKAN_CREATE_INFO);
    make(&Texture::HARDWOOD_CREATE_INFO);
    make(&Texture::MEDIEVAL_HOUSE_CREATE_INFO);
    make(&Texture::WOOD_CREATE_INFO);
    make(&Texture::MYBRICK_CREATE_INFO);
}

void Texture::Handler::reset() {
    const auto& dev = shell().context().dev;
    for (auto& pTexture : pTextures_) {
        for (auto& sampler : pTexture->samplers) {
            sampler.destroy(dev);
        }
    }
    pTextures_.clear();
}

std::shared_ptr<Texture::Base>& Texture::Handler::make(const Texture::CreateInfo* pCreateInfo) {
    pTextures_.emplace_back(std::make_shared<Texture::Base>(static_cast<uint32_t>(pTextures_.size()), pCreateInfo));

    bool nonAsyncTest = false;
    if (nonAsyncTest) {
        auto result = load(pTextures_.back(), *pCreateInfo);
    } else {
        // Load the texture data...
        texFutures_.emplace_back(
            std::async(std::launch::async, &Texture::Handler::load, this, pTextures_.back(), *pCreateInfo));
    }

    return pTextures_.back();
}

const std::shared_ptr<Texture::Base> Texture::Handler::getTextureByName(std::string name) const {
    auto it = std::find_if(pTextures_.begin(), pTextures_.end(), [&name](auto& pTexture) { return pTexture->NAME == name; });
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

std::shared_ptr<Texture::Base> Texture::Handler::load(std::shared_ptr<Texture::Base>& pTexture, CreateInfo createInfo) {
    pTexture->aspect = FLT_MAX;
    for (auto& samplerCreateInfo : createInfo.samplerCreateInfos) {
        pTexture->samplers.push_back(Sampler::Base(shell(), &samplerCreateInfo));

        assert(!pTexture->samplers.empty());
        auto& sampler = pTexture->samplers.back();

        // set texture-wide info
        pTexture->flags |= sampler.flags;

        float aspect = static_cast<float>(sampler.width) / static_cast<float>(sampler.height);
        if (pTexture->aspect == FLT_MAX) {
            pTexture->aspect = aspect;
        } else if (!helpers::almost_equal(aspect, pTexture->aspect, 1)) {
            std::string msg = "\nSampler \"" + samplerCreateInfo.name + "\" has an inconsistent aspect.\n";
            shell().log(Shell::LOG_WARN, msg.c_str());
        }
    }
    assert(!pTexture->samplers.empty());
    return pTexture;
}

// thread sync
void Texture::Handler::createTexture(std::shared_ptr<Texture::Base> pTexture) {
    pTexture->pLdgRes = loadingHandler().createLoadingResources();
    for (auto& sampler : pTexture->samplers) createTextureSampler(pTexture, sampler);
    loadingHandler().loadSubmit(std::move(pTexture->pLdgRes));
    pTexture->status = STATUS::READY;
}

void Texture::Handler::createTextureSampler(const std::shared_ptr<Texture::Base> pTexture, Sampler::Base& sampler) {
    assert(sampler.arrayLayers > 0 && sampler.arrayLayers == sampler.pPixels.size());

    createImage(sampler, pTexture->pLdgRes);
    if (sampler.mipLevels > 1) {
        generateMipmaps(sampler, pTexture->pLdgRes);
    }
    createImageView(shell().context().dev, sampler);
    createSampler(shell().context().dev, sampler);
    createDescInfo(sampler);

    sampler.cleanup();
}

void Texture::Handler::createImage(Sampler::Base& sampler, std::unique_ptr<Loading::Resources>& pLdgRes) {
    BufferResource stgRes = {};
    VkDeviceSize memorySize = sampler.size();
    auto memReqsSize = helpers::createBuffer(shell().context().dev, memorySize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                             shell().context().mem_props, stgRes.buffer, stgRes.memory);

    void* pData;
    size_t offset = 0;
    vk::assert_success(vkMapMemory(shell().context().dev, stgRes.memory, 0, memReqsSize, 0, &pData));
    // Copy data to memory
    sampler.copyData(pData, offset);

    vkUnmapMemory(shell().context().dev, stgRes.memory);

    VkImageCreateInfo imageInfo = sampler.getImageCreateInfo();

    imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    // Using CmdBufHandler::getUniqueQueueFamilies(true, false, true) here might not be wise... To work
    // right it relies on the the two command buffers being created with the same data.
    auto queueFamilyIndices = commandHandler().getUniqueQueueFamilies(true, false, true);
    imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
    imageInfo.pQueueFamilyIndices = queueFamilyIndices.data();

    helpers::createImage(shell().context().dev, imageInfo, shell().context().mem_props, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         sampler.image, sampler.memory);

    helpers::transitionImageLayout(pLdgRes->transferCmd, sampler.image, sampler.FORMAT, VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT, sampler.mipLevels, sampler.arrayLayers);

    helpers::copyBufferToImage(pLdgRes->graphicsCmd, sampler.width, sampler.height, sampler.arrayLayers, stgRes.buffer,
                               sampler.image);

    pLdgRes->stgResources.push_back(stgRes);
}

void Texture::Handler::generateMipmaps(Sampler::Base& sampler, std::unique_ptr<Loading::Resources>& pLdgRes) {
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
    barrier.image = sampler.image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = sampler.arrayLayers;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = sampler.width;
    int32_t mipHeight = sampler.height;

    for (uint32_t i = 1; i < sampler.mipLevels; i++) {
        // CREATE MIP LEVEL

        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(pLdgRes->graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
                             nullptr, 0, nullptr, 1, &barrier);

        VkImageBlit blit = {};
        // source
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = sampler.arrayLayers;
        // destination
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = sampler.arrayLayers;

        vkCmdBlitImage(pLdgRes->graphicsCmd, sampler.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, sampler.image,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

        // TRANSITION TO SHADER READY

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(pLdgRes->graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr, 0, nullptr, 1, &barrier);

        // This is a bit wonky methinks (non-sqaure is the case for this)
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // This is not handled in the loop so one more!!!! The last level is never never
    // blitted from.

    barrier.subresourceRange.baseMipLevel = sampler.mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(pLdgRes->graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);
}

void Texture::Handler::createImageView(const VkDevice& dev, Sampler::Base& sampler) {
    helpers::createImageView(dev, sampler.image, sampler.mipLevels, sampler.FORMAT, VK_IMAGE_ASPECT_COLOR_BIT,
                             sampler.imageViewType, sampler.arrayLayers, sampler.view);
}

void Texture::Handler::createSampler(const VkDevice& dev, Sampler::Base& sampler) {
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
    samplerInfo.maxLod = static_cast<float>(sampler.mipLevels);
    samplerInfo.mipLodBias = 0;  // Optional

    vk::assert_success(vkCreateSampler(dev, &samplerInfo, nullptr, &sampler.sampler));

    // Name some objects for debugging
    ext::DebugMarkerSetObjectName(dev, (uint64_t)sampler.sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT,
                                  (sampler.NAME + " sampler").c_str());
}

void Texture::Handler::createDescInfo(Sampler::Base& sampler) {
    sampler.imgDescInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    sampler.imgDescInfo.imageView = sampler.view;
    sampler.imgDescInfo.sampler = sampler.sampler;
}
