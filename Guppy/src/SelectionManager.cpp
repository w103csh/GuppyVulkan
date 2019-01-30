
#include "SelectionManager.h"

#include "Face.h"
#include "FaceMesh.h"
#include "Scene.h"
#include "SceneHandler.h"

// **********************
//      FaceInfo
// **********************

Selection::FaceInfo::FaceInfo(std::unique_ptr<Scene::Base>& pScene, const size_t& offset)
    : offset_(offset), pFace(nullptr), pScene(pScene) {}

// **********************
//      Manager
// **********************

Selection::Manager::Manager(const Scene::Handler& handler) : Handlee(handler), pFaceInfo_(nullptr) {}

void Selection::Manager::addFaceSelection(std::unique_ptr<Scene::Base>& pScene) {
    if (pFaceInfo_ != nullptr) assert(false && "I did not handle this");

    MeshCreateInfo createInfo = {};
    createInfo.pipelineType = PIPELINE::LINE;
    createInfo.isIndexed = false;
    createInfo.materialInfo.flags = Material::FLAG::HIDE;

    std::unique_ptr<LineMesh> pFaceSelection = std::make_unique<FaceMesh>(&createInfo);

    // thread sync
    auto& pMesh = pScene->moveMesh(std::move(pFaceSelection));

    pFaceInfo_ = std::make_unique<FaceInfo>(pScene, pMesh->getOffset());
}

void Selection::Manager::updateFaceSelection(std::unique_ptr<Face> pFace) {
    pFaceInfo_->pFace = std::move(pFace);
    if (pFaceInfo_->pFace != nullptr) {
        assert(false);
        // getFaceSelection()->getMaterial().setFlags(0);

        auto v0 = (*pFaceInfo_->pFace)[0].getColorVertex();
        v0.color = {1.0f, 0.0f, 0.0f, 1.0f};  // red
        auto v1 = (*pFaceInfo_->pFace)[1].getColorVertex();
        v1.color = {0.0f, 1.0f, 0.0f, 1.0f};  // green
        auto v2 = (*pFaceInfo_->pFace)[2].getColorVertex();
        v2.color = {0.0f, 0.0f, 1.0f, 1.0f};  // blue

        auto& pMesh = pFaceInfo_->pScene->getLineMesh(pFaceInfo_->getOffset());

        pMesh->addVertex(v0, 0);
        pMesh->addVertex(v1, 1);
        pMesh->addVertex(v1, 2);
        pMesh->addVertex(v2, 3);
        pMesh->addVertex(v2, 4);
        pMesh->addVertex(v0, 5);

        pMesh->updateBuffers(handler_.shell().context().dev);
    } else {
        assert(false);
        // getFaceSelection()->getMaterial().setFlags(Material::FLAG::HIDE);
    }
}
