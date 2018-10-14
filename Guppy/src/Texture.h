
#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"

class Texture {
   public:
    struct TextureData {
        VkImage image;
        VkDeviceMemory memory;
        VkSampler sampler;
        VkDescriptorImageInfo imgDescInfo;
        VkImageView view;
        uint32_t width, height, channels, mipLevels;
        std::string path, name;
    };
    static TextureData createTexture(const VkDevice& dev, const VkCommandBuffer& graph_cmd, const VkCommandBuffer& trans_cmd,
                                     const std::string tex_path, bool generate_mipmaps, BufferResource& stg_res);

   private:
    static void createImage(const VkDevice& dev, const VkCommandBuffer& trans_cmd, const VkCommandBuffer& graph_cmd,
                            const std::string texturePath, bool generate_mipmaps, TextureData& tex, BufferResource& stg_res);
    static void createImageView(const VkDevice& dev, TextureData& tex);
    static void createSampler(const VkDevice& dev, TextureData& tex);
    static void createDescInfo(TextureData& tex);
    static void generateMipmaps(const VkCommandBuffer& cmd, const VkImage& image, const int32_t& width, const int32_t& height,
                                const uint32_t& mip_levels);

};  // class Texture

#endif  // TEXTURE_H