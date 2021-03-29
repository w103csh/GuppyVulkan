/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "TextureHandler.h"

#include <algorithm>
#include <future>
#include <stb_image.h>
#include <variant>

#include "ConstantsAll.h"
#include "Deferred.h"
#include "FFT.h"
#include "Ocean.h"
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
    auto skyboxNightTexCreateInfo = Texture::MakeCubeMapTex(Texture::SKYBOX_NIGHT_ID, SAMPLER::DEFAULT_NEAREST, 1024);
    auto fftTestTexCreateInfo = Texture::FFT::MakeTestTex();
#if OCEAN_USE_COMPUTE_QUEUE_DISPATCH
    auto oceanBltTexInfo = Texture::Ocean::MakeCopyTexInfo(::Ocean::N, ::Ocean::M);
#endif

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
        &Texture::FABRIC_BROWN_CREATE_INFO,
        &Texture::BRIGHT_MOON_CREATE_INFO,
        &Texture::CIRCLES_CREATE_INFO,
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
        &Texture::Deferred::FLAGS_2D_CREATE_INFO,
        &Texture::Deferred::SSAO_2D_CREATE_INFO,
        &ssaoRandTexCreateInfo,
        &Texture::Shadow::MAP_2D_ARRAY_CREATE_INFO,
        &shadowOffsetTexCreateInfo,
        &skyboxNightTexCreateInfo,
        &fftTestTexCreateInfo,
#if OCEAN_USE_COMPUTE_QUEUE_DISPATCH
        &oceanBltTexInfo,
#endif
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
    const auto& ctx = shell().context();
    // TEXTURES
    for (auto& pTexture : pTextures_) {
        for (auto& sampler : pTexture->samplers) {
            if (pTexture->status != STATUS::DESTROYED) {
                sampler.destroy(ctx);
            }
        }
    }
    pTextures_.clear();
    // BUFFER VIEWS
    for (auto& bv : bufferViews_) {
        ctx.dev.destroyBufferView(bv.view, ctx.pAllocator);
        ctx.dev.destroyBuffer(bv.buffRes.buffer, ctx.pAllocator);
        ctx.dev.freeMemory(bv.buffRes.memory, ctx.pAllocator);
    }
    bufferViews_.clear();
}

