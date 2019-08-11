
#ifndef TEXTURE_CONSTANTS_H
#define TEXTURE_CONSTANTS_H

#include <string>
#include <string_view>
#include <vector>

#include "Types.h"
#include "SamplerConstants.h"

namespace Texture {

struct CreateInfo {
    std::string name;
    std::vector<Sampler::CreateInfo> samplerCreateInfos;
    bool hasData = true;
    bool perFramebuffer = false;
    // For now this is just a flag for COMBINED_SAMPLER v. STORAGE_IMAGE. At some
    // point this might need to be an overridable setting similar to uniform offsets.
    DESCRIPTOR descriptorType = COMBINED_SAMPLER::DONT_CARE;
};

// CREATE INFOS

constexpr std::string_view STATUE_ID = "Statue Texture";
extern const CreateInfo STATUE_CREATE_INFO;

constexpr std::string_view VULKAN_ID = "Vulkan Texture";
extern const CreateInfo VULKAN_CREATE_INFO;

constexpr std::string_view HARDWOOD_ID = "Hardwood Texture";
extern const CreateInfo HARDWOOD_CREATE_INFO;

constexpr std::string_view NEON_BLUE_TUX_GUPPY_ID = "Neon Blue Tux Guppy Texture";
extern const CreateInfo NEON_BLUE_TUX_GUPPY_CREATE_INFO;

constexpr std::string_view MEDIEVAL_HOUSE_ID = "Medieval House Texture";
extern const CreateInfo MEDIEVAL_HOUSE_CREATE_INFO;

constexpr std::string_view WOOD_ID = "Wood Texture";
extern const CreateInfo WOOD_CREATE_INFO;

constexpr std::string_view MYBRICK_ID = "Mybrick Texture";
extern const CreateInfo MYBRICK_CREATE_INFO;

constexpr std::string_view PISA_HDR_ID = "Pisa HDR Texture";
extern const CreateInfo PISA_HDR_CREATE_INFO;

constexpr std::string_view SKYBOX_ID = "Skybox Texture";
extern const CreateInfo SKYBOX_CREATE_INFO;

}  // namespace Texture

#endif  // !TEXTURE_CONSTANTS_H
