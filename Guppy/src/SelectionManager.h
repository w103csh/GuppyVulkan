#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include <memory>

#include "Handlee.h"
#include "Helpers.h"
#include "Mesh.h"

// clang-format off
class Face;
namespace Scene { class Handler; }
// clang-format on

namespace Selection {

class FaceInfo {
   public:
    std::unique_ptr<Face> pFace = nullptr;
    Mesh::INDEX offset = Mesh::BAD_OFFSET;
};

class Manager : public Handlee<Scene::Handler> {
   public:
    Manager(Scene::Handler &handler, bool makeFaceSelection);

    void makeFace();
    void updateFace(std::unique_ptr<Face> pFace = nullptr);

    inline bool isEnabled() { return pFaceInfo_->offset != Mesh::BAD_OFFSET; }
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