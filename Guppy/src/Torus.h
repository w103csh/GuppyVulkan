#ifndef TORUS_H
#define TORUS_H

#include <glm/glm.hpp>
#include <memory>
#include <vector>

#include "Constants.h"
#include "Face.h"
#include "Instance.h"
#include "Mesh.h"

namespace Mesh {
class Handler;
namespace Torus {

struct Info {
    float outerRadius = 1.0f;
    float innerRadius = 3.0f / 7.0f;
    uint32_t numSides = 20;
    uint32_t numRings = 20;
};

struct CreateInfo : public Mesh::CreateInfo {
    Torus::Info torusInfo;
};

template <typename TVertex, typename TIndex>
void make(const Torus::Info& torusInfo, std::vector<TVertex>& vertices, std::vector<TIndex>& indices) {
    // This code was appropriated from the cookbook

    // Generate the vertex data
    float ringFactor = glm::two_pi<float>() / torusInfo.numRings;
    float sideFactor = glm::two_pi<float>() / torusInfo.numSides;
    int idx = 0;
    Face face;
    for (uint32_t ring = 0; ring <= torusInfo.numRings; ring++) {
        float u = ring * ringFactor;
        float cu = cos(u);
        float su = sin(u);
        for (uint32_t side = 0; side < torusInfo.numSides; side++) {
            float v = side * sideFactor;
            float cv = cos(v);
            float sv = sin(v);
            float r = (torusInfo.outerRadius + torusInfo.innerRadius * cv);

            face[idx % 3] = {
                {
                    // position
                    r * cu,
                    r * su,
                    torusInfo.innerRadius * sv,
                },
                {
                    // normal
                    cv * cu * r,
                    cv * su * r,
                    sv * r,
                },
                0,            // smoothing group id
                COLOR_WHITE,  // color
                {
                    // texCoord
                    u / glm::two_pi<float>(),
                    v / glm::two_pi<float>(),
                },
                {},  // tangent
                {}   // bitangent
            };
            face[idx % 3].normal = glm::normalize(face[idx % 3].normal);
            idx++;
            if (idx % 3 == 0) {
                if (idx > 0) {
                    for (auto i = 0; i < Face::NUM_VERTICES; i++) {
                        vertices.push_back(face[i].getVertex<TVertex>());
                    }
                }
                face = {};
            }
        }
    }

    for (uint32_t ring = 0; ring < torusInfo.numRings; ring++) {
        uint32_t ringStart = ring * torusInfo.numSides;
        uint32_t nextRingStart = (ring + 1) * torusInfo.numSides;
        for (uint32_t side = 0; side < torusInfo.numSides; side++) {
            int nextSide = (side + 1) % torusInfo.numSides;
            // The quad
            indices.push_back(ringStart + side);
            indices.push_back(nextRingStart + side);
            indices.push_back(nextRingStart + nextSide);
            indices.push_back(ringStart + side);
            indices.push_back(nextRingStart + nextSide);
            indices.push_back(ringStart + nextSide);
        }
    }
}

class Color : public Mesh::Color {
   public:
    Color(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
          std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};

class Texture : public Mesh::Texture {
   public:
    Texture(Handler& handler, const index&& offset, CreateInfo* pCreateInfo,
            std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial);
};

}  // namespace Torus
}  // namespace Mesh

#endif  // !TORUS_H