#ifndef FACE_MESH_H
#define FACE_MESH_H

#include "Instance.h"
#include "Mesh.h"

namespace Mesh {
class Handler;
class Face : public Mesh::Line {
   public:
    Face(Mesh::Handler &handler, Mesh::CreateInfo *pCreateInfo, std::shared_ptr<Instance::Base> &pInstanceData,
         std::shared_ptr<Material::Base> &pMaterial);
};
}  // namespace Mesh

#endif  //! FACE_MESH_H