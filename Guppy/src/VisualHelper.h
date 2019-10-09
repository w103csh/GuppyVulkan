#ifndef VISUAL_HELPER_H
#define VISUAL_HELPER_H

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "Axes.h"
#include "Helpers.h"
#include "Instance.h"

namespace Mesh {
class Handler;
namespace VisualHelper {

class ModelSpace : public Axes {
    friend class Mesh::Handler; // clang

   protected:
    ModelSpace(Handler& handler, const index&& offset, AxesCreateInfo* pCreateInfo,
               std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
        : Axes(handler, std::forward<const index>(offset), "Model Space Visual Helper", pCreateInfo, pInstanceData,
               pMaterial) {}
};

class TangentSpace : public Line {
    friend class Mesh::Handler; // clang

   protected:
    // COLOR
    // TangentSpace(Handler& handler, AxesCreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial,
    //             std::unique_ptr<Color>& pMesh)
    //    : Line(handler, "Tangent Space Visual Helper", pCreateInfo, pMaterial) {
    //    assert(false && "No way to do tangent space for this yet. Need tex coords currently.");
    //    isIndexed_ = false;
    //    make(pMesh.get(), pCreateInfo);
    //    status_ = STATUS::PENDING_BUFFERS;
    //};
    TangentSpace(Handler& handler, const index&& offset, AxesCreateInfo* pCreateInfo,
                 std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial,
                 Color* pMesh)
        : Line(handler, std::forward<const index>(offset), "Tangent Space Visual Helper", pCreateInfo, pInstanceData,
               pMaterial) {
        assert(false && "No way to do tangent space for this yet. Need tex coords currently.");
        make(pMesh, pCreateInfo);
        status_ = STATUS::PENDING_BUFFERS;
    };
    // TEXTURE
    // TangentSpace(Handler& handler, AxesCreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial,
    //             std::unique_ptr<Texture>& pMesh)
    //    : Line(handler, "Tangent Space Visual Helper", pCreateInfo, pMaterial) {
    //    isIndexed_ = false;
    //    make(pMesh.get(), pCreateInfo);
    //    status_ = STATUS::PENDING_BUFFERS;
    //};
    TangentSpace(Handler& handler, const index&& offset, AxesCreateInfo* pCreateInfo,
                 std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial,
                 Texture* pMesh)
        : Line(handler, std::forward<const index>(offset), "Tangent Space Visual Helper", pCreateInfo, pInstanceData,
               pMaterial) {
        make(pMesh, pCreateInfo);
        status_ = STATUS::PENDING_BUFFERS;
    };

   private:
    template <class TMesh>
    void make(TMesh* pMesh, AxesCreateInfo* pCreateInfo) {
        // Determine the line size, which needs to account for the model matrix
        // having a scale.
        glm::vec3 scale = {};
        helpers::decomposeScale(model(), scale);
        glm::vec3 lineSize{1.0f};
        lineSize /= scale;  // inverse scale of model matrix
        lineSize *= pCreateInfo->lineSize;

        for (size_t vIndex = 0; vIndex < pMesh->getVertexCount(); vIndex++) {
            auto v = pMesh->getVertexComplete(vIndex);
            Vertex::Complete vCtr = v;
            Vertex::Complete vNew;
            // normal
            vCtr.color = {0.0f, 0.0f, 1.0f, 1.0f};  // z (blue)
            vNew = vCtr;
            vNew.position += glm::normalize(vNew.normal) * lineSize;
            addVertex(vCtr);
            addVertex(vNew);
            // tangent
            vCtr.color = {1.0f, 0.0f, 0.0f, 1.0f};  // x (red)
            vNew = vCtr;
            vNew.position += glm::normalize(vNew.tangent) * lineSize;
            addVertex(vCtr);
            addVertex(vNew);
            // binormal
            vCtr.color = {0.0f, 1.0f, 0.0f, 1.0f};  // y (green)
            vNew = vCtr;
            vNew.position += glm::normalize(vNew.binormal) * lineSize;
            addVertex(vCtr);
            addVertex(vNew);
        }
    };
};

}  // namespace VisualHelper
}  // namespace Mesh

#endif  // !VISUAL_HELPER_H