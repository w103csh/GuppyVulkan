
#ifndef SHADER_CONSTANTS_H
#define SHADER_CONSTANTS_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

enum class SHADER {
    LINK = -1,
    // These are index values, so the order here
    // must match order in ALL...
    // DEFAULT
    COLOR_VERT = 0,
    COLOR_FRAG,
    LINE_FRAG,
    TEX_VERT,
    TEX_FRAG,
    CUBE_VERT,
    CUBE_FRAG,
    // PBR
    PBR_COLOR_FRAG,
    PBR_TEX_FRAG,
    // PARALLAX
    PARALLAX_VERT,
    PARALLAX_SIMPLE_FRAG,
    PARALLAX_STEEP_FRAG,
    // Add new to SHADER_ALL and SHADER_LINK_MAP.
};

enum class SHADER_LINK {
    //
    COLOR_FRAG = 0,
    TEX_FRAG,
    UTILITY_VERT,
    UTILITY_FRAG,
    BLINN_PHONG,
    DEFAULT_MATERIAL,
    PBR_FRAG,
    PBR_MATERIAL,
    // Add new to SHADER_LINK_ALL and SHADER_LINK_MAP.
};

namespace Shader {

// directory of shader files relative to the root of the repo
const std::string BASE_DIRNAME = "shaders/";

struct CreateInfo {
    SHADER type;
    std::string name;
    std::string fileName;
    VkShaderStageFlagBits stage;
    std::set<SHADER_LINK> linkTypes;
};

// key:     Universal macro ID that will be used to define the descriptor slot in the shader
// value:   Slot number that will be set for the macro
typedef std::map<std::string, uint8_t> descSetMacroSlotMap;

// Shader type to descriptor set slots
typedef std::pair<SHADER, descSetMacroSlotMap> shaderInfoMapKey;
typedef VkPipelineShaderStageCreateInfo shaderInfoMapValue;
typedef std::pair<const shaderInfoMapKey, shaderInfoMapValue> shaderInfoMapKeyValue;
// Shader handler's data structure for shader types -> descriptor set slots - module handles
typedef std::map<shaderInfoMapKey, shaderInfoMapValue> shaderInfoMap;

namespace Link {

struct CreateInfo {
    SHADER_LINK type;
    std::string fileName;
};

}  // namespace Link

// extern const std::string LIGHT_DEFAULT_POSITIONAL_STRUCT_STRING;
// extern const std::string LIGHT_DEFAULT_SPOT_STRUCT_STRING;

}  // namespace Shader

extern const std::map<SHADER, Shader::CreateInfo> SHADER_ALL;
extern const std::map<SHADER_LINK, Shader::Link::CreateInfo> SHADER_LINK_ALL;
extern const std::map<SHADER, std::set<SHADER_LINK>> SHADER_LINK_MAP;

#endif  // !SHADER_CONSTANTS_H
