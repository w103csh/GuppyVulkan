
#include "TextureHandler.h"

#include <algorithm>
#include <future>
#include <stb_image.h>
#include <variant>

#include "ConstantsAll.h"
#include "Deferred.h"
#include "ScreenSpace.h"
#include "Shadow.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "MaterialHandler.h"
#include "ShaderHandler.h"

Texture::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),  //
      perFramebufferSuffix_("(.*) #(\\d+)$") {}

void Texture::Handler::init() {
    reset();

    auto ssaoRandTexCreateInfo = Deferred::MakeSSAORandRotationTex();
    auto shadowOffsetTexCreateInfo = Shadow::MakeOffsetTex();

    // Transition storage images. I can't think of a better time to do this. Its
    // not great but oh well.
    std::vector<const CreateInfo*> pCreateInfos = {
        &Texture::STATUE_CREATE_INFO,
        &Texture::VULKAN_CREATE_INFO,
        &Texture::HARDWOOD_CREATE_INFO,
        &Texture::NEON_BLUE_TUX_GUPPY_CREATE_INFO,
        &Texture::BLUEWATER_CREATE_INFO,
        &Texture::FIRE_CREATE_INFO,
        &Texture::SMOKE_CREATE_INFO,
        &Texture::STAR_CREATE_INFO,
        &Texture::MEDIEVAL_HOUSE_CREATE_INFO,
        &Texture::WOOD_CREATE_INFO,
        &Texture::MYBRICK_CREATE_INFO,
        &Texture::PISA_HDR_CREATE_INFO,
        &Texture::SKYBOX_CREATE_INFO,
        &RenderPass::DEFAULT_2D_TEXTURE_CREATE_INFO,
        &RenderPass::PROJECT_2D_TEXTURE_CREATE_INFO,
        &RenderPass::PROJECT_2D_ARRAY_TEXTURE_CREATE_INFO,
        &RenderPass::SWAPCHAIN_TARGET_TEXTURE_CREATE_INFO,
        &Texture::ScreenSpace::DEFAULT_2D_TEXTURE_CREATE_INFO,
        //&Texture::ScreenSpace::COMPUTE_2D_TEXTURE_CREATE_INFO,
        &Texture::ScreenSpace::HDR_LOG_2D_TEXTURE_CREATE_INFO,
        &Texture::ScreenSpace::HDR_LOG_BLIT_A_2D_TEXTURE_CREATE_INFO,
        &Texture::ScreenSpace::HDR_LOG_BLIT_B_2D_TEXTURE_CREATE_INFO,
        &Texture::ScreenSpace::BLUR_A_2D_TEXTURE_CREATE_INFO,
        &Texture::ScreenSpace::BLUR_B_2D_TEXTURE_CREATE_INFO,
        //&Texture::Deferred::POS_NORM_2D_ARRAY_CREATE_INFO,
        &Texture::Deferred::POS_2D_CREATE_INFO,
        &Texture::Deferred::NORM_2D_CREATE_INFO,
        &Texture::Deferred::DIFFUSE_2D_CREATE_INFO,
        &Texture::Deferred::AMBIENT_2D_CREATE_INFO,
        &Texture::Deferred::SPECULAR_2D_CREATE_INFO,
        &Texture::Deferred::SSAO_2D_CREATE_INFO,
        &ssaoRandTexCreateInfo,
        &Texture::Shadow::MAP_2D_ARRAY_CREATE_INFO,
        &shadowOffsetTexCreateInfo,
    };

    // I think this does not get set properly, so I am not sure where the texture generation
    // should be initialized.
    assert(shell().context().imageCount == 3);

    for (const auto& pCreateInfo : pCreateInfos) {
        if (!pCreateInfo->perFramebuffer) {
            // Just make normally.
            make(pCreateInfo);
        } else {
            // Make a texture per framebuffer.
            for (uint32_t i = 0; i < shell().context().imageCount; i++) {
                auto textureInfo = *pCreateInfo;
                // Append frame index suffixes so that the id's are unique.
                textureInfo.name += Texture::Handler::getIdSuffix(i);
                for (auto& sampInfo : textureInfo.samplerCreateInfos) sampInfo.name += Texture::Handler::getIdSuffix(i);
                // Make texture
                make(&textureInfo);
            }
        }
    }
}

void Texture::Handler::reset() {
    const auto& dev = shell().context().dev;
    for (auto& pTexture : pTextures_) {
        for (auto& sampler : pTexture->samplers) {
            if (pTexture->status != STATUS::DESTROYED) {
                sampler.destroy(dev);
            }
        }
    }
    pTextures_.clear();
}

