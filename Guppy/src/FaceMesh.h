#ifndef FACE_MESH_H
#define FACE_MESH_H

#include "Instance.h"
#include "Mesh.h"

// clang-format off
namespace Mesh { class Handler; }
// clang-format on

class FaceMesh : public Mesh::Line {
   public:
    FaceMesh(Mesh::Handler &handler, Mesh::CreateInfo *pCreateInfo, std::shared_ptr<Instance::Base> &pInstanceData,
             std::shared_ptr<Material::Base> &pMaterial);
};

#endif  //! FACE_MESH_H