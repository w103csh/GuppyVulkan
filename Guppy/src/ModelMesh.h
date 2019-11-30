/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef MODEL_MESH_H
#define MODEL_MESH_H

#include <memory>

#include "Instance.h"
#include "Mesh.h"
#include "Model.h"

// clang-format off
namespace Material  { class Base; }
namespace Mesh      { class Handler; }
// clang-format on

namespace Model {

// TODO: Move these
class ColorMesh : public Mesh::Color {
    friend class Mesh::Handler;

   protected:
    ColorMesh(Mesh::Handler &handler, const index &&offset, Model::CreateInfo *pCreateInfo,
              std::shared_ptr<::Instance::Obj3d::Base> &pInstanceData, std::shared_ptr<Material::Base> &pMaterial);
};
class TextureMesh : public Mesh::Texture {
    friend class Mesh::Handler;

   protected:
    TextureMesh(Mesh::Handler &handler, const index &&offset, Model::CreateInfo *pCreateInfo,
                std::shared_ptr<::Instance::Obj3d::Base> &pInstanceData, std::shared_ptr<Material::Base> &pMaterial);
};

}  // namespace Model

#endif  // !MODEL_MESH_H