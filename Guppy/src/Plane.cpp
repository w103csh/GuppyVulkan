/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Plane.h"

// HANDLERS
#include "MeshHandler.h"

using namespace Mesh;

namespace {
void addFaces(Base* pMesh, const Plane::Info planeInfo, const Mesh::Geometry::Info& geoInfo) {
    // Mimic approach in loadObj in FileLoader. This way everything is
    // using the same ideas (for testing)...
    unique_vertices_map_smoothing vertexMap;
    auto faces = Plane::make(planeInfo, geoInfo);
    for (auto& face : faces) {
        if (pMesh->SETTINGS.indexVertices) {
            face.indexVertices(vertexMap, pMesh);
        } else {
            pMesh->addVertex(face);
        }
    }
}
}  // namespace

std::vector<Face> Plane::make(const Plane::Info& planeInfo, const Mesh::Geometry::Info& geoInfo) {
    assert(planeInfo.horzDivs > 0);
    assert(planeInfo.vertDivs > 0);

    // position
    float _l = (planeInfo.width / 2 * -1), _r = _l + planeInfo.width;    // edge values
    float _b = (planeInfo.height / 2 * -1), _t = _b + planeInfo.height;  // edge values
    float l = _l, r;
    float t = _t, b;

    // texture coordinates
    float lu = 0.0f, ru;
    float tv, bv;

    std::vector<Face> faces;

    for (uint32_t horzDiv = 1; horzDiv <= planeInfo.horzDivs; horzDiv++) {
        ru = static_cast<float>(horzDiv) / static_cast<float>(planeInfo.horzDivs);
        r = glm::mix(_l, _r, ru);
        tv = 0.0f;
        t = _t;

        for (uint32_t vertDiv = 1; vertDiv <= planeInfo.vertDivs; vertDiv++) {
            bv = static_cast<float>(vertDiv) / static_cast<float>(planeInfo.vertDivs);
            b = glm::mix(_t, _b, bv);

            // bottom left
            faces.push_back({});
            faces.back()[0] = {
                // geom - bottom left
                {l, b, 0.0f},  // position
                {},            // normal
                0,             // smoothing group id
                COLOR_RED,     // color
                {lu, bv},      // texCoord (bottom left)
                {},            // tangent
                {}             // bitangent
            };
            faces.back()[1] = {
                // geom - bottom right
                {r, b, 0.0f},  // position
                {},            // normal
                0,             // smoothing group id
                COLOR_BLUE,    // color
                {ru, bv},      // texCoord (bottom right)
                {},            // tangent
                {}             // bitangent
            };
            faces.back()[2] = {
                // geom - top left
                {l, t, 0.0f},  // position
                {},            // normal
                0,             // smoothing group id
                COLOR_GREEN,   // color
                {lu, tv},      // texCoord (top left)
                {},            // tangent
                {}             // bitangent
            };

            faces.back().calculateNormal();
            faces.back().calculateTangentSpaceVectors();
            if (geoInfo.reverseFaceWinding) faces.back().reverseWinding();
            faces.back().transform(geoInfo.transform);

            // top right
            faces.push_back({});
            faces.back()[0] = {
                // geom - top left
                {l, t, 0.0f},  // position
                {},            // normal
                0,             // smoothing group id
                COLOR_GREEN,   // color
                {lu, tv},      // texCoord (top left)
                {},            // tangent
                {}             // bitangent
            };
            faces.back()[1] = {
                // geom - bottom right
                {r, b, 0.0f},  // position
                {},            // normal
                0,             // smoothing group id
                COLOR_BLUE,    // color
                {ru, bv},      // texCoord (bottom right)
                {},            // tangent
                {}             // bitangent
            };
            faces.back()[2] = {
                // geom - top right
                {r, t, 0.0f},  // position
                {},            // normal
                0,             // smoothing group id
                COLOR_YELLOW,  // color
                {ru, tv},      // texCoord (top right)
                {},            // tangent
                {}             // bitangent
            };

            faces.back().calculateNormal();
            faces.back().calculateTangentSpaceVectors();
            if (geoInfo.reverseFaceWinding) faces.back().reverseWinding();
            faces.back().transform(geoInfo.transform);

            tv = bv;
            t = b;
        }
        lu = ru;
        l = r;
    }

    // TODO: validate face size is DEFAULT_DIMENSION

    // Add backside faces at end of face list if necessary
    if (geoInfo.doubleSided) {
        std::vector<Face> backSideFaces;
        for (auto& face : faces) {
            backSideFaces.push_back(face);
            backSideFaces.back().reverseWinding();
            backSideFaces.back().setSmoothingGroup(1);
        }
        faces.insert(faces.end(), backSideFaces.begin(), backSideFaces.end());
    }

    return faces;
}

