#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include <memory>

#include "Face.h"
#include "Helpers.h"
#include "SceneHandler.h"

class LineMesh;
struct MeshCreateInfo;

class FaceSelection : public LineMesh {
    friend class SelectionManager;

   public:
    FaceSelection(MeshCreateInfo *pCreateInfo);
};

class SceneHandler::SelectionManager {
    friend class SceneHandler;

   public:
    SelectionManager() : pFace_(nullptr) {}
    inline std::unique_ptr<Face> &getFaceSelectionFace() { return pFace_; }
    void updateFaceSelection(const VkDevice &dev, std::unique_ptr<Face> pFace = nullptr);

   private:
    std::unique_ptr<LineMesh> &getFaceSelection();

    template <typename T>
    void selectFace(const Ray &ray, float &tMin, T &pMeshes, Face &face) {
        for (size_t offset = 0; offset < pMeshes.size(); offset++) {
            const auto &pMesh = pMeshes[offset];
            if (pMesh->isSelectable() && pMesh->getStatus() == STATUS::READY) {
                if (pMesh->testBoundingBox(ray, tMin)) {
                    pMesh->selectFace(ray, tMin, face, offset);
                }
            }
        }
    }

    size_t faceSelectionOffset_;
    std::unique_ptr<Face> pFace_;
};

#endif  // !SELECTION_MANAGER_H