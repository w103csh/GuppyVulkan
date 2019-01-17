
#include <algorithm>

#include "Sampler.h"
#include "ShaderHandler.h"
#include "Shell.h"
#include "TextureHandler.h"

namespace {

// TODO: move to a Sampler class if I need a bunch of these...

VkDescriptorSetLayoutBinding&& getDefaultSamplerBindings(
    uint32_t binding = static_cast<uint32_t>(DESCRIPTOR_TYPE::DEFAULT_SAMPLER), uint32_t count = 1) {
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = binding;
    layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    layoutBinding.descriptorCount = count;
    layoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    layoutBinding.pImmutableSamplers = nullptr;  // Optional
    return std::move(layoutBinding);
}

VkWriteDescriptorSet getDefaultWrite() {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = static_cast<uint32_t>(DESCRIPTOR_TYPE::DEFAULT_SAMPLER);
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    return write;
}

VkCopyDescriptorSet getDefaultCopy() {
    /*  1/9/19 I am not using this code because I think that this is only really necessary
        if you want to reuse the writes but with different bindings... I am currently not
        to sure about any of this.
    */
    VkCopyDescriptorSet copy = {};
    copy.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
    copy.srcBinding = static_cast<uint32_t>(DESCRIPTOR_TYPE::DEFAULT_UNIFORM);
    copy.srcArrayElement = 0;
    copy.dstBinding = static_cast<uint32_t>(DESCRIPTOR_TYPE::DEFAULT_UNIFORM);
    copy.dstArrayElement = 0;
    return copy;
}

}  // namespace

TextureHandler TextureHandler::inst_;

void TextureHandler::init(Shell* pShell, const Game::Settings& settings) {
    inst_.reset();

    inst_.sh_ = pShell;
    inst_.ctx_ = pShell->context();
    inst_.settings_ = settings;

    // Default textures
    addTexture(STATUE_TEX_PATH);
    addTexture(CHALET_TEX_PATH);
    addTexture(VULKAN_TEX_PATH);
    addTexture(MED_H_DIFF_TEX_PATH, MED_H_NORM_TEX_PATH, MED_H_SPEC_TEX_PATH);
    addTexture(HARDWOOD_FLOOR_TEX_PATH);
    addTexture(WOOD_007_DIFF_TEX_PATH, WOOD_007_NORM_TEX_PATH);
}

void TextureHandler::reset() {
    for (auto& pTex : pTextures_) {
        vkDestroySampler(sh_->context().dev, pTex->sampler, nullptr);
        vkDestroyImageView(sh_->context().dev, pTex->view, nullptr);
        vkDestroyImage(sh_->context().dev, pTex->image, nullptr);
        vkFreeMemory(sh_->context().dev, pTex->memory, nullptr);
    }
    pTextures_.clear();
}

// This should be the only avenue for texture creation.
void TextureHandler::addTexture(std::string path, std::string normPath, std::string specPath) {
    for (auto& tex : inst_.pTextures_) {
        if (tex->path == path) inst_.sh_->log(Shell::LOG_WARN, "Texture with same path was already loaded.");
        if (!tex->normPath.empty() && tex->normPath == normPath)
            inst_.sh_->log(Shell::LOG_WARN, "Texture with same normal path was already loaded.");
        if (!tex->specPath.empty() && tex->specPath == specPath)
            inst_.sh_->log(Shell::LOG_WARN, "Texture with same spectral path was already loaded.");
    }

    // If this asserts read the comment where TEXTURE_LIMIT is declared.
    assert(inst_.pTextures_.size() < TEXTURE_LIMIT);

    // Make the texture.
    inst_.pTextures_.emplace_back(std::make_shared<Texture::Data>(inst_.pTextures_.size(), path, normPath, specPath));
    // Load the texture data...
    auto fut = inst_.loadTexture(inst_.sh_->context().dev, true, inst_.pTextures_.back());
    // Store the loading future.
    inst_.texFutures_.emplace_back(std::move(fut));
}

std::shared_ptr<Texture::Data> TextureHandler::getTextureByPath(std::string path) {
    auto it =
        std::find_if(inst_.pTextures_.begin(), inst_.pTextures_.end(), [&path](auto& pTex) { return pTex->path == path; });
    return it == inst_.pTextures_.end() ? nullptr : (*it);
}

void TextureHandler::update() {
    // Check texture futures
    if (!inst_.texFutures_.empty()) {
        bool wasUpdated = false;
        auto itFut = inst_.texFutures_.begin();

        while (itFut != inst_.texFutures_.end()) {
            auto& fut = (*itFut);

            // Check the status but don't wait...
            auto status = fut.wait_for(std::chrono::seconds(0));

            if (status == std::future_status::ready) {
                // Finish creating the texture
                auto& tex = fut.get();

                Texture::createTexture(inst_.sh_->context().dev, true, tex);

                // Remove the future from the list if all goes well.
                itFut = inst_.texFutures_.erase(itFut);
                wasUpdated = true;

            } else {
                ++itFut;
            }
        }

        if (wasUpdated) Shader::Handler::updateDefaultDynamicUniform();
    }
}

VkDescriptorSetLayoutBinding TextureHandler::getDescriptorLayoutBinding(const DESCRIPTOR_TYPE& type) {
    switch (type) {
        case DESCRIPTOR_TYPE::DEFAULT_SAMPLER:
            return getDefaultSamplerBindings();
        default:
            throw std::runtime_error("descriptor type not handled!");
    }
}

VkWriteDescriptorSet TextureHandler::getDescriptorWrite(const DESCRIPTOR_TYPE& type) {
    switch (type) {
        case DESCRIPTOR_TYPE::DEFAULT_SAMPLER:
            return getDefaultWrite();
        default:
            throw std::runtime_error("descriptor type not handled!");
    }
}

VkCopyDescriptorSet TextureHandler::getDescriptorCopy(const DESCRIPTOR_TYPE& type) {
    switch (type) {
        case DESCRIPTOR_TYPE::DEFAULT_SAMPLER:
            return getDefaultCopy();
        default:
            throw std::runtime_error("descriptor type not handled!");
    }
}

std::future<std::shared_ptr<Texture::Data>> TextureHandler::loadTexture(const VkDevice& dev, const bool makeMipmaps,
                                                                        std::shared_ptr<Texture::Data>& pTexture) {
    pTexture->pLdgRes = LoadingResourceHandler::createLoadingResources();

    return std::async(std::launch::async, [&dev, &makeMipmaps, pTexture]() {
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
