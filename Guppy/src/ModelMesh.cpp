
#include "ModelMesh.h"

#include "Material.h"
// HANDLERS
#include "MeshHandler.h"

Model::ColorMesh::ColorMesh(Mesh::Handler &handler, Model::CreateInfo *pCreateInfo,
                            std::shared_ptr<Instance::Base> &pInstanceData, std::shared_ptr<Material::Base> &pMaterial)
    : Mesh::Color(handler, (std::string)(pCreateInfo->settings.modelPath + "Color Mesh"), pCreateInfo, pInstanceData,
                  pMaterial) {
    status_ = STATUS::PENDING_VERTICES;
}

Model::TextureMesh::TextureMesh(Mesh::Handler &handler, Model::CreateInfo *pCreateInfo,
                                std::shared_ptr<Instance::Base> &pInstanceData, std::shared_ptr<Material::Base> &pMaterial)
    : Mesh::Texture(handler, (std::string)(pCreateInfo->settings.modelPath + "Texture Mesh"), pCreateInfo, pInstanceData,
                    pMaterial) {
    status_ = STATUS::PENDING_VERTICES;
}
