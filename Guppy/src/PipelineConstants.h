
#ifndef PIPELINE_CONSTANTS_H
#define PIPELINE_CONSTANTS_H

#include <map>
#include <set>
#include <vector>

enum class VERTEX;

enum class PIPELINE {
    DONT_CARE = -1,
    // These are index values, so the order here
    // must match order in ALL...
    // DEFAULT
    TRI_LIST_COLOR = 0,
    LINE,
    TRI_LIST_TEX,
    CUBE,
    // PBR
    PBR_COLOR,
    PBR_TEX,
    // BLINN PHONG
    BP_TEX_CULL_NONE,
    // PARALLAX
    PARALLAX_SIMPLE,
    PARALLAX_STEEP,
    // Add new to PIPELINE_ALL and VERTEX_PIPELINE_MAP
    // in code file.
};

extern const std::vector<PIPELINE> PIPELINE_ALL;
extern const std::map<VERTEX, std::set<PIPELINE>> VERTEX_PIPELINE_MAP;

#endif  // !PIPELINE_CONSTANTS_H
