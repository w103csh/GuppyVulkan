
#include "FaceMesh.h"

#include "Face.h"
// HANDLERS
#include "MeshHandler.h"

using namespace Mesh;

Mesh::Face::Face(Handler& handler, CreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
                 std::shared_ptr<Material::Base>& pMaterial)
    : Line(handler, "FaceMesh", pCreateInfo, pInstanceData, pMaterial) {
    vertices_.resize(::Face::NUM_VERTICES * 2, {});
    status_ = STATUS::PENDING_BUFFERS;
}
