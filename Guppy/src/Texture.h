
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
    typedef enum FLAGS {
        DIFFUSE = 0x00000001,
        // THROUGH 0x00000008
        NORMAL = 0x00000010,
        // THROUGH 0x00000080
        SPECULAR = 0x00000100,
        // THROUGH 0x00000800
    } FLAGS;
    struct Data {
        Data(uint32_t offset, std::string path, std::string normPath = "", std::string specPath = "")
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
        FlagBits flags;
        VkImage image;
        VkDeviceMemory memory;
        VkSampler sampler;
        VkDescriptorImageInfo imgDescInfo;
        VkImageView view;
        uint32_t offset, width, height, channels, mipLevels;
        float aspect;
        std::string name, path, normPath, specPath;
        std::unique_ptr<LoadingResources> pLdgRes;
        stbi_uc *pixels, *normPixels, *specPixels;
    };

    static std::future<std::shared_ptr<Texture::Data>> loadTexture(const VkDevice& dev, const bool makeMipmaps,
                                                                          std::shared_ptr<Data> pTexture);
    static void createTexture(const VkDevice& dev, const bool makeMipmaps, std::shared_ptr<Data> pTexture);

   private:
    static void createImage(const VkDevice& dev, Data& tex);
    static void createImageView(const VkDevice& dev, Data& tex);
    static void createSampler(const VkDevice& dev, Data& tex);
    static void createDescInfo(Data& tex);
    static void generateMipmaps(const Data& tex);
    static uint32_t getArrayLayerCount(const Data& tex);

};  // class Texture

#endif  // TEXTURE_H