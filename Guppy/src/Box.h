/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef BOX_H
#define BOX_H

#include <vector>
#include <memory>

#include "Face.h"
#include "Geometry.h"
#include "Instance.h"
#include "Mesh.h"
// Frustum
#include "Camera.h"

namespace Mesh {
class Handler;

namespace Box {

std::vector<Face> make(const Mesh::Geometry::Info& geoInfo = {});

class Color : public Mesh::Color {
   public:
    Color(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
          std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};

class Texture : public Mesh::Texture {
   public:
    Texture(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
            std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};

}  // namespace Box

namespace Frustum {

std::vector<Face> make(const Mesh::Geometry::Info& geoInfo, const Camera::FrustumInfo& frustumInfo);

struct CreateInfo : Mesh::CreateInfo {
    Camera::FrustumInfo frustumInfo;
};

class Base : public Mesh::Color {
   public:
    Base(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
         std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};

}  // namespace Frustum

}  // namespace Mesh

#endif  // !BOX_H