std::shared_ptr<Texture::Base>& Texture::Handler::make(const Texture::CreateInfo* pCreateInfo) {
    // Using the name as an ID for now, so make sure its unique.
    for (const auto& pTexture : pTextures_) {
        assert(pTexture->NAME.compare(pCreateInfo->name) != 0);
    }

    pTextures_.emplace_back(std::make_shared<Texture::Base>(static_cast<uint32_t>(pTextures_.size()), pCreateInfo));

    if (pCreateInfo->needsData) {
        bool nonAsyncTest = false;
        if (nonAsyncTest) {
            load(pTextures_.back(), pCreateInfo);
        } else {
            // Load the texture data...
            texFutures_.emplace_back(
                std::async(std::launch::async, &Texture::Handler::asyncLoad, this, pTextures_.back(), *pCreateInfo));
        }
    } else {
        // Prepare samplers.
        load(pTextures_.back(), pCreateInfo);
        // Check if the texture is ready to be created on the device.
        bool isReady = true;
        for (auto& sampler : pTextures_.back()->samplers) {
            if (sampler.swpchnInfo.usesSwapchain()) {
                isReady = false;
                break;
            }
        }

        if (isReady) {
            // Some of these textures can have pixel data, and as a result need to be
            // staged, and copied.
            bool stageResources = false;
            for (const auto& sampler : pTextures_.back()->samplers)
                if (sampler.pPixels.size()) stageResources = true;
            // Finish the texture creation.
            createTexture(pTextures_.back(), stageResources);
        } else {
            pTextures_.back()->status = STATUS::PENDING_SWAPCHAIN;
        }
    }

    return pTextures_.back();
}

const std::shared_ptr<Texture::Base> Texture::Handler::getTexture(const std::string_view& textureId) const {
    for (const auto& pTexture : pTextures_)
        if (pTexture->NAME == textureId) return pTexture;
    return nullptr;
}

const std::shared_ptr<Texture::Base> Texture::Handler::getTexture(const uint32_t index) const {
    if (index < pTextures_.size()) return pTextures_[index];
    return nullptr;
}

const std::shared_ptr<Texture::Base> Texture::Handler::getTexture(const std::string_view& textureId,
                                                                  const uint8_t frameIndex) const {
    auto textureIdWithSuffix = std::string(textureId) + Texture::Handler::getIdSuffix(frameIndex);
    return getTexture(textureIdWithSuffix);
}

bool Texture::Handler::update(std::shared_ptr<Texture::Base> pTexture) {
    // Update swapchain dependent textures
    if (pTexture != nullptr) {
        assert(pTexture->status != STATUS::READY);
        assert(pTexture->usesSwapchain);
        for (auto& sampler : pTexture->samplers) {
            if (sampler.swpchnInfo.usesExtent) {
                const auto& extent = shell().context().extent;
                sampler.imgCreateInfo.extent.width =
                    static_cast<uint32_t>(static_cast<float>(extent.width) * sampler.swpchnInfo.extentFactor);
                sampler.imgCreateInfo.extent.height =
                    static_cast<uint32_t>(static_cast<float>(extent.height) * sampler.swpchnInfo.extentFactor);
                if (sampler.mipmapInfo.usesExtent)
                    sampler.imgCreateInfo.mipLevels = Sampler::GetMipLevels(sampler.imgCreateInfo.extent);
            }
            if (sampler.swpchnInfo.usesFormat) {
                sampler.imgCreateInfo.format = shell().context().surfaceFormat.format;
            }
        }
        bool wasDestroyed = pTexture->status == STATUS::DESTROYED;
        createTexture(pTexture, false);
        return wasDestroyed;
    }

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

    return false;
}

std::shared_ptr<Texture::Base> Texture::Handler::asyncLoad(std::shared_ptr<Texture::Base> pTexture, CreateInfo createInfo) {
    load(pTexture, &createInfo);
    return pTexture;
}

void Texture::Handler::load(std::shared_ptr<Texture::Base>& pTexture, const CreateInfo* pCreateInfo) {
    for (auto& samplerCreateInfo : pCreateInfo->samplerCreateInfos) {
        pTexture->samplers.emplace_back(Sampler::make(shell(), &samplerCreateInfo, pCreateInfo->needsData));

        assert(!pTexture->samplers.empty());
        auto& sampler = pTexture->samplers.back();

        if (pCreateInfo->needsData) assert(sampler.aspect != BAD_ASPECT);

        // Set texture-wide info
        pTexture->flags |= sampler.flags;
        pTexture->usesSwapchain |= sampler.swpchnInfo.usesSwapchain();

        if (sampler.aspect != BAD_ASPECT) {
            if (pTexture->aspect == BAD_ASPECT) {
                pTexture->aspect = sampler.aspect;
            } else if (!helpers::almost_equal(sampler.aspect, pTexture->aspect, 1)) {
                std::string msg = "\nSampler \"" + samplerCreateInfo.name + "\" has an inconsistent aspect.\n";
                shell().log(Shell::LogPriority::LOG_WARN, msg.c_str());
            }
        }
    }
    assert(pTexture->samplers.size() && pTexture->samplers.size() == pCreateInfo->samplerCreateInfos.size());
}

