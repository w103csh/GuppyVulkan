/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

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
    bool needsData = true;
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

constexpr std::string_view BLUEWATER_ID = "Bluewater Texture";
extern const CreateInfo BLUEWATER_CREATE_INFO;

constexpr std::string_view FIRE_ID = "Fire Texture";
extern const CreateInfo FIRE_CREATE_INFO;

constexpr std::string_view SMOKE_ID = "Smoke Texture";
extern const CreateInfo SMOKE_CREATE_INFO;

constexpr std::string_view STAR_ID = "Star Texture";
extern const CreateInfo STAR_CREATE_INFO;

constexpr std::string_view FABRIC_BROWN_ID = "Fabric Brown Texture";
extern const CreateInfo FABRIC_BROWN_CREATE_INFO;

constexpr std::string_view BRIGHT_MOON_ID = "Bright Moon Texture";
extern const CreateInfo BRIGHT_MOON_CREATE_INFO;

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

constexpr std::string_view SKYBOX_NIGHT_ID = "Skybox Night Texture";

CreateInfo MakeRandom1dTex(const std::string_view& textureId, const std::string_view& samplerId, const uint32_t bufSize);
CreateInfo MakeCubeMapTex(const std::string_view id, const SAMPLER type, const uint32_t size, const uint32_t numMaps = 1);

}  // namespace Texture

#endif  // !TEXTURE_CONSTANTS_H
