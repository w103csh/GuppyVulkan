/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Arc.h"

// HANDLERS
#include "MeshHandler.h"

using namespace Mesh;

Arc::Arc(Handler& handler, const index&& offset, const CreateInfo* pCreateInfo,
         std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
    : Line(handler, std::forward<const index>(offset), "Arc", pCreateInfo, pInstanceData, pMaterial) {
    // Only doing bezier 4 control point that is tessellated on the gpu atm.
    assert(pCreateInfo->controlPoints.size() == 4);
    vertices_.insert(vertices_.end(), pCreateInfo->controlPoints.begin(), pCreateInfo->controlPoints.end());

    status_ = STATUS::PENDING_BUFFERS;
}