// thread sync
void Texture::Handler::createTexture(std::shared_ptr<Texture::Base> pTexture, bool stageResources) {
    assert(pTexture->samplers.size());

    // Dataless textures don't need to be staged
    if (stageResources) pTexture->pLdgRes = loadingHandler().createLoadingResources();
    // Create the texture on the device
    for (auto& sampler : pTexture->samplers) makeTexture(pTexture, sampler);

    if (stageResources) loadingHandler().loadSubmit(std::move(pTexture->pLdgRes));
    pTexture->status = STATUS::READY;

    // Notify any materials to update their data
    materialHandler().updateTexture(pTexture);
}

void Texture::Handler::makeTexture(std::shared_ptr<Texture::Base>& pTexture, Sampler::Base& sampler) {
    assert(sampler.imgCreateInfo.arrayLayers > 0);

    if (sampler.imgCreateInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        createDepthImage(sampler, pTexture->pLdgRes);
    } else {
        createImage(sampler, pTexture->pLdgRes);
    }

    if (sampler.mipmapInfo.generateMipmaps && sampler.imgCreateInfo.mipLevels > 1) {
        generateMipmaps(sampler, pTexture->pLdgRes);
    } else if (std::visit(Descriptor::IsInputAttachment{}, pTexture->DESCRIPTOR_TYPE)) {
        // TODO: Test this once the framerate gets slower to see if using VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL as
        // the initial layout is faster.

        // helpers::transitionImageLayout(commandHandler().graphicsCmd(), sampler.image, sampler.imgCreateInfo.format,
        //                               VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        //                               VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        //                               sampler.imgCreateInfo.mipLevels, sampler.imgCreateInfo.arrayLayers);
    } else if (pTexture->pLdgRes != nullptr) {
        /* As of now there are memory barries (transitions) in "createImages", "generateMipmaps",
         *	and here. Its kind of confusing and should potentionally be combined into one call
         *	to vkCmdPipelineBarrier. A single call might not be possible because you
         *	can/should use different queues based on staging and things.
         */
        helpers::transitionImageLayout(pTexture->pLdgRes->graphicsCmd, sampler.image, sampler.imgCreateInfo.format,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                       VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                       sampler.imgCreateInfo.mipLevels, sampler.imgCreateInfo.arrayLayers);
    }

    for (auto& [layer, layerResource] : sampler.layerResourceMap) {
        uint32_t layerCount = (layer == Sampler::IMAGE_ARRAY_LAYERS_ALL) ? sampler.imgCreateInfo.arrayLayers : 1;
        uint32_t baseArrayLayer = (layer == Sampler::IMAGE_ARRAY_LAYERS_ALL) ? 0 : layer;
        // Image view
        createImageView(shell().context().dev, sampler, baseArrayLayer, layerCount, layerResource);
        // Sampler (optional)
        if (layerResource.hasSampler) createSampler(shell().context().dev, sampler, layerResource);

        createDescInfo(pTexture, layer, layerResource, sampler);
    }

    sampler.cleanup();
}

