
#include "TextureHandler.h"

#include <algorithm>
#include <future>
#include <stb_image.h>
#include <variant>

#include "ConstantsAll.h"
#include "ScreenSpace.h"
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

    // Transition storage images. I can't think of a better time to do this. Its
    // not great but oh well.
    std::vector<const CreateInfo*> pCreateInfos = {
        &Texture::STATUE_CREATE_INFO,
        &Texture::VULKAN_CREATE_INFO,
        &Texture::HARDWOOD_CREATE_INFO,
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

    if (pCreateInfo->hasData) {
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
            if (sampler.usesSwapchain) {
                isReady = false;
                break;
            }
        }

        if (isReady) {
            createTexture(pTextures_.back(), false);
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
        // Currently this way of creation is only going to be used for dataless textures
        // that rely on the parameters of the swapchain.
        assert(pTexture->usesSwapchain);
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
        pTexture->samplers.emplace_back(Sampler::make(shell(), &samplerCreateInfo, pCreateInfo->hasData));

        assert(!pTexture->samplers.empty());
        auto& sampler = pTexture->samplers.back();

        if (pCreateInfo->hasData) assert(sampler.aspect != BAD_ASPECT);

        // Set texture-wide info
        pTexture->flags |= sampler.flags;
        pTexture->usesSwapchain |= sampler.usesSwapchain;

        if (sampler.aspect != BAD_ASPECT) {
            if (pTexture->aspect == BAD_ASPECT) {
                pTexture->aspect = sampler.aspect;
            } else if (!helpers::almost_equal(sampler.aspect, pTexture->aspect, 1)) {
                std::string msg = "\nSampler \"" + samplerCreateInfo.name + "\" has an inconsistent aspect.\n";
                shell().log(Shell::LOG_WARN, msg.c_str());
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
    for (auto& sampler : pTexture->samplers) createTextureSampler(pTexture, sampler);

    if (stageResources) loadingHandler().loadSubmit(std::move(pTexture->pLdgRes));
    pTexture->status = STATUS::READY;

    // Notify any materials to update their data
    materialHandler().updateTexture(pTexture);
}

void Texture::Handler::createTextureSampler(const std::shared_ptr<Texture::Base> pTexture, Sampler::Base& sampler) {
    assert(sampler.arrayLayers > 0);

    createImage(sampler, pTexture->pLdgRes);
    if (sampler.mipLevels > 1) {
        generateMipmaps(sampler, pTexture->pLdgRes);
    } else if (pTexture->pLdgRes != nullptr) {
        /* As of now there are memory barries (transitions) in "createImages", "generateMipmaps",
         *	and here. Its kind of confusing and should potentionally be combined into one call
         *	to vkCmdPipelineBarrier. A single call might not be possible because you
         *	can/should use different queues based on staging and things.
         */
        helpers::transitionImageLayout(pTexture->pLdgRes->graphicsCmd, sampler.image, sampler.format,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                       VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                       sampler.mipLevels, sampler.arrayLayers);
    }
    createImageView(shell().context().dev, sampler);
    createSampler(shell().context().dev, sampler);
    createDescInfo(pTexture->DESCRIPTOR_TYPE, sampler);

    sampler.cleanup();
}

void Texture::Handler::createImage(Sampler::Base& sampler, std::unique_ptr<Loading::Resources>& pLdgRes) {
    VkImageCreateInfo imageInfo = sampler.getImageCreateInfo();

    // Loading data only settings for image
    if (pLdgRes != nullptr) {
        assert(sampler.arrayLayers == sampler.pPixels.size());
        imageInfo.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    } else {
        assert(sampler.pPixels.empty());
    }

    // Using CmdBufHandler::getUniqueQueueFamilies(true, false, true) here might not be wise... To work
    // right it relies on the the two command buffers being created with the same data.
    auto queueFamilyIndices = commandHandler().getUniqueQueueFamilies(true, false, true);
    imageInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
    imageInfo.pQueueFamilyIndices = queueFamilyIndices.data();

    helpers::createImage(shell().context().dev, imageInfo, shell().context().memProps, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                         sampler.image, sampler.memory);

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

        helpers::transitionImageLayout(pLdgRes->transferCmd, sampler.image, sampler.format, VK_IMAGE_LAYOUT_UNDEFINED,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                       VK_PIPELINE_STAGE_TRANSFER_BIT, sampler.mipLevels, sampler.arrayLayers);

        helpers::copyBufferToImage(pLdgRes->graphicsCmd, sampler.extent.width, sampler.extent.height, sampler.arrayLayers,
                                   stgRes.buffer, sampler.image);

        pLdgRes->stgResources.push_back(stgRes);
    }
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

    int32_t mipWidth = sampler.extent.width;
    int32_t mipHeight = sampler.extent.height;

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
    helpers::createImageView(dev, sampler.image, sampler.mipLevels, sampler.format, VK_IMAGE_ASPECT_COLOR_BIT,
                             sampler.imageViewType, sampler.arrayLayers, sampler.view);
}

void Texture::Handler::createSampler(const VkDevice& dev, Sampler::Base& sampler) {
    VkSamplerCreateInfo samplerInfo = Sampler::GetVkSamplerCreateInfo(sampler);
    vk::assert_success(vkCreateSampler(dev, &samplerInfo, nullptr, &sampler.sampler));
    // Name some objects for debugging
    ext::DebugMarkerSetObjectName(dev, (uint64_t)sampler.sampler, VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT,
                                  (sampler.NAME + " sampler").c_str());
}

void Texture::Handler::createDescInfo(const DESCRIPTOR& descType, Sampler::Base& sampler) {
    sampler.imgInfo.descInfo.imageLayout = std::visit(Descriptor::GetTextureImageLayout{}, descType);
    sampler.imgInfo.descInfo.imageView = sampler.view;
    sampler.imgInfo.descInfo.sampler = sampler.sampler;
    sampler.imgInfo.image = sampler.image;
}

void Texture::Handler::attachSwapchain() {
    const auto& ctx = shell().context();

    // Update swapchain dependent textures
    std::vector<std::string> updateList;
    for (auto& pTexture : pTextures_) {
        // The swapchain texture should not use this code. Its final setup is done in the pass handler.
        if (std::visit(Descriptor::IsSwapchainStorageImage{}, pTexture->DESCRIPTOR_TYPE)) continue;

        if (pTexture->usesSwapchain && pTexture->status != STATUS::READY) {
            for (auto& sampler : pTexture->samplers) {
                if (sampler.usesSwapchain) {
                    sampler.extent = ctx.extent;
                    sampler.format = ctx.surfaceFormat.format;
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
                helpers::transitionImageLayout(commandHandler().transferCmd(), sampler.image, sampler.format,
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
                if (sampler.usesSwapchain) {
                    pTexture->destroy(shell().context().dev);
                }
            }
        }
        // TODO: Will the imageCount ever change? If it can then
        // PER_FRAMEBUFFER images need to also be remade/or checked
        // here.
    }
}
