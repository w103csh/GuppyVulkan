#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include <memory>

#include "Handlee.h"
#include "Helpers.h"
#include "SceneHandler.h"

class Face;

namespace Selection {

class FaceInfo {
   public:
    FaceInfo(std::unique_ptr<Scene::Base> &pScene, const size_t &offset);

    std::unique_ptr<Scene::Base> &pScene;
    std::unique_ptr<Face> pFace;
    inline size_t getOffset() { return offset_; }

   private:
    size_t offset_;
};

class Manager : public Handlee<Scene::Handler> {
   public:
    Manager(const Scene::Handler &handler);

    void addFaceSelection(std::unique_ptr<Scene::Base> &pScene);
    void updateFaceSelection(std::unique_ptr<Face> pFace = nullptr);

    inline const std::unique_ptr<Face> &getFace() { return pFaceInfo_->pFace; }

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

   private:
    std::unique_ptr<FaceInfo> pFaceInfo_;
};

}  // namespace Selection

#endif  // !SELECTION_MANAGER_H