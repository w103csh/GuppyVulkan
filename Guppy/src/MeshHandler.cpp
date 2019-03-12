
#include "MeshHandler.h"

#include "Shell.h"

Mesh::Handler::Handler(Game* pGame) : Game::Handler(pGame) {}

void Mesh::Handler::init() { reset(); }

void Mesh::Handler::reset() {
    const auto& dev = shell().context().dev;
    for (auto& pMesh : colorMeshes_) pMesh->destroy();
    colorMeshes_.clear();
    for (auto& pMesh : lineMeshes_) pMesh->destroy();
    lineMeshes_.clear();
    for (auto& pMesh : texMeshes_) pMesh->destroy();
    texMeshes_.clear();
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
            bool erase = false;

            switch (it->first) {
                case MESH::COLOR: {
                } break;
                case MESH::LINE: {
                } break;
                case MESH::TEXTURE: {
                    auto& pMesh = getTextureMesh(it->second);
                    if (pMesh->getStatus() == STATUS::PENDING_TEXTURE) {
                        pMesh->prepare();
                    }
                    erase = pMesh->getStatus() == STATUS::READY;
                } break;
            }

            if (erase)
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
