
#ifndef TEXTURE_CONSTANTS_H
#define TEXTURE_CONSTANTS_H

#include <string>
#include <vector>

#include "SamplerConstants.h"

namespace Texture {

struct CreateInfo {
    std::string name;
    std::vector<Sampler::CreateInfo> samplerCreateInfos;
    bool hasData = true;
};

// CREATE INFOS

extern const CreateInfo STATUE_CREATE_INFO;
extern const CreateInfo VULKAN_CREATE_INFO;
extern const CreateInfo HARDWOOD_CREATE_INFO;
extern const CreateInfo MEDIEVAL_HOUSE_CREATE_INFO;
extern const CreateInfo WOOD_CREATE_INFO;
extern const CreateInfo MYBRICK_CREATE_INFO;
extern const CreateInfo PISA_HDR_CREATE_INFO;
extern const CreateInfo SKYBOX_CREATE_INFO;

}  // namespace Texture

#endif  // !TEXTURE_CONSTANTS_H
