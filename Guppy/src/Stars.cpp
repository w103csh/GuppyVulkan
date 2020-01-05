/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Stars.h"

#include "Random.h"
// HANDLERS
#include "MeshHandler.h"

namespace Mesh {
Stars::Stars(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
             std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Color(handler, std::forward<const index>(offset), "Stars", pCreateInfo, pInstanceData, pMaterial) {
    for (uint32_t i = 0; i < 1000; i++) {
        vertices_.push_back({
            // position
            Random::inst().uniformSphere(),
            {0.0f,  // point size (TODO: figure out how to get variable point sizes in a cube map)
             0.0f, 0.0f},
            // color
            COLOR_WHITE,
        });
    }
    // pInstObj3d_->updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}
}  // namespace Mesh