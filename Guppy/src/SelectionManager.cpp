
#include "Mesh.h"
#include "SceneHandler.h"
#include "SelectionManager.h"

FaceSelection::FaceSelection(MeshCreateInfo* pCreateInfo) : LineMesh(pCreateInfo) {
    vertices_.resize(Face::NUM_VERTICES * 2, {});
    status_ = STATUS::PENDING_BUFFERS;
}

void SceneHandler::SelectionManager::updateFaceSelection(const VkDevice& dev, std::unique_ptr<Face> pFace) {
    pFace_ = std::move(pFace);
    if (pFace_ != nullptr) {
        getFaceSelection()->getMaterial().setFlags(0);

        auto v0 = (*pFace_)[0].getColorVertex();
        v0.color = {1.0f, 0.0f, 0.0f, 1.0f};  // red
        auto v1 = (*pFace_)[1].getColorVertex();
        v1.color = {0.0f, 1.0f, 0.0f, 1.0f};  // green
        auto v2 = (*pFace_)[2].getColorVertex();
        v2.color = {0.0f, 0.0f, 1.0f, 1.0f};  // blue

        getFaceSelection()->addVertex(v0, 0);
        getFaceSelection()->addVertex(v1, 1);
        getFaceSelection()->addVertex(v1, 2);
        getFaceSelection()->addVertex(v2, 3);
        getFaceSelection()->addVertex(v2, 4);
        getFaceSelection()->addVertex(v0, 5);

        getFaceSelection()->updateBuffers(dev);
    } else {
        getFaceSelection()->getMaterial().setFlags(Material::FLAG::HIDE);
    }
}

std::unique_ptr<LineMesh>& SceneHandler::SelectionManager::getFaceSelection() {
    /*  TODO: this is a crazy pattern. Rethink this at some point.
        the "this" pointer inside the function is a pointer that exists
        on the "inst_" SceneHandler instance. Move SelectionManager
        class to sub-class of SceneHandler or something.

        Also, I am using pFace being nullptr or not to indicate an
        active face selection in the active scene... UGH!
    */
    return getActiveScene()->getLineMesh(faceSelectionOffset_);
}