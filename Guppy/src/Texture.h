
#ifndef TEXTURE_H
#define TEXTURE_H

#include <future>
#include <stb_image.h>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "LoadingResourceHandler.h"

class Texture {
   public:
    enum class STATUS { PENDING = 0, READY };
    struct TextureData {
        TextureData(uint32_t offset, std::string path) : offset(offset), status(STATUS::PENDING), path(path){};
        STATUS status;
        VkImage image;
        VkDeviceMemory memory;
        VkSampler sampler;
        VkDescriptorImageInfo imgDescInfo;
        VkImageView view;
        uint32_t offset, width, height, channels, mipLevels;
        std::string path, name;
        std::unique_ptr<LoadingResources> pLdgRes;
        stbi_uc* pixels;
    };

    static std::future<std::shared_ptr<Texture::TextureData>> loadTexture(const VkDevice& dev, const bool makeMipmaps,
                                                                        std::shared_ptr<TextureData> pTex);
    static void createTexture(const VkDevice& dev, const bool makeMipmaps, std::shared_ptr<TextureData> pTex);

   private:
    static void createImage(const VkDevice& dev, TextureData& tex);
    static void createImageView(const VkDevice& dev, TextureData& tex);
    static void createSampler(const VkDevice& dev, TextureData& tex);
    static void createDescInfo(TextureData& tex);
    static void generateMipmaps(const TextureData& tex);

};  // class Texture

#endif  // TEXTURE_H