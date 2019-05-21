
#ifndef SHADER_CONSTANTS_H
#define SHADER_CONSTANTS_H

#include <map>
#include <set>
#include <vector>

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
    BLINN_PHONG_FRAG,
    DEFAULT_MATERIAL,
    PBR_FRAG,
    PBR_MATERIAL,
    // Add new to SHADER_LINK_ALL and SHADER_LINK_MAP.
};

extern const std::vector<SHADER> SHADER_ALL;
extern const std::vector<SHADER_LINK> SHADER_LINK_ALL;
extern const std::map<SHADER, std::set<SHADER_LINK>> SHADER_LINK_MAP;

#endif  // !SHADER_CONSTANTS_H
