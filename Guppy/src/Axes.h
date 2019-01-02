#ifndef AXES_H
#define AXES_H

#include <glm\glm.hpp>

#include "Helpers.h"
#include "Mesh.h"

class ColorMesh;
class TextureMesh;

const float AXES_MAX_SIZE = 2000.0f;

class Axes : public LineMesh {
   public:
    Axes(MeshCreateInfo* pCreateInfo, float lineSize = 1.0f, bool showNegative = false);
};

class VisualHelper : public Axes {
   public:
    // Model space helper
    VisualHelper(MeshCreateInfo* pCreateInfo, float lineSize = 1.0f) : Axes(pCreateInfo, lineSize) {}

    // Tangent space helpers
    VisualHelper(MeshCreateInfo* pCreateInfo, std::unique_ptr<ColorMesh>& pMesh, float lineSize = 1.0f)
        : Axes(pCreateInfo, lineSize) {
        createTangentSpaceVertices(pMesh);
    };
    VisualHelper(MeshCreateInfo* pCreateInfo, std::unique_ptr<TextureMesh>& pMesh, float lineSize = 1.0f)
        : Axes(pCreateInfo, lineSize) {
        createTangentSpaceVertices(pMesh);
    };

   private:
    template <class T>
    void createTangentSpaceVertices(std::unique_ptr<T>& pMesh) {
        for (size_t vIndex = 0; vIndex < pMesh->getVertexCount(); vIndex++) {
            auto v = pMesh->getVertexComplete(vIndex);
            Vertex::Complete vNew;
            // normal
            vNew = v;
            vNew.position += glm::normalize(vNew.normal) * 0.1f;
            vNew.color = {0.0f, 0.0f, 1.0f, 1.0f};  // z (blue)
            addVertex(v);
            addVertex(vNew);
            // tangent
            vNew = v;
            vNew.position += glm::normalize(vNew.tangent) * 0.1f;
            vNew.color = {1.0f, 0.0f, 0.0f, 1.0f};  // x (red)
            addVertex(v);
            addVertex(vNew);
            // binormal
            vNew = v;
            vNew.position += glm::normalize(vNew.binormal) * 0.1f;
            vNew.color = {0.0f, 1.0f, 0.0f, 1.0f};  // y (green)
            addVertex(v);
            addVertex(vNew);
        }
    };
};

#endif  // !AXES_H