void Texture::Handler::createImage(Sampler::Base& sampler, std::unique_ptr<LoadingResource>& pLdgRes) {
    // Loading data only settings for image
    if (pLdgRes != nullptr) {
        assert(sampler.imgCreateInfo.arrayLayers == sampler.pPixels.size());
        sampler.imgCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    } else {
        assert(sampler.pPixels.empty());
    }

    // Using CmdBufHandler::getUniqueQueueFamilies(true, false, true) here might not be wise... To work
    // right it relies on the the two command buffers being created with the same data.
    auto queueFamilyIndices = commandHandler().getUniqueQueueFamilies(true, false, true, false);
    sampler.imgCreateInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
    sampler.imgCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();

    // Samples
    if (sampler.swpchnInfo.usesSamples) {
        sampler.imgCreateInfo.samples = shell().context().samples;
    }

    // Create image
    vk::assert_success(vkCreateImage(shell().context().dev, &sampler.imgCreateInfo, nullptr, &sampler.image));

    VkMemoryPropertyFlags memFlags;
    if (sampler.NAME.find("Deferred 2D Array Position/Normal Sampler") != std::string::npos ||
        sampler.NAME.find("Deferred 2D Color Sampler") != std::string::npos) {
        // Test to see if the device has any memory with the lazily allocated bit.
        memFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(shell().context().dev, sampler.image, &memReqs);
        uint32_t heapIndex;
        if (helpers::getMemoryType(shell().context().memProps, memReqs.memoryTypeBits, memFlags, &heapIndex)) {
            assert(false);  // Never tested the lazy allocation bit. Check to see if all is well!!!
        } else {
            // Section 10.2 of the spec has a list of heap types and the one we want when doing deferred for attachments
            // is VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT. The check above is for
            // testing when I use a gpu that has one of these heaps.
            memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            // TODO: the getMemoryType function stinks. If I use VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
            // VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT its picks the worst kind of memory.
        }
    } else {
        memFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    // Allocate memory
    helpers::createImageMemory(shell().context().dev, shell().context().memProps, memFlags, sampler.image, sampler.memory);

    // If loading data create a staging buffer, and copy/transition the data to the image.
    if (pLdgRes != nullptr) {
        BufferResource stgRes = {};
        VkDeviceSize memorySize = sampler.size();
        auto memReqsSize = helpers::createBuffer(shell().context().dev, memorySize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                 shell().context().memProps, stgRes.buffer, stgRes.memory);

        void* pData;
        size_t offset = 0;
        vk::assert_success(vkMapMemory(shell().context().dev, stgRes.memory, 0, memReqsSize, 0, &pData));
        // Copy data to memory
        sampler.copyData(pData, offset);

        vkUnmapMemory(shell().context().dev, stgRes.memory);

        helpers::transitionImageLayout(pLdgRes->transferCmd, sampler.image, sampler.imgCreateInfo.format,
                                       VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                       sampler.imgCreateInfo.mipLevels, sampler.imgCreateInfo.arrayLayers);

        helpers::copyBufferToImage(pLdgRes->graphicsCmd, sampler.imgCreateInfo.extent.width,
                                   sampler.imgCreateInfo.extent.height, sampler.imgCreateInfo.arrayLayers, stgRes.buffer,
                                   sampler.image);

        pLdgRes->stgResources.push_back(stgRes);
    }
}

void Texture::Handler::createDepthImage(Sampler::Base& sampler, std::unique_ptr<LoadingResource>& pLdgRes) {
    assert(pLdgRes == nullptr);
    assert(sampler.pPixels.empty());

    sampler.imgCreateInfo.format = shell().context().depthFormat;

    auto queueFamilyIndices = commandHandler().getUniqueQueueFamilies(true, false, false, false);
    sampler.imgCreateInfo.queueFamilyIndexCount = commandHandler().graphicsIndex();
    sampler.imgCreateInfo.pQueueFamilyIndices = queueFamilyIndices.data();

    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(shell().context().physicalDev, sampler.imgCreateInfo.format, &props);
    if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        sampler.imgCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
    } else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        sampler.imgCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    } else {
        // TODO: Try other depth formats.
        assert(false && "depth format unsupported");
        exit(EXIT_FAILURE);
    }

    // Create image
    vk::assert_success(vkCreateImage(shell().context().dev, &sampler.imgCreateInfo, nullptr, &sampler.image));

    // Allocate memory
    helpers::createImageMemory(shell().context().dev, shell().context().memProps, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                               sampler.image, sampler.memory);
}

