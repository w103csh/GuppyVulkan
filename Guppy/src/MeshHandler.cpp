
#include "MeshHandler.h"

#include "Shell.h"

Mesh::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),                                      //
      instDefMgr_{"Default Instance Data", (2 * 1000000) + 100}  //
{}

void Mesh::Handler::init() {
    reset();
    // INSTANCE
    instDefMgr_.init(shell().context(), settings());
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
    instDefMgr_.destroy(shell().context().dev);
}

void Mesh::Handler::update() {
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
            switch (mesh.getStatus()) {
                case STATUS::PENDING_TEXTURE:
                    mesh.prepare();
                    break;
                default:
                    assert(false);
                    throw std::runtime_error("Unhandled status");
            }

            if (mesh.getStatus() == STATUS::READY)
                it = ldgOffsets_.erase(it);
            else
                ++it;
        }
    }
}

void Mesh::Handler::removeMesh(std::unique_ptr<Mesh::Base>& pMesh) {
    // TODO...
    assert(false);
}
