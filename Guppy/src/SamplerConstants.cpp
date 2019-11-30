/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "SamplerConstants.h"

#include "ConstantsAll.h"
#include "Sampler.h"

// IMAGES
const std::string IMG_PATH = DATA_PATH + "images/";
const std::string STATUE_TEX_PATH = IMG_PATH + "texture.jpg";
const std::string VULKAN_TEX_PATH = IMG_PATH + "vulkan.png";
const std::string HARDWOOD_FLOOR_TEX_PATH = IMG_PATH + "hardwood_floor.jpg";
const std::string NEON_BLUE_TUX_GUPPY_TEX_PATH = IMG_PATH + "neon_blue_tux_guppy.jpg";
const std::string BLUEWATER_TEX_PATH = IMG_PATH + "bluewater.png";
const std::string FIRE_TEX_PATH = IMG_PATH + "fire.png";
const std::string SMOKE_TEX_PATH = IMG_PATH + "smoke.png";
const std::string STAR_TEX_PATH = IMG_PATH + "star.png";
const std::string FABRIC_BROWN_TEX_PATH = IMG_PATH + "fabric_brown.jpg";
const std::string BRIGHT_MOON_TEX_PATH = IMG_PATH + "png-moon-bright-full-moon-png-by-clairesolo-800.png";
// WOOD
const std::string WOOD_PATH = IMG_PATH + "Wood_007/";
const std::string WOOD_007_DIFF_TEX_PATH = WOOD_PATH + "Wood_007_COLOR.jpg";
const std::string WOOD_007_NORM_TEX_PATH = WOOD_PATH + "Wood_007_NORM.jpg";
// MYBRICK
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
    "Statue Color Sampler",                        //
    {{{Sampler::USAGE::COLOR, STATUE_TEX_PATH}}},  //
    VK_IMAGE_VIEW_TYPE_2D,                         //
    BAD_EXTENT_3D,
    {},
    0,
    SAMPLER::CLAMP_TO_BORDER,
};

