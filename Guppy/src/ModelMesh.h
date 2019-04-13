
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
    ColorMesh(Mesh::Handler &handler, Model::CreateInfo *pCreateInfo, std::shared_ptr<Instance::Base> &pInstanceData,
              std::shared_ptr<Material::Base> &pMaterial);
};
class TextureMesh : public Mesh::Texture {
    friend class Mesh::Handler;

   protected:
    TextureMesh(Mesh::Handler &handler, Model::CreateInfo *pCreateInfo, std::shared_ptr<Instance::Base> &pInstanceData,
                std::shared_ptr<Material::Base> &pMaterial);
};

}  // namespace Model

#endif  // !MODEL_MESH_H