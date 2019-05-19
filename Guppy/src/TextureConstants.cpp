
#include "TextureConstants.h"

#include "Texture.h"

namespace Texture {

// CREATE INFOS

// STATUE
const CreateInfo STATUE_CREATE_INFO = {
    "Statue Texture",
    {Sampler::STATUE_CREATE_INFO},
};

// VULKAN LOGO
const CreateInfo VULKAN_CREATE_INFO = {
    "Vulkan Texture",
    {Sampler::VULKAN_CREATE_INFO},
};

// HARDWOOD
const CreateInfo HARDWOOD_CREATE_INFO = {
    "Hardwood Texture",
    {Sampler::HARDWOOD_CREATE_INFO},
};

// MEDIEVAL HOUSE
const CreateInfo MEDIEVAL_HOUSE_CREATE_INFO = {
    "Medieval House Texture",
    {Sampler::MEDIEVAL_HOUSE_CREATE_INFO},
};

// WOOD
const CreateInfo WOOD_CREATE_INFO = {
    "Wood Texture",
    {Sampler::WOOD_CREATE_INFO},
};

// MYBRICK
const CreateInfo MYBRICK_CREATE_INFO = {
    "Mybrick Texture",
    {
        Sampler::MYBRICK_COLOR_CREATE_INFO,
        Sampler::MYBRICK_NORMAL_CREATE_INFO,
    },
};

}  // namespace Texture
