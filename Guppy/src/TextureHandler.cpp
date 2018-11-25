
#include <algorithm>

#include "MyShell.h"
#include "TextureHandler.h"

TextureHandler TextureHandler::inst_;

void TextureHandler::init(MyShell* sh) { inst_.sh_ = sh; }

void TextureHandler::reset() {
    for (auto& pTex : textures_) {
        vkDestroySampler(sh_->context().dev, pTex->sampler, nullptr);
        vkDestroyImageView(sh_->context().dev, pTex->view, nullptr);
        vkDestroyImage(sh_->context().dev, pTex->image, nullptr);
        vkFreeMemory(sh_->context().dev, pTex->memory, nullptr);
    }
    textures_.clear();
}

void TextureHandler::addTexture(std::string path, std::string normPath, std::string specPath) {
    for (auto& tex : inst_.textures_) {
        if (tex->path == path) inst_.sh_->log(MyShell::LOG_WARN, "Texture with same path was already loaded.");
        if (!tex->normPath.empty() && tex->normPath == normPath)
            inst_.sh_->log(MyShell::LOG_WARN, "Texture with same normal path was already loaded.");
        if (!tex->specPath.empty() && tex->specPath == specPath)
            inst_.sh_->log(MyShell::LOG_WARN, "Texture with same spectral path was already loaded.");
    }
    // make texture and a loading future
    auto pTexture = std::make_shared<Texture::Data>(inst_.textures_.size(), path, normPath, specPath);
    auto fut = Texture::loadTexture(inst_.sh_->context().dev, true, pTexture);
    // move texture and future to the vectors
    inst_.textures_.emplace_back(std::move(pTexture));
    inst_.texFutures_.emplace_back(std::move(fut));
}

std::shared_ptr<Texture::Data> TextureHandler::getTextureByPath(std::string path) {
    auto it = std::find_if(inst_.textures_.begin(), inst_.textures_.end(), [&path](auto& pTex) { return pTex->path == path; });
    return it == inst_.textures_.end() ? nullptr : (*it);
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