std::shared_ptr<Texture::Base>& Texture::Handler::make(const Texture::CreateInfo* pCreateInfo) {
    for (const auto& samplerCreateInfo : pCreateInfo->samplerCreateInfos) {
        /**
         * This assert here is for the MoltenVK limitation:
         *  error 'MTLTextureDescriptor has width (...) greater than the maximum allowed size of 16384.'
         * I would imagine there is a similar limitation for height, but I have not hit the problem yet.
         */
        if (!helpers::compExtent3D(samplerCreateInfo.extent, BAD_EXTENT_3D) && samplerCreateInfo.extent.width > 16384) {
            assert(false);
            exit(EXIT_FAILURE);
        }
    }

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

void Texture::Handler::makeBufferView(const std::string_view& id, const vk::Format format, const vk::DeviceSize size,
                                      void* pData) {
    // Just make a buffer per make call for now. Not sure if I will use this enough for it to matter.
    const auto& ctx = shell().context();
    bufferViews_.push_back({id});
    bufferViews_.back().pLdgRes = loadingHandler().createLoadingResources();

    BufferResource stgRes = {};
    ctx.createBuffer(bufferViews_.back().pLdgRes->transferCmd,
                     vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformTexelBuffer, size,
                     std::string(id) + " uniform texel buffer", stgRes, bufferViews_.back().buffRes, pData);
    bufferViews_.back().pLdgRes->stgResources.push_back(stgRes);

    loadingHandler().loadSubmit(std::move(bufferViews_.back().pLdgRes));

    vk::BufferViewCreateInfo createInfo = {};
    createInfo.format = format;
    createInfo.range = size;
    createInfo.buffer = bufferViews_.back().buffRes.buffer;
    bufferViews_.back().view = ctx.dev.createBufferView(createInfo, ctx.pAllocator);

    bufferViews_.back().status = STATUS::READY;
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

const BufferView::Base* Texture::Handler::getBufferView(const std::string_view& id) const {
    for (auto& bv : bufferViews_)
        if (bv.id == id) return &bv;
    return nullptr;
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

    if (sampler.imgCreateInfo.usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
        createDepthImage(sampler, pTexture->pLdgRes);
    } else {
        createImage(sampler, pTexture->pLdgRes);
    }

    if (sampler.mipmapInfo.generateMipmaps && sampler.imgCreateInfo.mipLevels > 1) {
        generateMipmaps(sampler, pTexture->pLdgRes);
    } else if (pTexture->pLdgRes == nullptr) {
        if (!std::visit(Descriptor::IsInputAttachment{}, pTexture->DESCRIPTOR_TYPE)) {
            // shell().log(Shell::LogPriority::LOG_INFO, ("Transitioning image: " + pTexture->NAME).c_str());
            vk::ImageMemoryBarrier barrier = {};
            barrier.srcAccessMask = {};
            barrier.dstAccessMask = {};
            barrier.oldLayout = vk::ImageLayout::eUndefined;
            barrier.newLayout = std::visit(Descriptor::GetImageInitalTransitionLayout{}, pTexture->DESCRIPTOR_TYPE);
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // ignored for now
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;  // ignored for now
            barrier.image = sampler.image;
            barrier.subresourceRange.aspectMask = (pTexture->DESCRIPTOR_TYPE == DESCRIPTOR{COMBINED_SAMPLER::PIPELINE_DEPTH})
                                                      ? vk::ImageAspectFlagBits::eDepth
                                                      : vk::ImageAspectFlagBits::eColor;
            barrier.subresourceRange.levelCount = sampler.imgCreateInfo.mipLevels;
            barrier.subresourceRange.layerCount = sampler.imgCreateInfo.arrayLayers;
            commandHandler().transferCmd().pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                                           vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, {barrier});
        }
    } else if (pTexture->pLdgRes != nullptr) {
        /* As of now there are memory barries (transitions) in "createImages", "generateMipmaps",
         *	and here. Its kind of confusing and should potentionally be combined into one call
         *	to pipelineBarrier. A single call might not be possible because you
         *	can/should use different queues based on staging and things.
         */
        helpers::transitionImageLayout(pTexture->pLdgRes->graphicsCmd, sampler.image, sampler.imgCreateInfo.format,
                                       vk::ImageLayout::eTransferDstOptimal,
                                       std::visit(Descriptor::GetTextureImageLayout{}, pTexture->DESCRIPTOR_TYPE),
                                       vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                                       sampler.imgCreateInfo.mipLevels, sampler.imgCreateInfo.arrayLayers);
    }

    for (auto& [layer, layerResource] : sampler.layerResourceMap) {
        uint32_t layerCount = (layer == Sampler::IMAGE_ARRAY_LAYERS_ALL) ? sampler.imgCreateInfo.arrayLayers : 1;
        uint32_t baseArrayLayer = (layer == Sampler::IMAGE_ARRAY_LAYERS_ALL) ? 0 : layer;
        // Image view
        createImageView(shell().context(), sampler, baseArrayLayer, layerCount, layerResource);
        // Sampler (optional)
        if (layerResource.hasSampler) createSampler(shell().context(), sampler, layerResource);

        createDescInfo(pTexture, layer, layerResource, sampler);
    }

    sampler.cleanup();
}

void Texture::Handler::createImage(Sampler::Base& sampler, std::unique_ptr<LoadingResource>& pLdgRes) {
    // Loading data only settings for image
    if (pLdgRes != nullptr) {
        assert(sampler.imgCreateInfo.arrayLayers == sampler.pPixels.size());
        sampler.imgCreateInfo.usage |= vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst;
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
    sampler.image = shell().context().dev.createImage(sampler.imgCreateInfo, shell().context().pAllocator);

    vk::MemoryPropertyFlags memFlags;
    if (sampler.NAME.find("Deferred 2D Array Position/Normal Sampler") != std::string::npos ||
        sampler.NAME.find("Deferred 2D Color Sampler") != std::string::npos) {
        // Test to see if the device has any memory with the lazily allocated bit.
        memFlags = vk::MemoryPropertyFlagBits::eLazilyAllocated;
        vk::MemoryRequirements memReqs = shell().context().dev.getImageMemoryRequirements(sampler.image);
        uint32_t heapIndex;
        if (helpers::getMemoryType(shell().context().memProps, memReqs.memoryTypeBits, memFlags, &heapIndex)) {
            assert(false);  // Never tested the lazy allocation bit. Check to see if all is well!!!
        } else {
            // Section 10.2 of the spec has a list of heap types and the one we want when doing deferred for attachments
            // is vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eLazilyAllocated. The check above is
            // for testing when I use a gpu that has one of these heaps.
            memFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
            // TODO: the getMemoryType function stinks. If I use vk::MemoryPropertyFlagBits::eDeviceLocal |
            // vk::MemoryPropertyFlagBits::eLazilyAllocated its picks the worst kind of memory.
        }
    } else {
        memFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
    }

    // Allocate memory
    helpers::createImageMemory(shell().context().dev, shell().context().memProps, memFlags, sampler.image, sampler.memory,
                               shell().context().pAllocator);

    // If loading data create a staging buffer, and copy/transition the data to the image.
    if (pLdgRes != nullptr) {
        BufferResource stgRes = {};
        vk::DeviceSize memorySize = sampler.size();
        auto memReqsSize =
            helpers::createBuffer(shell().context().dev, memorySize, vk::BufferUsageFlagBits::eTransferSrc,
                                  vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                                  shell().context().memProps, stgRes.buffer, stgRes.memory, shell().context().pAllocator);

        void* pData = shell().context().dev.mapMemory(stgRes.memory, 0, memReqsSize);
        size_t offset = 0;
        // Copy data to memory
        sampler.copyData(pData, offset);

        shell().context().dev.unmapMemory(stgRes.memory);

        helpers::transitionImageLayout(pLdgRes->transferCmd, sampler.image, sampler.imgCreateInfo.format,
                                       vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
                                       vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer,
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

    vk::FormatProperties props = shell().context().physicalDev.getFormatProperties(sampler.imgCreateInfo.format);
    if (props.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
        // bool b = vk::FormatFeatureFlagBits::eColorAttachmentBlend & props.linearTilingFeatures;
        sampler.imgCreateInfo.tiling = vk::ImageTiling::eLinear;
    } else if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
        // bool b = vk::FormatFeatureFlagBits::eColorAttachmentBlend & props.optimalTilingFeatures;
        sampler.imgCreateInfo.tiling = vk::ImageTiling::eOptimal;
    } else {
        // TODO: Try other depth formats.
        assert(false && "depth format unsupported");
        exit(EXIT_FAILURE);
    }

    // Create image
    sampler.image = shell().context().dev.createImage(sampler.imgCreateInfo, shell().context().pAllocator);

    // Allocate memory
    helpers::createImageMemory(shell().context().dev, shell().context().memProps, vk::MemoryPropertyFlagBits::eDeviceLocal,
                               sampler.image, sampler.memory, shell().context().pAllocator);
}

void Texture::Handler::generateMipmaps(Sampler::Base& sampler, std::unique_ptr<LoadingResource>& pLdgRes) {
    // This was the way before mip maps
    // transitionImageLayout(
    //    srcQueueFamilyIndexFinal,
    //    dstQueueFamilyIndexFinal,
    //    m_mipLevels,
    //    image,
    //    vk::Format::eR8G8B8A8Unorm,
    //    vk::ImageLayout::eTransferDstOptimal,
    //    vk::ImageLayout::eShaderReadOnlyOptimal,
    //    m_transferCommandPool
    //);

    // transition to vk::ImageLayout::eShaderReadOnlyOptimal while generating mipmaps

    vk::ImageMemoryBarrier barrier = {};
    barrier.image = sampler.image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = sampler.imgCreateInfo.arrayLayers;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = sampler.imgCreateInfo.extent.width;
    int32_t mipHeight = sampler.imgCreateInfo.extent.height;

    for (uint32_t i = 1; i < sampler.imgCreateInfo.mipLevels; i++) {
        // CREATE MIP LEVEL

        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        pLdgRes->graphicsCmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {},
                                             {}, {}, {barrier});

        vk::ImageBlit blit = {};
        // source
        blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = sampler.imgCreateInfo.arrayLayers;
        // destination
        blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.dstOffsets[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = sampler.imgCreateInfo.arrayLayers;

        pLdgRes->graphicsCmd.blitImage(sampler.image, vk::ImageLayout::eTransferSrcOptimal, sampler.image,
                                       vk::ImageLayout::eTransferDstOptimal, 1, &blit, vk::Filter::eLinear);

        // TRANSITION TO SHADER READY

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        pLdgRes->graphicsCmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                             vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, {barrier});

        // This is a bit wonky methinks (non-sqaure is the case for this)
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // This is not handled in the loop so one more!!!! The last level is never never
    // blitted from.

    barrier.subresourceRange.baseMipLevel = sampler.imgCreateInfo.mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    pLdgRes->graphicsCmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader,
                                         {}, {}, {}, {barrier});
}

void Texture::Handler::createImageView(const Context& ctx, const Sampler::Base& sampler, const uint32_t baseArrayLayer,
                                       const uint32_t layerCount, Sampler::LayerResource& layerResource) {
    vk::ImageAspectFlags aspectFlags;
    if (sampler.imgCreateInfo.usage & vk::ImageUsageFlagBits::eDepthStencilAttachment) {
        aspectFlags = vk::ImageAspectFlagBits::eDepth;
        if (helpers::hasStencilComponent(sampler.imgCreateInfo.format)) aspectFlags |= vk::ImageAspectFlagBits::eStencil;
    } else {
        aspectFlags = vk::ImageAspectFlagBits::eColor;
    }
    vk::ImageSubresourceRange range = {aspectFlags, 0, sampler.imgCreateInfo.mipLevels, baseArrayLayer, layerCount};
    helpers::createImageView(ctx.dev, sampler.image, sampler.imgCreateInfo.format, sampler.imageViewType, range,
                             layerResource.view, ctx.pAllocator);
}

void Texture::Handler::createSampler(const Context& ctx, const Sampler::Base& sampler,
                                     Sampler::LayerResource& layerResource) {
    vk::SamplerCreateInfo samplerInfo = Sampler::GetVulkanSamplerCreateInfo(sampler);
    layerResource.sampler = ctx.dev.createSampler(samplerInfo, ctx.pAllocator);
    // shell().context().dbg.setMarkerName(layerResource.sampler, (sampler.NAME + " sampler").c_str());
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
                                               vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral,
                                               vk::PipelineStageFlagBits::eAllCommands,
                                               vk::PipelineStageFlagBits::eAllCommands, 1, 1);
            }
        }
    }
    // This will leave the global transfer command in a bad spot, but I don't like any of
    // the command handler. It was the first thing I wrote, and makes no sense now that I
    // know better.
    commandHandler().transferCmd().end();

    vk::SubmitInfo submitInfo = {};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandHandler().transferCmd();
    commandHandler().transferQueue().submit({submitInfo}, {});
}

void Texture::Handler::detachSwapchain() {
    for (auto& pTexture : pTextures_) {
        if (std::visit(Descriptor::IsSwapchainStorageImage{}, pTexture->DESCRIPTOR_TYPE)) continue;
        if (pTexture->usesSwapchain) {
            for (auto& sampler : pTexture->samplers) {
                if (sampler.swpchnInfo.usesSwapchain()) {
                    pTexture->destroy(shell().context());
                }
            }
        }
        // TODO: Will the imageCount ever change? If it can then
        // PER_FRAMEBUFFER images need to also be remade/or checked
        // here.
    }
}
