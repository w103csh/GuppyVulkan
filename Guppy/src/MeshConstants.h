/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef MESH_CONSTANTS_H
#define MESH_CONSTANTS_H

#include <glm/glm.hpp>
#include <vector>
#include <set>
#include <string>

#include "PipelineConstants.h"

class Face;

namespace Mesh {

using index = uint32_t;
constexpr index BAD_OFFSET = UINT32_MAX;

namespace Geometry {
struct Info {
    bool doubleSided = false;
    bool faceVertexColorsRGB = false;
    bool reverseFaceWinding = false;
    bool smoothNormals = false;
    glm::mat4 transform{1.0f};
};
}  // namespace Geometry

struct Settings {
    bool doVisualHelper = false;
    Geometry::Info geometryInfo = {};
    bool indexVertices = true;
    bool needAdjacenyList = false;
    float visualHelperLineSize = 0.1f;
};

struct CreateInfo {
    PIPELINE pipelineType = GRAPHICS::ALL_ENUM;
    bool selectable = true;
    bool mappable = false;
    Settings settings = {};
    std::set<PASS> passTypes = Uniform::PASS_ALL_SET;
};

struct GenericCreateInfo : public CreateInfo {
    GenericCreateInfo();
    std::vector<Face> faces;
    std::string name;
};

}  // namespace Mesh

#endif  //! MESH_CONSTANTS_H