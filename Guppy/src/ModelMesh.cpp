
#include "ModelMesh.h"

#include "Material.h"
// HANDLERS
#include "MeshHandler.h"

Model::ColorMesh::ColorMesh(Mesh::Handler &handler, const index &&offset, Model::CreateInfo *pCreateInfo,
                            std::shared_ptr<::Instance::Obj3d::Base> &pInstanceData,
                            std::shared_ptr<Material::Base> &pMaterial)
    : Mesh::Color(handler, std::forward<const index>(offset), (std::string)(pCreateInfo->modelPath + "Color Mesh"),
                  pCreateInfo, pInstanceData, pMaterial) {
    status_ = STATUS::PENDING_VERTICES;
}

Model::TextureMesh::TextureMesh(Mesh::Handler &handler, const index &&offset, Model::CreateInfo *pCreateInfo,
                                std::shared_ptr<::Instance::Obj3d::Base> &pInstanceData,
                                std::shared_ptr<Material::Base> &pMaterial)
    : Mesh::Texture(handler, std::forward<const index>(offset), (std::string)(pCreateInfo->modelPath + "Texture Mesh"),
                    pCreateInfo, pInstanceData, pMaterial) {
    status_ = STATUS::PENDING_VERTICES;
}
