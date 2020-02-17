/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef TEXTURE_HANDLER_H
#define TEXTURE_HANDLER_H

#include <future>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Game.h"
#include "Texture.h"

namespace Texture {

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame);

    void init() override;
    inline void tick() override { update(); };
    inline void destroy() override { reset(); }

    // Note: pCreateInfo will be copied because of asynchronous loading, so all
    // the members of Texture::CreateInfo should be trivial.
    std::shared_ptr<Texture::Base>& make(const Texture::CreateInfo* pCreateInfo);

    void makeBufferView(const std::string_view& id, const vk::Format format, const vk::DeviceSize size, void* pData);

    const std::shared_ptr<Texture::Base> getTexture(const std::string_view& name) const;
    const std::shared_ptr<Texture::Base> getTexture(const uint32_t index) const;
    const std::shared_ptr<Texture::Base> getTexture(const std::string_view& name, const uint8_t frameIndex) const;
    const BufferView::Base* getBufferView(const std::string_view& id) const;
    inline uint32_t getCount() { return static_cast<uint32_t>(pTextures_.size()); }

    // TODO: should these be public? Moved these for render to sampler.
    void createSampler(const Context& ctx, const Sampler::Base& sampler, Sampler::LayerResource& layerResource);

    // TODO: most of these functions should be static
    static void createDescInfo(std::shared_ptr<Texture::Base>& pTexture, const uint32_t layerKey,
                               const Sampler::LayerResource& layerResource, Sampler::Base& sampler);

    // SWAPCHAIN
    void attachSwapchain();
    void detachSwapchain();

    static inline std::string getIdSuffix(const uint32_t index) { return (" #" + std::to_string(index)); }

   private:
    void reset() override;

    bool update(std::shared_ptr<Texture::Base> pTexture = nullptr);

    std::shared_ptr<Texture::Base> asyncLoad(std::shared_ptr<Texture::Base> pTexture, CreateInfo createInfo);
    void load(std::shared_ptr<Texture::Base>& pTexture, const CreateInfo* pCreateInfo);

    void createTexture(std::shared_ptr<Texture::Base> pTexture, bool stageResources = true);
    void makeTexture(std::shared_ptr<Texture::Base>& pTexture, Sampler::Base& texSampler);
    void createImage(Sampler::Base& sampler, std::unique_ptr<LoadingResource>& pLdgRes);
    void createDepthImage(Sampler::Base& sampler, std::unique_ptr<LoadingResource>& pLdgRes);
    void generateMipmaps(Sampler::Base& sampler, std::unique_ptr<LoadingResource>& pLdgRes);
    void createImageView(const Context& ctx, const Sampler::Base& sampler, const uint32_t baseArrayLayer,
                         const uint32_t layerCount, Sampler::LayerResource& layerResource);

    std::vector<std::shared_ptr<Texture::Base>> pTextures_;
    std::vector<std::future<std::shared_ptr<Texture::Base>>> texFutures_;

    std::vector<BufferView::Base> bufferViews_;

    // REGEX
    std::regex perFramebufferSuffix_;
};

}  // namespace Texture

#endif  // !TEXTURE_HANDLER_H