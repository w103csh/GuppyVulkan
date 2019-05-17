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

    void addTexture(Texture::CreateInfo* pCreateInfo);
    const std::shared_ptr<Texture::Base> getTextureByPath(std::string path) const;
    void update();

    const std::shared_ptr<Texture::Base> getTexture(uint32_t index) {
        if (index < pTextures_.size()) return pTextures_[index];
        return nullptr;
    }
    inline uint32_t getCount() { return static_cast<uint32_t>(pTextures_.size()); }

   private:
    void reset() override;

    std::shared_ptr<Texture::Base> load(std::shared_ptr<Texture::Base>& pTexture);
    void loadDataAndValidate(const std::string& path, int req_comp, std::shared_ptr<Texture::Base>& pTexture,
                             Sampler::Base& texSampler);
    std::future<std::shared_ptr<Texture::Base>> loadTexture(std::shared_ptr<Texture::Base>& pTexture);

    void createTexture(std::shared_ptr<Texture::Base> pTexture);
    void createTextureSampler(const std::shared_ptr<Texture::Base> pTexture, Sampler::Base& texSampler);
    void createImage(const std::shared_ptr<Texture::Base> pTexture, Sampler::Base& texSampler);
    void generateMipmaps(const std::shared_ptr<Texture::Base> pTexture, const Sampler::Base& texSampler);
    void createImageView(const VkDevice& dev, Sampler::Base& texSampler);
    void createSampler(const VkDevice& dev, Sampler::Base& texSampler);

    void createDescInfo(Sampler::Base& texSampler);

    std::vector<std::shared_ptr<Texture::Base>> pTextures_;
    std::vector<std::future<std::shared_ptr<Texture::Base>>> texFutures_;
};

}  // namespace Texture

#endif  // !TEXTURE_HANDLER_H