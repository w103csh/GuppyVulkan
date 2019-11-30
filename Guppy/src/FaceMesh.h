/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef FACE_MESH_H
#define FACE_MESH_H

#include "Instance.h"
#include "Mesh.h"

namespace Mesh {
class Handler;
class Face : public Line {
   public:
    Face(Handler &handler, const index &&offset, CreateInfo *pCreateInfo,
         std::shared_ptr<::Instance::Obj3d::Base> &pInstanceData, std::shared_ptr<Material::Base> &pMaterial);
};
}  // namespace Mesh

#endif  //! FACE_MESH_H