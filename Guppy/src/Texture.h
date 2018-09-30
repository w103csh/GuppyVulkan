
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
    };
    static TextureData createTexture(const MyShell::Context& ctx, StagingBufferResource& stg_res, VkCommandBuffer& trans_cmd,
                                     VkCommandBuffer& graph_cmd, const std::vector<uint32_t>& queueFamilyIndices,
                                     std::string tex_path);

   private:
    static void createImage(const VkDevice& dev, const VkFormat& format, const VkPhysicalDevice& physical_dev,
                            const VkPhysicalDeviceMemoryProperties& mem_props, StagingBufferResource& stg_res,
                            VkCommandBuffer& trans_cmd, VkCommandBuffer& graph_cmd, const std::vector<uint32_t>& queueFamilyIndices,
                            std::string texturePath, TextureData& tex);
    static void createImageView(const VkDevice& dev, TextureData& tex);
    static void createSampler(const VkDevice& dev, TextureData& tex);
    static void createDescInfo(TextureData& tex);
    static void generateMipmaps(VkCommandBuffer& cmd, const VkPhysicalDevice& physical_dev, const VkImage& image,
                                const VkFormat& format, const int32_t& width, const int32_t& height, const uint32_t& mip_levels);

};  // class Texture

#endif  // TEXTURE_H