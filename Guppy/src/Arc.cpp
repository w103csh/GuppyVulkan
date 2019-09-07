
#include "Arc.h"

// HANDLERS
#include "MeshHandler.h"

Mesh::Arc::Arc(Mesh::Handler& handler, const ArcCreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
               std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Line(handler, "Arc", pCreateInfo, pInstanceData, pMaterial) {
    isIndexed_ = false;

    // Only doing bezier 4 control point that is tessellated on the gpu atm.
    assert(pCreateInfo->controlPoints.size() == 4);
    vertices_.insert(vertices_.end(), pCreateInfo->controlPoints.begin(), pCreateInfo->controlPoints.end());

    status_ = STATUS::PENDING_BUFFERS;
}