
#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "StagingBufferHandler.h"

// TODO: the function sigs here are god awful...

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
    static TextureData createTexture(const VkDevice& dev, const VkPhysicalDeviceMemoryProperties& mem_props,
                                     const std::vector<uint32_t>& queueFamilyIndices, const VkCommandBuffer& graph_cmd,
                                     const VkCommandBuffer& trans_cmd, const std::string tex_path, bool generate_mipmaps,
                                     StagingBufferResource& stg_res);

   private:
    static void createImage(const VkDevice& dev, const VkPhysicalDeviceMemoryProperties& mem_props,
                            const VkCommandBuffer& trans_cmd, const VkCommandBuffer& graph_cmd,
                            const std::vector<uint32_t>& queueFamilyIndices, const std::string texturePath, bool generate_mipmaps,
                            TextureData& tex, StagingBufferResource& stg_res);
    static void createImageView(const VkDevice& dev, TextureData& tex);
    static void createSampler(const VkDevice& dev, TextureData& tex);
    static void createDescInfo(TextureData& tex);
    static void generateMipmaps(const VkCommandBuffer& cmd, const VkImage& image, const int32_t& width, const int32_t& height,
                                const uint32_t& mip_levels);

};  // class Texture

#endif  // TEXTURE_H