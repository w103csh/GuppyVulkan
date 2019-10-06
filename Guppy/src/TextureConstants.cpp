
#include "TextureConstants.h"

#include "Texture.h"

namespace Texture {

// STATUE
const CreateInfo STATUE_CREATE_INFO = {
    std::string(STATUE_ID),
    {Sampler::STATUE_CREATE_INFO},
};

// VULKAN LOGO
const CreateInfo VULKAN_CREATE_INFO = {
    std::string(VULKAN_ID),
    {Sampler::VULKAN_CREATE_INFO},
};

// HARDWOOD
const CreateInfo HARDWOOD_CREATE_INFO = {
    std::string(HARDWOOD_ID),
    {Sampler::HARDWOOD_CREATE_INFO},
};

// NEON BLUE TUX GUPPY
const CreateInfo NEON_BLUE_TUX_GUPPY_CREATE_INFO = {
    std::string(NEON_BLUE_TUX_GUPPY_ID),
    {Sampler::NEON_BLUE_TUX_GUPPY_CREATE_INFO},
};

// BLUEWATER
const CreateInfo BLUEWATER_CREATE_INFO = {
    std::string(BLUEWATER_ID),
    {Sampler::BLUEWATER_CREATE_INFO},
};

// FIRE
const CreateInfo FIRE_CREATE_INFO = {
    std::string(FIRE_ID),
    {Sampler::FIRE_CREATE_INFO},
};

// SMOKE
const CreateInfo SMOKE_CREATE_INFO = {
    std::string(SMOKE_ID),
    {Sampler::SMOKE_CREATE_INFO},
};

// STAR
const CreateInfo STAR_CREATE_INFO = {
    std::string(STAR_ID),
    {Sampler::STAR_CREATE_INFO},
};

// MEDIEVAL HOUSE
const CreateInfo MEDIEVAL_HOUSE_CREATE_INFO = {
    std::string(MEDIEVAL_HOUSE_ID),
    {Sampler::MEDIEVAL_HOUSE_CREATE_INFO},
};

// WOOD
const CreateInfo WOOD_CREATE_INFO = {
    std::string(WOOD_ID),
    {Sampler::WOOD_CREATE_INFO},
};

// MYBRICK
const CreateInfo MYBRICK_CREATE_INFO = {
    std::string(MYBRICK_ID),
    {
        Sampler::MYBRICK_COLOR_CREATE_INFO,
        Sampler::MYBRICK_NORMAL_CREATE_INFO,
    },
};

// PISA HDR
const CreateInfo PISA_HDR_CREATE_INFO = {
    std::string(PISA_HDR_ID),
    {Sampler::PISA_HDR_CREATE_INFO},
};

// PISA HDR
const CreateInfo SKYBOX_CREATE_INFO = {
    std::string(SKYBOX_ID),
    {Sampler::SKYBOX_CREATE_INFO},
};

}  // namespace Texture
