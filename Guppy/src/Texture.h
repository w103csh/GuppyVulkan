
#ifndef TEXTURE_H
#define TEXTURE_H

#include <stb_image.h>
#include <string>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Helpers.h"
#include "LoadingHandler.h"

namespace Texture {

typedef enum FLAG {
    NONE = 0x00000000,
    DIFFUSE = 0x00000001,
    // THROUGH 0x00000008
    NORMAL = 0x00000010,
    // THROUGH 0x00000080
    SPECULAR = 0x00000100,
    // THROUGH 0x00000800
} FLAG;

struct DATA {
    DATA(uint32_t offset, std::string path, std::string normPath = "", std::string specPath = "", std::string alphPath = "")
        : status(STATUS::PENDING),
          flags(0),
          image(VK_NULL_HANDLE),
          memory(VK_NULL_HANDLE),
          sampler(VK_NULL_HANDLE),
          imgDescInfo(),
          view(VK_NULL_HANDLE),
          offset(offset),
          width(0),
          height(0),
          channels(0),
          mipLevels(0),
          aspect(4.0f / 3.0f),
          name(helpers::getFileName(path)),
          path(path),
          normPath(normPath),
          specPath(specPath),
          alphPath(alphPath),
          pixels(nullptr),
          normPixels(nullptr),
          specPixels(nullptr),
          alphPixels(nullptr) {
        // Make sure we have valid flags
        if (!path.empty()) flags |= FLAG::DIFFUSE;
        if (!normPath.empty()) flags |= FLAG::NORMAL;
        if (!specPath.empty()) flags |= FLAG::SPECULAR;
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
    std::string name, path, normPath, specPath, alphPath;
    std::unique_ptr<Loading::Resources> pLdgRes;
    stbi_uc *pixels, *normPixels, *specPixels, *alphPixels;
};

}  // namespace Texture

#endif  // TEXTURE_H