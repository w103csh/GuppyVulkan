
#include "SelectionManager.h"

#include "Face.h"
#include "FaceMesh.h"
#include "Instance.h"
#include "Scene.h"
// HANDLERS
#include "MeshHandler.h"
#include "SceneHandler.h"

Selection::Manager::Manager(Scene::Handler& handler, bool makeFaceSelection)
    : Handlee(handler), pFaceInfo_(std::make_unique<FaceInfo>()) {
    if (makeFaceSelection) makeFace();
}

void Selection::Manager::makeFace() {
    if (pFaceInfo_ == nullptr) assert(false && "I did not handle this");

    Mesh::CreateInfo meshInfo = {};
    meshInfo.pipelineType = GRAPHICS::DEFERRED_MRT_LINE;  // GRAPHICS::LINE;
    meshInfo.mappable = true;
    Instance::Obj3d::CreateInfo instInfo = {};
    Material::Default::CreateInfo matInfo = {};
    matInfo.flags = Material::FLAG::HIDE & Material::FLAG::PER_VERTEX_COLOR;
    auto& pFaceMesh = handler().meshHandler().makeLineMesh<Mesh::Face>(&meshInfo, &matInfo, &instInfo);

    pFaceInfo_->offset = pFaceMesh->getOffset();
}

void Selection::Manager::updateFace(std::unique_ptr<Face> pFace) {
    auto& pMesh = handler().meshHandler().getLineMesh(pFaceInfo_->offset);

    pFaceInfo_->pFace = std::move(pFace);
    if (pFaceInfo_->pFace != nullptr) {
        if (pMesh->getMaterial()->getFlags() & Material::FLAG::HIDE) {
            pMesh->getMaterial()->setFlags(pMesh->getMaterial()->getFlags() ^ Material::FLAG::HIDE);
            handler().materialHandler().update(pMesh->getMaterial());
        }

        auto v0 = (*pFaceInfo_->pFace)[0].getVertex<Vertex::Color>();
        v0.color = {1.0f, 0.0f, 0.0f, 1.0f};  // red
        auto v1 = (*pFaceInfo_->pFace)[1].getVertex<Vertex::Color>();
        v1.color = {0.0f, 1.0f, 0.0f, 1.0f};  // green
        auto v2 = (*pFaceInfo_->pFace)[2].getVertex<Vertex::Color>();
        v2.color = {0.0f, 0.0f, 1.0f, 1.0f};  // blue

        pMesh->addVertex(v0, 0);
        pMesh->addVertex(v1, 1);
        pMesh->addVertex(v1, 2);
        pMesh->addVertex(v2, 3);
        pMesh->addVertex(v2, 4);
        pMesh->addVertex(v0, 5);

        pMesh->updateBuffers();
    } else {
        if (pMesh->getMaterial()->getFlags() ^ Material::FLAG::HIDE) {
            pMesh->getMaterial()->setFlags(pMesh->getMaterial()->getFlags() & Material::FLAG::HIDE);
            handler().materialHandler().update(pMesh->getMaterial());
        }
    }
}
