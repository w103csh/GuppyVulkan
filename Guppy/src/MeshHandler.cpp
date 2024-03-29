/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "MeshHandler.h"

#include "Shell.h"
// HANDLERS
#include "SceneHandler.h"

Mesh::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),                                          //
      instObj3dMgr_{"Instance Object 3d Data", (2 * 1000000) + 100}  //
{}

void Mesh::Handler::init() {
    reset();
    // INSTANCE
    instObj3dMgr_.init(shell().context());
}

void Mesh::Handler::tick() {
    // Check loading futures...
    if (!ldgFutures_.empty()) {
        for (auto it = ldgFutures_.begin(); it != ldgFutures_.end();) {
            auto& fut = (*it);

            // Check the status but don't wait...
            auto status = fut.wait_for(std::chrono::seconds(0));

            if (status == std::future_status::ready) {
                auto pMesh = fut.get();
                // Future is ready so prepare the mesh...
                pMesh->prepare();

                // Remove the future from the list if all goes well.
                it = ldgFutures_.erase(it);

            } else {
                ++it;
            }
        }
    }

    // Check loading offsets...
    if (!ldgOffsets_.empty()) {
        for (auto it = ldgOffsets_.begin(); it != ldgOffsets_.end();) {
            auto& mesh = getMesh(it->first, it->second);

            // Check the status
            if (mesh.getStatus() ^ STATUS::READY) {
                mesh.prepare();
            }

            if (mesh.getStatus() == STATUS::READY)
                it = ldgOffsets_.erase(it);
            else
                ++it;
        }
    }
}

bool Mesh::Handler::checkOffset(const MESH type, const Mesh::index offset) {
    switch (type) {
        case MESH::COLOR:
            return offset < colorMeshes_.size();
        case MESH::LINE:
            return offset < lineMeshes_.size();
        case MESH::TEXTURE:
            return offset < texMeshes_.size();
        default:
            assert(false);
            exit(EXIT_FAILURE);
    }
}

void Mesh::Handler::removeMesh(std::unique_ptr<Mesh::Base>& pMesh) {
    // TODO...
    assert(false);
}

void Mesh::Handler::reset() {
    // MESHES
    for (auto& pMesh : colorMeshes_) pMesh->destroy();
    colorMeshes_.clear();
    for (auto& pMesh : lineMeshes_) pMesh->destroy();
    lineMeshes_.clear();
    for (auto& pMesh : texMeshes_) pMesh->destroy();
    texMeshes_.clear();
    // INSTANCE
    instObj3dMgr_.destroy(shell().context());
}

void Mesh::Handler::addOffsetToScene(const MESH type, const Mesh::index offset) {
    sceneHandler().getActiveScene()->addMeshIndex(type, offset);
}
