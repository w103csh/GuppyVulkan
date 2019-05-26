
#ifndef DESCRIPTOR_CONSTANTS_H
#define DESCRIPTOR_CONSTANTS_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

enum class DESCRIPTOR {
    // CAMERA
    CAMERA_PERSPECTIVE_DEFAULT,
    // LIGHT
    LIGHT_POSITIONAL_DEFAULT,
    LIGHT_SPOT_DEFAULT,
    LIGHT_POSITIONAL_PBR,
    // FOG,
    FOG_DEFAULT,
    // PROJECTOR,
    PROJECTOR_DEFAULT,
    // MATERIAL
    MATERIAL_DEFAULT,
    MATERIAL_PBR,
    // SAMPLER
    SAMPLER_MATERIAL_COMBINED,
    SAMPLER_PIPELINE_COMBINED,
    // Add new to DESCRIPTOR_TYPE_MAP and either DESCRIPTOR_UNIFORM_ALL
    // or DESCRIPTOR_SAMPLER_ALL in code file.
};

enum class DESCRIPTOR_SET {
    // DEFAULT
    UNIFORM_DEFAULT,
    SAMPLER_DEFAULT,
    SAMPLER_CUBE_DEFAULT,
    PROJECTOR_DEFAULT,
    // PBR
    UNIFORM_PBR,
    // PARALLAX
    UNIFORM_PARALLAX,
    SAMPLER_PARALLAX,
    // Add new to DESCRIPTOR_SET_ALL in code file.
};

const std::string DESC_SET_MACRO_ID_PREFIX = "DSMI_";

extern const std::map<DESCRIPTOR, VkDescriptorType> DESCRIPTOR_TYPE_MAP;
extern const std::set<DESCRIPTOR> DESCRIPTOR_UNIFORM_ALL;
extern const std::set<DESCRIPTOR> DESCRIPTOR_MATERIAL_ALL;
extern const std::set<DESCRIPTOR_SET> DESCRIPTOR_SET_ALL;

namespace Descriptor {

struct Reference {
    uint32_t firstSet;
    std::vector<std::vector<VkDescriptorSet>> descriptorSets;
    std::vector<uint32_t> dynamicOffsets;
};

}  // namespace Descriptor

#endif  // !DESCRIPTOR_CONSTANTS_H
