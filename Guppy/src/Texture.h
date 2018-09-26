
#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <vulkan/vulkan.h>

#include "util.hpp"

namespace Texture {

void create_texture(struct sample_info& info, std::string tex_path);

void create_image(struct sample_info& info, std::string texturePath, tex_data& tex);

void create_image_view(struct sample_info& info, tex_data& tex);

void create_sampler(struct sample_info& info, tex_data& tex);

void create_desc_info(tex_data& tex);

void generate_mipmaps(struct sample_info& info, const VkImage& image, const VkFormat& format, const int32_t& width,
                      const int32_t& height, const uint32_t& mip_levels);

void submit_queues(struct sample_info& info);

}  // namespace Texture

#endif  // TEX_H