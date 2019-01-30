#ifndef TEXTURE_HANDLER_H
#define TEXTURE_HANDLER_H

#include <future>
#include <unordered_map>
#include <vector>

#include "Game.h"
#include "Texture.h"

// class Game;

namespace Texture {

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame);

    void init() override;
    inline void destroy() { reset(); }

    void addTexture(std::string path, std::string normPath = "", std::string specPath = "");
    const std::shared_ptr<Texture::DATA> getTextureByPath(std::string path) const;
    void update();

    const std::shared_ptr<Texture::DATA> getTexture(uint32_t index) {
        if (index < pTextures_.size()) return pTextures_[index];
        return nullptr;
    }
    uint32_t getCount() { return static_cast<uint32_t>(pTextures_.size()); }

    // DESCRIPTOR
    VkDescriptorSetLayoutBinding getDescriptorLayoutBinding(const DESCRIPTOR& type) const;
    VkWriteDescriptorSet getDescriptorWrite(const DESCRIPTOR& type) const;
    VkCopyDescriptorSet getDescriptorCopy(const DESCRIPTOR& type) const;

   private:
    void reset() override;

    std::future<std::shared_ptr<Texture::DATA>> loadTexture(const VkDevice& dev, const bool makeMipmaps,
                                                            std::shared_ptr<Texture::DATA>& pTexture);

    void createTexture(const bool makeMipmaps, std::shared_ptr<Texture::DATA> pTexture);
    void createImage(Texture::DATA& tex, uint32_t layerCount);
    void createImageView(const VkDevice& dev, Texture::DATA& tex, uint32_t layerCount);
    void createSampler(const VkDevice& dev, Texture::DATA& tex);
    void createDescInfo(Texture::DATA& tex);
    void generateMipmaps(const Texture::DATA& tex, uint32_t layerCount);
    uint32_t getArrayLayerCount(const Texture::DATA& tex);

    std::vector<std::shared_ptr<Texture::DATA>> pTextures_;
    std::vector<std::future<std::shared_ptr<Texture::DATA>>> texFutures_;
};

}  // namespace Texture

#endif  // !TEXTURE_HANDLER_H
