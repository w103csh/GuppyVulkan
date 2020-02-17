/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Box.h"

#include <Common/Helpers.h>

#include "Plane.h"
// HANDLERS
#include "MeshHandler.h"

using namespace Mesh;

namespace {
void addPlane(const glm::mat4& t, const Mesh::Geometry::Info& geoInfo, std::vector<Face>& faces) {
    auto planeFaces = Plane::make({}, geoInfo);
    for (auto& face : planeFaces) face.transform(t);
    faces.insert(faces.end(), planeFaces.begin(), planeFaces.end());
}
void addFaces(Base* pMesh, const Mesh::Geometry::Info& geoInfo) {
    // Mimic approach in loadObj in FileLoader. This way everything is
    // using the same ideas (for testing)...
    unique_vertices_map_non_smoothing vertexMap = {};
    auto faces = Box::make(geoInfo);
    for (auto& face : faces) {
        if (pMesh->SETTINGS.indexVertices) {
            face.indexVertices(vertexMap, pMesh);
        } else {
            pMesh->addVertex(face);
        }
    }
}
}  // namespace

std::vector<Face> Box::make(const Mesh::Geometry::Info& geoInfo) {
    auto offset = 1.0f / 2.0f;
    std::vector<Face> faces;

    // front
    auto transform = helpers::affine(glm::vec3{1.0}, glm::vec3{0.0f, 0.0f, offset});
    addPlane(transform, geoInfo, faces);
    // back
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{0.0f, 0.0f, -offset}, M_PI_FLT, CARDINAL_Y);
    addPlane(transform, geoInfo, faces);

    // top
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{0.0f, offset, 0.0f}, -M_PI_2_FLT, CARDINAL_X);
    addPlane(transform, geoInfo, faces);
    // bottom
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{0.0f, -offset, 0.0f}, M_PI_2_FLT, CARDINAL_X);
    addPlane(transform, geoInfo, faces);

    // left
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{offset, 0.0f, 0.0f}, M_PI_2_FLT, CARDINAL_Y);
    addPlane(transform, geoInfo, faces);
    // right
    transform = helpers::affine(glm::vec3{1.0}, glm::vec3{-offset, 0.0f, 0.0f}, -M_PI_2_FLT, CARDINAL_Y);
    addPlane(transform, geoInfo, faces);

    return faces;
}

Box::Color::Color(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
                  std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Color(handler, std::forward<const index>(offset), "Color Box", pCreateInfo, pInstanceData, pMaterial) {
    addFaces(this, pCreateInfo->settings.geometryInfo);
    pInstObj3d_->updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}

Box::Texture::Texture(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
                      std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Texture(handler, std::forward<const index>(offset), "Texture Box", pCreateInfo, pInstanceData, pMaterial) {
    addFaces(this, pCreateInfo->settings.geometryInfo);
    pInstObj3d_->updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}
