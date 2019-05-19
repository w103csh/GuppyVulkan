
#ifndef PIPELINE_CONSTANTS_H
#define PIPELINE_CONSTANTS_H

#include <map>
#include <set>
#include <vector>

enum class VERTEX;

enum class PIPELINE {
    DONT_CARE = -1,
    // These are index values...
    TRI_LIST_COLOR = 0,
    LINE = 1,
    TRI_LIST_TEX = 2,
    PBR_COLOR = 3,
    PBR_TEX = 4,
    BP_TEX_CULL_NONE = 5,
    PARALLAX_SIMPLE = 6,
    PARALLAX_STEEP = 7,
    // Add new to PIPELINE_ALL and VERTEX_PIPELINE_MAP in code file.
};

extern const std::vector<PIPELINE> PIPELINE_ALL;
extern const std::map<VERTEX, std::set<PIPELINE>> VERTEX_PIPELINE_MAP;

#endif  // !PIPELINE_CONSTANTS_H
