#ifndef TEXTURE_HANDLER_H
#define TEXTURE_HANDLER_H

#include <unordered_map>
#include <vector>

#include "Singleton.h"
#include "Texture.h"

class Shell;

class TextureHandler : public Singleton {
   public:
    static void init(Shell* pShell, const Game::Settings& settings);
    static inline void destroy() { inst_.reset(); }

    static void addTexture(std::string path, std::string normPath = "", std::string specPath = "");
    static std::shared_ptr<Texture::Data> getTextureByPath(std::string path);
    static void update();

    static const std::shared_ptr<Texture::Data> getTexture(uint32_t index) {
        if (index < inst_.pTextures_.size()) return inst_.pTextures_[index];
        return nullptr;
    }
    static uint32_t getCount() { return static_cast<uint32_t>(inst_.pTextures_.size()); }

    // DESCRIPTOR
    static VkDescriptorSetLayoutBinding getDescriptorLayoutBinding(const DESCRIPTOR_TYPE& type);
    static VkWriteDescriptorSet getDescriptorWrite(const DESCRIPTOR_TYPE& type);
    static VkCopyDescriptorSet getDescriptorCopy(const DESCRIPTOR_TYPE& type);

   private:
    TextureHandler() : sh_(nullptr){};  // Prevent construction
    ~TextureHandler(){};                // Prevent construction
    static TextureHandler inst_;
    void reset() override;

    Shell* sh_;  // TODO: shared_ptr
    Shell::Context ctx_;
    Game::Settings settings_;

    std::future<std::shared_ptr<Texture::Data>> loadTexture(const VkDevice& dev, const bool makeMipmaps,
                                                            std::shared_ptr<Texture::Data>& pTexture);

    std::vector<std::shared_ptr<Texture::Data>> pTextures_;
    std::vector<std::future<std::shared_ptr<Texture::Data>>> texFutures_;
};

#endif  // !TEXTURE_HANDLER_H
