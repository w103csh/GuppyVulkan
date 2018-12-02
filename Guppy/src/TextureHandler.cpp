
#include <algorithm>

#include "MyShell.h"
#include "TextureHandler.h"

TextureHandler TextureHandler::inst_;

void TextureHandler::init(MyShell* sh) {
    inst_.sh_ = sh;

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

void TextureHandler::addTexture(std::string path, std::string normPath, std::string specPath) {
    for (auto& tex : inst_.pTextures_) {
        if (tex->path == path) inst_.sh_->log(MyShell::LOG_WARN, "Texture with same path was already loaded.");
        if (!tex->normPath.empty() && tex->normPath == normPath)
            inst_.sh_->log(MyShell::LOG_WARN, "Texture with same normal path was already loaded.");
        if (!tex->specPath.empty() && tex->specPath == specPath)
            inst_.sh_->log(MyShell::LOG_WARN, "Texture with same spectral path was already loaded.");
    }
    // make texture and a loading future
    auto pTexture = std::make_shared<Texture::Data>(inst_.pTextures_.size(), path, normPath, specPath);
    auto fut = Texture::loadTexture(inst_.sh_->context().dev, true, pTexture);
    // move texture and future to the vectors
    inst_.pTextures_.emplace_back(std::move(pTexture));
    inst_.texFutures_.emplace_back(std::move(fut));
}

std::shared_ptr<Texture::Data> TextureHandler::getTextureByPath(std::string path) {
    auto it = std::find_if(inst_.pTextures_.begin(), inst_.pTextures_.end(), [&path](auto& pTex) { return pTex->path == path; });
    return it == inst_.pTextures_.end() ? nullptr : (*it);
}

void TextureHandler::update() {
    // Check texture futures
    if (!inst_.texFutures_.empty()) {
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

            } else {
                ++itFut;
            }
        }
    }
}
