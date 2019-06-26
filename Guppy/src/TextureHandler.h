#ifndef TEXTURE_HANDLER_H
#define TEXTURE_HANDLER_H

#include <future>
#include <unordered_map>
#include <vector>

#include "Game.h"
#include "Texture.h"

namespace Texture {

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame);

    void init() override;
    inline void destroy() override { reset(); }

    // Note: pCreateInfo will be copied because of asynchronous loading, so all
    // the members of Texture::CreateInfo should be trivial.
    std::shared_ptr<Texture::Base>& make(const Texture::CreateInfo* pCreateInfo);

    const std::shared_ptr<Texture::Base> getTextureByName(const std::string_view& name) const;
    void update();

    const std::shared_ptr<Texture::Base> getTexture(uint32_t index) {
        if (index < pTextures_.size()) return pTextures_[index];
        return nullptr;
    }
    inline uint32_t getCount() { return static_cast<uint32_t>(pTextures_.size()); }

    // TODO: should these be public? Moved these for render to sampler.
    void createSampler(const VkDevice& dev, Sampler::Base& texSampler);
    void createDescInfo(Sampler::Base& texSampler);

   private:
    void reset() override;

    std::shared_ptr<Texture::Base> asyncLoad(std::shared_ptr<Texture::Base> pTexture, CreateInfo createInfo);
    void load(std::shared_ptr<Texture::Base>& pTexture, const CreateInfo* pCreateInfo);

    void createTexture(std::shared_ptr<Texture::Base> pTexture);
    void createTextureSampler(const std::shared_ptr<Texture::Base> pTexture, Sampler::Base& texSampler);
    void createImage(Sampler::Base& sampler, std::unique_ptr<Loading::Resources>& pLdgRes);
    void generateMipmaps(Sampler::Base& sampler, std::unique_ptr<Loading::Resources>& pLdgRes);
    void createImageView(const VkDevice& dev, Sampler::Base& texSampler);

    std::vector<std::shared_ptr<Texture::Base>> pTextures_;
    std::vector<std::future<std::shared_ptr<Texture::Base>>> texFutures_;
};

}  // namespace Texture

#endif  // !TEXTURE_HANDLER_H