/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Torus.h"

// HANDLERS
#include "MeshHandler.h"

using namespace Mesh;

Torus::Color::Color(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
                    std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Color(handler, std::forward<const index>(offset), "Color Torus", pCreateInfo, pInstanceData, pMaterial) {
    make<Vertex::Color>(pCreateInfo->torusInfo, vertices_, indices_);
    pInstObj3d_->updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}

Torus::Texture::Texture(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
                        std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Texture(handler, std::forward<const index>(offset), "Texture Torus", pCreateInfo, pInstanceData, pMaterial) {
    make(pCreateInfo->torusInfo, vertices_, indices_);
    pInstObj3d_->updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}
