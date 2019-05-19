
#include "SamplerConstants.h"

#include "Constants.h"
#include "Sampler.h"

// IMAGES
const std::string IMG_PATH = DATA_PATH + "images/";
const std::string STATUE_TEX_PATH = IMG_PATH + "texture.jpg";
const std::string VULKAN_TEX_PATH = IMG_PATH + "vulkan.png";
const std::string HARDWOOD_FLOOR_TEX_PATH = IMG_PATH + "hardwood_floor.jpg";
// wood
const std::string WOOD_PATH = IMG_PATH + "Wood_007/";
const std::string WOOD_007_DIFF_TEX_PATH = WOOD_PATH + "Wood_007_COLOR.jpg";
const std::string WOOD_007_NORM_TEX_PATH = WOOD_PATH + "Wood_007_NORM.jpg";
// wood
const std::string MYBRICK_PATH = IMG_PATH + "mybrick/";
const std::string MYBRICK_DIFF_TEX_PATH = MYBRICK_PATH + "mybrick-color.png";
const std::string MYBRICK_NORM_TEX_PATH = MYBRICK_PATH + "mybrick-normal-x2.png";
const std::string MYBRICK_HGHT_TEX_PATH = MYBRICK_PATH + "mybrick-height.png";

/* Formats that I know don't throw errors:
 *
 *		VK_FORMAT_R8_UNORM
 *		VK_FORMAT_R8G8_UNORM
 *		VK_FORMAT_R5G6B5_UNORM_PACK16 (I tested this and I was getting white spots)
 *		VK_FORMAT_R8G8B8A8_UNORM
 *
 *	I found these from here:
 *	https://vulkan.lunarg.com/doc/view/1.0.26.0/linux/vkspec.chunked/ch31s03.html.
 *	Not sure if the formats on that site are linux specific?
 */

namespace Sampler {

// CREATE INFOS

// STATUE
const CreateInfo STATUE_CREATE_INFO = {
    "Statue Color Sampler",
    {{Sampler::TYPE::COLOR, STATUE_TEX_PATH}},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// VULKAN LOGO
const CreateInfo VULKAN_CREATE_INFO = {
    "Vulkan Color Sampler",
    {{Sampler::TYPE::COLOR, VULKAN_TEX_PATH}},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// HARDWOOD
const CreateInfo HARDWOOD_CREATE_INFO = {
    "Hardwood Color Sampler",
    {{Sampler::TYPE::COLOR, HARDWOOD_FLOOR_TEX_PATH}},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// MEDIEVAL HOUSE
const CreateInfo MEDIEVAL_HOUSE_CREATE_INFO = {
    "Medieval Color/Normal/Specular Sampler",
    {
        {Sampler::TYPE::COLOR, MED_H_DIFF_TEX_PATH},
        {Sampler::TYPE::NORMAL, MED_H_NORM_TEX_PATH},
        {Sampler::TYPE::SPECULAR, MED_H_SPEC_TEX_PATH},
    },
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// WOOD
const CreateInfo WOOD_CREATE_INFO = {
    "Wood Color/Normal Sampler",
    {
        {Sampler::TYPE::COLOR, WOOD_007_DIFF_TEX_PATH},
        {Sampler::TYPE::NORMAL, WOOD_007_NORM_TEX_PATH},
    },
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// MYBRICK
const CreateInfo MYBRICK_COLOR_CREATE_INFO = {
    "Mybrick Color Sampler",
    {{Sampler::TYPE::COLOR, MYBRICK_DIFF_TEX_PATH}},
};
const CreateInfo MYBRICK_NORMAL_CREATE_INFO = {
    "Mybrick Normal Sampler",
    {
        {Sampler::TYPE::NORMAL,
         MYBRICK_NORM_TEX_PATH,
         {
             {MYBRICK_HGHT_TEX_PATH, Sampler::CHANNELS::_1, 3},
         }},
    },
};

}  // namespace Sampler
