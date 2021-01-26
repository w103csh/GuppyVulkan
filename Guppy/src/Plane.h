/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef PLANE_H
#define PLANE_H

#include <memory>
#include <vector>

#include "ConstantsAll.h"
#include "Face.h"
#include "Mesh.h"

namespace Mesh {
class Handler;
namespace Plane {

constexpr auto VERTEX_SIZE = 4;
constexpr auto INDEX_SIZE = 6;

struct Info {
    uint32_t horzDivs = 1;
    uint32_t vertDivs = 1;
    float width = 1.0f;
    float height = 1.0f;
};

struct CreateInfo : public Mesh::CreateInfo {
    Plane::Info planeInfo;
};

std::vector<Face> make(const Plane::Info& planeInfo = {}, const Mesh::Geometry::Info& geoInfo = {});

// corners assumed order: bl, br, tl, tr
// Note: uvs are not generated, and colors are fixed.
std::vector<Face> make(const std::array<glm::vec3, 4>& corners, const Mesh::Geometry::Info& geoInfo = {});

class Color : public Mesh::Color {
    friend class Mesh::Handler;  // clang

   protected:
    Color(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
          std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};

class Texture : public Mesh::Texture {
    friend class Mesh::Handler;  // clang

   protected:
    Texture(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
            std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};

}  // namespace Plane
};  // namespace Mesh

#endif  //! PLANE_H