// VULKAN LOGO
const CreateInfo VULKAN_CREATE_INFO = {
    "Vulkan Color Sampler",
    {{{Sampler::USAGE::COLOR, VULKAN_TEX_PATH}}},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// HARDWOOD
const CreateInfo HARDWOOD_CREATE_INFO = {
    "Hardwood Color Sampler",
    {{{Sampler::USAGE::COLOR, HARDWOOD_FLOOR_TEX_PATH}}},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// NEON BLUE TUX GUPPY
const CreateInfo NEON_BLUE_TUX_GUPPY_CREATE_INFO = {
    "Neon Blue Tux Guppy Color Sampler",
    {{{Sampler::USAGE::COLOR, NEON_BLUE_TUX_GUPPY_TEX_PATH}}},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// BLUEWATER
const CreateInfo BLUEWATER_CREATE_INFO = {
    "Bluewater Color Sampler",
    {{{Sampler::USAGE::COLOR, BLUEWATER_TEX_PATH}}},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// FIRE
const CreateInfo FIRE_CREATE_INFO = {
    "Fire Color Sampler",
    {{{Sampler::USAGE::COLOR, FIRE_TEX_PATH}}},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// SMOKE
const CreateInfo SMOKE_CREATE_INFO = {
    "Smoke Color Sampler",
    {{{Sampler::USAGE::COLOR, SMOKE_TEX_PATH}}},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// STAR
const CreateInfo STAR_CREATE_INFO = {
    "Star Color Sampler",
    {{{Sampler::USAGE::COLOR, STAR_TEX_PATH}}},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// FABRIC BROWN
const CreateInfo FABRIC_BROWN_CREATE_INFO = {
    "Fabric Brown Color Sampler",
    {{{Sampler::USAGE::COLOR, FABRIC_BROWN_TEX_PATH}}},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// BRIGHT MOON
const CreateInfo BRIGHT_MOON_CREATE_INFO = {
    "Bright Moon Color Sampler",
    {{{Sampler::USAGE::COLOR, BRIGHT_MOON_TEX_PATH}}},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// MEDIEVAL HOUSE
const CreateInfo MEDIEVAL_HOUSE_CREATE_INFO = {
    "Medieval Color/Normal/Specular Sampler",
    {{
        {Sampler::USAGE::COLOR, MED_H_DIFF_TEX_PATH},
        {Sampler::USAGE::NORMAL, MED_H_NORM_TEX_PATH},
        {Sampler::USAGE::SPECULAR, MED_H_SPEC_TEX_PATH},
    }},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// WOOD
const CreateInfo WOOD_CREATE_INFO = {
    "Wood Color/Normal Sampler",
    {{
        {Sampler::USAGE::COLOR, WOOD_007_DIFF_TEX_PATH},
        {Sampler::USAGE::NORMAL, WOOD_007_NORM_TEX_PATH},
    }},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
};

// MYBRICK
const CreateInfo MYBRICK_COLOR_CREATE_INFO = {
    "Mybrick Color Sampler",
    {{{Sampler::USAGE::COLOR, MYBRICK_DIFF_TEX_PATH}}},
    VK_IMAGE_VIEW_TYPE_MAX_ENUM,
};
const CreateInfo MYBRICK_NORMAL_CREATE_INFO = {
    "Mybrick Normal Sampler",
    {{
        {
            Sampler::USAGE::NORMAL,
            MYBRICK_NORM_TEX_PATH,
            false,
            false,
            {{MYBRICK_HGHT_TEX_PATH, Sampler::CHANNELS::_1, 3}},
        },
    }},
    VK_IMAGE_VIEW_TYPE_MAX_ENUM,
};

/* It appears the cube map layers are in this order:
 *	+ X
 *	- X
 *	+ Y
 *	- Y
 *	+ Z
 *	- Z
 */

// CUBE MAPS
const std::string CUBE_PATH = IMG_PATH + "cube/";
// PISA
const std::string PISA_HDR_PATH = CUBE_PATH + "pisa_hdr/";
const std::string PISA_HDR_POS_X_TEX_PATH = PISA_HDR_PATH + "pisa_posx.hdr";
const std::string PISA_HDR_NEG_X_TEX_PATH = PISA_HDR_PATH + "pisa_negx.hdr";
const std::string PISA_HDR_POS_Y_TEX_PATH = PISA_HDR_PATH + "pisa_posy.hdr";
const std::string PISA_HDR_NEG_Y_TEX_PATH = PISA_HDR_PATH + "pisa_negy.hdr";
const std::string PISA_HDR_POS_Z_TEX_PATH = PISA_HDR_PATH + "pisa_posz.hdr";
const std::string PISA_HDR_NEG_Z_TEX_PATH = PISA_HDR_PATH + "pisa_negz.hdr";
// SKYBOX
const std::string SKYBOX_PATH = CUBE_PATH + "skybox/";
const std::string SKYBOX_POS_X_TEX_PATH = SKYBOX_PATH + "right.jpg";
const std::string SKYBOX_NEG_X_TEX_PATH = SKYBOX_PATH + "left.jpg";
const std::string SKYBOX_POS_Y_TEX_PATH = SKYBOX_PATH + "top.jpg";
const std::string SKYBOX_NEG_Y_TEX_PATH = SKYBOX_PATH + "bottom.jpg";
const std::string SKYBOX_POS_Z_TEX_PATH = SKYBOX_PATH + "front.jpg";
const std::string SKYBOX_NEG_Z_TEX_PATH = SKYBOX_PATH + "back.jpg";

// PISA HDR
const CreateInfo PISA_HDR_CREATE_INFO = {
    "Pisa HDR Cube Sampler",
    {{
        {Sampler::USAGE::COLOR, PISA_HDR_POS_X_TEX_PATH},
        {Sampler::USAGE::COLOR, PISA_HDR_NEG_X_TEX_PATH},
        {Sampler::USAGE::COLOR, PISA_HDR_POS_Y_TEX_PATH},
        {Sampler::USAGE::COLOR, PISA_HDR_NEG_Y_TEX_PATH},
        {Sampler::USAGE::COLOR, PISA_HDR_POS_Z_TEX_PATH},
        {Sampler::USAGE::COLOR, PISA_HDR_NEG_Z_TEX_PATH},
    }},
    VK_IMAGE_VIEW_TYPE_CUBE,
    BAD_EXTENT_3D,
    {},
    VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
    SAMPLER::CUBE,
    VK_IMAGE_USAGE_SAMPLED_BIT,
    {{false, false}, 1},
};

// SKYBOX
const CreateInfo SKYBOX_CREATE_INFO = {
    "Skybox Cube Sampler",
    {{
        {Sampler::USAGE::COLOR, SKYBOX_POS_X_TEX_PATH},
        {Sampler::USAGE::COLOR, SKYBOX_NEG_X_TEX_PATH},
        {Sampler::USAGE::COLOR, SKYBOX_POS_Y_TEX_PATH},
        {Sampler::USAGE::COLOR, SKYBOX_NEG_Y_TEX_PATH},
        {Sampler::USAGE::COLOR, SKYBOX_POS_Z_TEX_PATH},
        {Sampler::USAGE::COLOR, SKYBOX_NEG_Z_TEX_PATH},
    }},
    VK_IMAGE_VIEW_TYPE_CUBE,
    BAD_EXTENT_3D,
    {},
    VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
    SAMPLER::CUBE,
    VK_IMAGE_USAGE_SAMPLED_BIT,
    {{false, false}, 1},
};

uint32_t GetMipLevels(const VkExtent3D& extent) {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
}

}  // namespace Sampler
