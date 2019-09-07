
#include "FaceMesh.h"

#include "Face.h"
// HANDLERS
#include "MeshHandler.h"

Mesh::Face::Face(Mesh::Handler& handler, Mesh::CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
                 std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Line(handler, "FaceMesh", pCreateInfo, pInstanceData, pMaterial) {
    vertices_.resize(::Face::NUM_VERTICES * 2, {});
    status_ = STATUS::PENDING_BUFFERS;
}
