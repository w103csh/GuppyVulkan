#ifndef VISUAL_HELPER_H
#define VISUAL_HELPER_H

#include <glm/glm.hpp>
#include <vector>

#include "Helpers.h"
#include "Axes.h"

// clang-format off
namespace Mesh { class Handler; }
// clang-format on

namespace VisualHelper {

class ModelSpace : public Axes {
    friend class Mesh::Handler;

   protected:
    ModelSpace(Mesh::Handler& handler, AxesCreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial)
        : Axes(handler, "Model Space Visual Helper", pCreateInfo, pMaterial) {}
};

class TangentSpace : public Mesh::Line {
    friend class Mesh::Handler;

   protected:
    // COLOR
    // TangentSpace(Mesh::Handler& handler, AxesCreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial,
    //             std::unique_ptr<Mesh::Color>& pMesh)
    //    : Mesh::Line(handler, "Tangent Space Visual Helper", pCreateInfo, pMaterial) {
    //    assert(false && "No way to do tangent space for this yet. Need tex coords currently.");
    //    isIndexed_ = false;
    //    make(pMesh.get(), pCreateInfo);
    //    status_ = STATUS::PENDING_BUFFERS;
    //};
    TangentSpace(Mesh::Handler& handler, AxesCreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial,
                 Mesh::Color* pMesh)
        : Mesh::Line(handler, "Tangent Space Visual Helper", pCreateInfo, pMaterial) {
        assert(false && "No way to do tangent space for this yet. Need tex coords currently.");
        isIndexed_ = false;
        make(pMesh, pCreateInfo);
        status_ = STATUS::PENDING_BUFFERS;
    };
    // TEXTURE
    // TangentSpace(Mesh::Handler& handler, AxesCreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial,
    //             std::unique_ptr<Mesh::Texture>& pMesh)
    //    : Mesh::Line(handler, "Tangent Space Visual Helper", pCreateInfo, pMaterial) {
    //    isIndexed_ = false;
    //    make(pMesh.get(), pCreateInfo);
    //    status_ = STATUS::PENDING_BUFFERS;
    //};
    TangentSpace(Mesh::Handler& handler, AxesCreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial,
                 Mesh::Texture* pMesh)
        : Mesh::Line(handler, "Tangent Space Visual Helper", pCreateInfo, pMaterial) {
        isIndexed_ = false;
        make(pMesh, pCreateInfo);
        status_ = STATUS::PENDING_BUFFERS;
    };

   private:
    template <class TMesh>
    void make(TMesh* pMesh, AxesCreateInfo* pCreateInfo) {
        // Determine the line size, which needs to account for the model matrix
        // having a scale.
        glm::vec3 scale = {};
        helpers::decomposeScale(getModel(), scale);
        glm::vec3 lineSize{1.0f};
        lineSize /= scale; // inverse scale of model matrix
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

#endif  // !VISUAL_HELPER_H