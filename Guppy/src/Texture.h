
#ifndef TEXTURE_H
#define TEXTURE_H

#include <future>
#include <stb_image.h>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Helpers.h"
#include "LoadingResourceHandler.h"

// TODO: hold the data on the class. This is getting stupid.
class Texture {
   public:
    enum class STATUS { PENDING = 0, READY };
    typedef enum FLAGS {
        DIFFUSE = 0x00000001,
        // THROUGH 0x00000008
        NORMAL = 0x00000010,
        // THROUGH 0x00000080
        SPECULAR = 0x00000100,
        // THROUGH 0x00000800
    } FLAGS;
    struct TextureData {
        TextureData(uint32_t offset, std::string path, std::string normPath = "", std::string specPath = "")
            : offset(offset),
              status(STATUS::PENDING),
              flags(0),
              path(path),
              normPath(normPath),
              specPath(specPath),
              pixels(nullptr),
              normPixels(nullptr),
              specPixels(nullptr) {
            // Just make the name from the path
            name = helpers::getFileName(path);
            // Make sure we have valid flags
            if (!path.empty()) flags |= FLAGS::DIFFUSE;
            if (!normPath.empty()) flags |= FLAGS::NORMAL;
            if (!specPath.empty()) flags |= FLAGS::SPECULAR;
            assert(flags);
        };
        STATUS status;
        Flags flags;
        VkImage image;
        VkDeviceMemory memory;
        VkSampler sampler;
        VkDescriptorImageInfo imgDescInfo;
        VkImageView view;
        uint32_t offset, width, height, channels, mipLevels;
        std::string name, path, normPath, specPath;
        std::unique_ptr<LoadingResources> pLdgRes;
        stbi_uc *pixels, *normPixels, *specPixels;
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
    static uint32_t getArrayLayerCount(const TextureData& tex);

};  // class Texture

#endif  // TEXTURE_H