void Texture::Handler::generateMipmaps(Sampler::Base& sampler, std::unique_ptr<LoadingResource>& pLdgRes) {
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
    barrier.subresourceRange.layerCount = sampler.imgCreateInfo.arrayLayers;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = sampler.imgCreateInfo.extent.width;
    int32_t mipHeight = sampler.imgCreateInfo.extent.height;

    for (uint32_t i = 1; i < sampler.imgCreateInfo.mipLevels; i++) {
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
        blit.srcSubresource.layerCount = sampler.imgCreateInfo.arrayLayers;
        // destination
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = sampler.imgCreateInfo.arrayLayers;

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

    barrier.subresourceRange.baseMipLevel = sampler.imgCreateInfo.mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(pLdgRes->graphicsCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0,
                         nullptr, 0, nullptr, 1, &barrier);
}

void Texture::Handler::createImageView(const VkDevice& dev, const Sampler::Base& sampler, const uint32_t baseArrayLayer,
                                       const uint32_t layerCount, Sampler::LayerResource& layerResource) {
    VkImageAspectFlags aspectFlags;
    if (sampler.imgCreateInfo.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (helpers::hasStencilComponent(sampler.imgCreateInfo.format)) aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
    } else {
        aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    VkImageSubresourceRange range = {aspectFlags, 0, sampler.imgCreateInfo.mipLevels, baseArrayLayer, layerCount};
    helpers::createImageView(dev, sampler.image, sampler.imgCreateInfo.format, sampler.imageViewType, range,
                             layerResource.view);
}

void Texture::Handler::createSampler(const VkDevice& dev, const Sampler::Base& sampler,
                                     Sampler::LayerResource& layerResource) {
    VkSamplerCreateInfo samplerInfo = Sampler::GetVkSamplerCreateInfo(sampler);
    vk::assert_success(vkCreateSampler(dev, &samplerInfo, nullptr, &layerResource.sampler));
    // Name some objects for debugging
    ext::DebugMarkerSetObjectName(dev, (uint64_t)layerResource.sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT,
                                  (sampler.NAME + " sampler").c_str());
}

void Texture::Handler::createDescInfo(std::shared_ptr<Texture::Base>& pTexture, const uint32_t layerKey,
                                      const Sampler::LayerResource& layerResource, Sampler::Base& sampler) {
    if (pTexture->status == STATUS::DESTROYED)
        assert(sampler.imgInfo.descInfoMap.count(layerKey) == 1);
    else
        assert(sampler.imgInfo.descInfoMap.count(layerKey) == 0);

    sampler.imgInfo.descInfoMap[layerKey] = {
        layerResource.sampler,
        layerResource.view,
        std::visit(Descriptor::GetTextureImageLayout{}, pTexture->DESCRIPTOR_TYPE),
    };

    sampler.imgInfo.image = sampler.image;
}

void Texture::Handler::attachSwapchain() {
    // Update swapchain dependent textures
    std::vector<std::string> updateList;
    for (auto& pTexture : pTextures_) {
        // The swapchain texture should not use this code. Its final setup is done in the pass handler.
        if (std::visit(Descriptor::IsSwapchainStorageImage{}, pTexture->DESCRIPTOR_TYPE)) continue;
        if (pTexture->usesSwapchain && pTexture->status != STATUS::READY) {
            // Only dealing single sampler textures atm.
            assert(pTexture->samplers.size() == 1);
            if (pTexture->samplers[0].swpchnInfo.usesSwapchain()) {
                if (update(pTexture)) {
                    std::smatch sm;
                    if (std::regex_match(pTexture->NAME, sm, perFramebufferSuffix_)) {
                        assert(sm.size() == 3);
                        if (std::stoi(sm[2]) == 0) updateList.push_back(sm[1]);
                    } else {
                        updateList.push_back(pTexture->NAME);
                    }
                }
            }
        }
    }

    // Notify the descriptor handler of all the remade textures
    if (updateList.size()) descriptorHandler().updateBindData(updateList);

    // Transition storage images. I can't think of a better time to do this. Its
    // not great, but oh well.
    for (const auto& pTexture : pTextures_) {
        // The swapchain texture should not use this code. Its final setup is done in the pass handler.
        if (std::visit(Descriptor::IsSwapchainStorageImage{}, pTexture->DESCRIPTOR_TYPE)) continue;

        if (std::visit(Descriptor::IsStorageImage{}, pTexture->DESCRIPTOR_TYPE) && pTexture->PER_FRAMEBUFFER) {
            for (const auto& sampler : pTexture->samplers) {
                // TODO: Make an actual barrier. Not sure if it will ever be necessary
                helpers::transitionImageLayout(commandHandler().transferCmd(), sampler.image, sampler.imgCreateInfo.format,
                                               VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL,
                                               VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 1, 1);
            }
        }
    }
    // This will leave the global transfer command in a bad spot, but I don't like any of
    // the command handler. It was the first thing I wrote, and makes no sense now that I
    // know better.
    commandHandler().endCmd(commandHandler().transferCmd());

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandHandler().transferCmd();
    vk::assert_success(vkQueueSubmit(commandHandler().transferQueue(), 1, &submitInfo, nullptr));
}

void Texture::Handler::detachSwapchain() {
    for (auto& pTexture : pTextures_) {
        if (std::visit(Descriptor::IsSwapchainStorageImage{}, pTexture->DESCRIPTOR_TYPE)) continue;
        if (pTexture->usesSwapchain) {
            for (auto& sampler : pTexture->samplers) {
                if (sampler.swpchnInfo.usesSwapchain()) {
                    pTexture->destroy(shell().context().dev);
                }
            }
        }
        // TODO: Will the imageCount ever change? If it can then
        // PER_FRAMEBUFFER images need to also be remade/or checked
        // here.
    }
}
