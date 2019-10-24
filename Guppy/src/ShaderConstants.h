#ifndef SHADER_CONSTANTS_H
#define SHADER_CONSTANTS_H

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vulkan/vulkan.h>

#include "DescriptorConstants.h"

enum class PIPELINE : uint32_t;
enum class PASS : uint32_t;

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
    // SCREEN SPACE
    SCREEN_SPACE_VERT,
    SCREEN_SPACE_FRAG,
    SCREEN_SPACE_HDR_LOG_FRAG,
    SCREEN_SPACE_BRIGHT_FRAG,
    SCREEN_SPACE_BLUR_FRAG,
    SCREEN_SPACE_COMP,
    // DEFERRED
    DEFERRED_VERT,
    DEFERRED_FRAG,
    DEFERRED_MS_FRAG,
    DEFERRED_MRT_TEX_WS_VERT,
    DEFERRED_MRT_TEX_CS_VERT,
    DEFERRED_MRT_TEX_FRAG,
    DEFERRED_MRT_COLOR_CS_VERT,
    DEFERRED_MRT_COLOR_FRAG,
    DEFERRED_SSAO_FRAG,
    // SHADOW
    SHADOW_COLOR_VERT,
    SHADOW_TEX_VERT,
    SHADOW_FRAG,
    // TESSELLATION
    TESS_COLOR_VERT,
    BEZIER_4_TESC,
    BEZIER_4_TESE,
    TRIANGLE_TESC,
    TRIANGLE_TESE,
    // GEOMETRY
    WIREFRAME_GEOM,
    SILHOUETTE_GEOM,
    // PARTICLE
    WAVE_COLOR_DEFERRED_MRT_VERT,
    FOUNTAIN_PART_DEFERRED_MRT_VERT,
    FOUNTAIN_PART_DEFERRED_MRT_FRAG,
    PARTICLE_EULER_COMP,
    FOUNTAIN_PART_EULER_DEFERRED_MRT_VERT,
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
    GEOMETRY_FRAG,
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

// Pipeline shader stage create info map
using infoMapKey = std::tuple<SHADER, PIPELINE, std::set<PASS>>;
using infoMapValue = std::pair<Descriptor::Set::textReplaceTuples, VkPipelineShaderStageCreateInfo>;
using infoMapKeyValue = std::pair<const infoMapKey, infoMapValue>;
using infoMap = std::multimap<infoMapKey, infoMapValue>;

namespace Link {

struct CreateInfo {
    SHADER_LINK type;
    std::string fileName;
};

}  // namespace Link

// extern const std::string LIGHT_DEFAULT_POSITIONAL_STRUCT_STRING;
// extern const std::string LIGHT_DEFAULT_SPOT_STRUCT_STRING;

extern const std::map<SHADER, Shader::CreateInfo> ALL;
extern const std::map<SHADER_LINK, Shader::Link::CreateInfo> LINK_ALL;
extern const std::map<SHADER, std::set<SHADER_LINK>> LINK_MAP;

/* When this was defined in Tessellation.h clang was throwing initialization
 *  errors, so I moved it here. I tried to make a TessellationConstants.h and it did
 *  not help the problem. The extern const initializers might be revealing that they
 *  are not reliable enough... ugh.
 */
namespace Tessellation {
extern const CreateInfo COLOR_VERT_CREATE_INFO;
extern const CreateInfo BEZIER_4_TESC_CREATE_INFO;
extern const CreateInfo BEZIER_4_TESE_CREATE_INFO;
extern const CreateInfo TRIANGLE_TESC_CREATE_INFO;
extern const CreateInfo TRIANGLE_TESE_CREATE_INFO;
}  // namespace Tessellation

}  // namespace Shader

#endif  // !SHADER_CONSTANTS_H