std::vector<Face> Plane::make(const std::array<glm::vec3, 4>& corners, const Mesh::Geometry::Info& geoInfo) {
    std::vector<Face> faces;

    // bottom left
    faces.push_back({});
    faces.back()[0] = {
        // geom - bottom left
        corners[0],  // position
        {},          // normal
        0,           // smoothing group id
        COLOR_RED,   // color
        {},          // texCoord (bottom left)
        {},          // tangent
        {}           // bitangent
    };
    faces.back()[1] = {
        // geom - bottom right
        corners[1],  // position
        {},          // normal
        0,           // smoothing group id
        COLOR_BLUE,  // color
        {},          // texCoord (bottom right)
        {},          // tangent
        {}           // bitangent
    };
    faces.back()[2] = {
        // geom - top left
        corners[2],   // position
        {},           // normal
        0,            // smoothing group id
        COLOR_GREEN,  // color
        {},           // texCoord (top left)
        {},           // tangent
        {}            // bitangent
    };

    faces.back().calculateNormal();
    faces.back().calculateTangentSpaceVectors();
    if (geoInfo.reverseFaceWinding) faces.back().reverseWinding();
    faces.back().transform(geoInfo.transform);

    // top right
    faces.push_back({});
    faces.back()[0] = {
        // geom - top left
        corners[2],   // position
        {},           // normal
        0,            // smoothing group id
        COLOR_GREEN,  // color
        {},           // texCoord (top left)
        {},           // tangent
        {}            // bitangent
    };
    faces.back()[1] = {
        // geom - bottom right
        corners[1],  // position
        {},          // normal
        0,           // smoothing group id
        COLOR_BLUE,  // color
        {},          // texCoord (bottom right)
        {},          // tangent
        {}           // bitangent
    };
    faces.back()[2] = {
        // geom - top right
        corners[3],    // position
        {},            // normal
        0,             // smoothing group id
        COLOR_YELLOW,  // color
        {},            // texCoord (top right)
        {},            // tangent
        {}             // bitangent
    };

    faces.back().calculateNormal();
    faces.back().calculateTangentSpaceVectors();
    if (geoInfo.reverseFaceWinding) faces.back().reverseWinding();
    faces.back().transform(geoInfo.transform);

    // Add backside faces at end of face list if necessary
    if (geoInfo.doubleSided) {
        std::vector<Face> backSideFaces;
        for (auto& face : faces) {
            backSideFaces.push_back(face);
            backSideFaces.back().reverseWinding();
            backSideFaces.back().setSmoothingGroup(1);
        }
        faces.insert(faces.end(), backSideFaces.begin(), backSideFaces.end());
    }

    return faces;
}

Plane::Color::Color(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
                    std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Color(handler, std::forward<const index>(offset), "Color Plane", pCreateInfo, pInstanceData, pMaterial) {
    addFaces(this, pCreateInfo->planeInfo, pCreateInfo->settings.geometryInfo);
    pInstObj3d_->updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}

Plane::Texture::Texture(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
                        std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Texture(handler, std::forward<const index>(offset), "Texture Plane", pCreateInfo, pInstanceData, pMaterial) {
    addFaces(this, pCreateInfo->planeInfo, pCreateInfo->settings.geometryInfo);
    pInstObj3d_->updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}
