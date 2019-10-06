
#ifndef FACE_H
#define FACE_H

#include <algorithm>
#include <array>
#include <unordered_map>

#include "Helpers.h"
#include "Mesh.h"
#include "Vertex.h"

/*  This is used to store unique vertex information when loading a mesh from a file. The key is a
    Vertex::Complete, and the value is a list of mesh offset to mesh vertex index. So, if you are
    creating more than one mesh based on the vertices, you can smooth vertices and attempt to
    index as many of the vertices as possible.
*/
using unique_vertices_map_smoothing = std::unordered_multimap<Vertex::Complete, std::pair<size_t, VB_INDEX_TYPE>,
                                                              Vertex::Complete::hash_vertex_complete_smoothing,
                                                              Vertex::Complete::hash_vertex_complete_smoothing>;

using unique_vertices_map_non_smoothing = std::unordered_multimap<Vertex::Complete, std::pair<size_t, VB_INDEX_TYPE>,
                                                                  Vertex::Complete::hash_vertex_complete_non_smoothing,
                                                                  Vertex::Complete::hash_vertex_complete_non_smoothing>;

class Face {
   public:
    static const uint8_t NUM_VERTICES = 3;

    Face();
    Face(Vertex::Complete va, Vertex::Complete vb, Vertex::Complete vc, VB_INDEX_TYPE ia, VB_INDEX_TYPE ib, VB_INDEX_TYPE ic,
         size_t meshOffset);

    constexpr auto &operator[](const uint8_t index) { return vertices_.at(index); }
    constexpr const auto &operator[](const uint8_t index) const { return vertices_.at(index); }

    inline void reverseWinding() { std::swap(vertices_[1], vertices_[2]); }
    constexpr void setSmoothingGroup(const uint32_t id) {
        for (auto &v : vertices_) v.smoothingGroupId = id;
    }

    constexpr auto getIndex(const uint8_t offset) const { return indices_.at(offset); }

    void calculateNormal();
    void calculateTangentSpaceVectors();

    template <typename TMap>
    void indexVertices(TMap &vertexMap, Mesh::Base *pMesh, bool calcNormal = true) {
        std::vector<Mesh::Base *> pMeshes = {pMesh};
        indexVertices(vertexMap, pMeshes, 0, calcNormal);
    }

    template <typename TMap, class TMesh>
    void indexVertices(TMap &vertexMap, std::vector<TMesh> &pMeshes, uint8_t meshOffset, bool calcNormal = true) {
        // Calculate per face data.
        if (calcNormal) calculateNormal();
        // Check if there is a normal map...

        bool normalMapped =
            std::any_of(pMeshes.begin(), pMeshes.end(), [](const auto &pMesh) { return pMesh->hasNormalMap(); });
        bool needsTangentSpace =
            std::any_of(pMeshes.begin(), pMeshes.end(), [](const auto &pMesh) { return pMesh->TYPE == MESH::TEXTURE; });

        if (needsTangentSpace || normalMapped) calculateTangentSpaceVectors();

        for (size_t i = 0; i < NUM_VERTICES; ++i) {
            long index = -1;

            auto range = vertexMap.equal_range(vertices_[i]);
            if (range.first == range.second) {
                // New vertex
                index = static_cast<VB_INDEX_TYPE>(pMeshes[meshOffset]->getVertexCount());
                vertexMap.insert({{vertices_[i], {meshOffset, index}}});
                pMeshes[meshOffset]->addVertex(std::move(vertices_[i]));
            } else {
                // Non-unique vertex
                Vertex::Complete vertex;

                std::for_each(range.first, range.second, [&](auto &keyValue) {
                    auto &mOffset = keyValue.second.first;  // mesh offset
                    auto &vIndex = keyValue.second.second;  // vertex index

                    // Averge the vertex attributes
                    vertex = pMeshes[mOffset]->getVertexComplete(vIndex);

                    /*  Below was an attempt to fix bad orange smoothing. I decided
                        that the problem was probably coming from the obj's having
                        faces with 4 vertices. tiny_obj can spit out 4 vertex faces
                        so I should try that at some point instead...
                    */
                    // if (glm::dot(vertices_[i].binormal, vertex.binormal) < 0.0f ||
                    //    glm::dot(vertices_[i].tangent, vertex.tangent) < 0.0f) {
                    //    // This is shitty...
                    //    auto r = glm::rotate(glm::mat4(1.0f), M_PI_FLT, vertices_[i].normal);
                    //    vertices_[i].binormal = r * glm::vec4(vertices_[i].binormal, 0.0f);
                    //    vertices_[i].tangent = r * glm::vec4(vertices_[i].tangent, 0.0f);
                    //}

                    vertex.normal += vertices_[i].normal;
                    vertex.tangent += vertices_[i].tangent;
                    vertex.binormal += vertices_[i].binormal;

                    // If the vertex already exists in the current mesh then use the existing index.
                    if (mOffset == meshOffset &&
                        //
                        (pMeshes[mOffset]->VERTEX_TYPE == VERTEX::COLOR ||
                         // If the vertex is in going to be in a texture mesh then check tex coords
                         (pMeshes[mOffset]->VERTEX_TYPE == VERTEX::TEXTURE && vertex.compareTexCoords(vertices_[i])))
                        //
                    ) {
                        index = vIndex;
                    }

                    // Update the vertex for all meshes.
                    pMeshes[mOffset]->addVertex(vertex, vIndex);
                });

                // If the vertex is indexed for other meshes but not the current mesh then add
                // a new value to the vertex map.
                if (index < 0) {
                    index = static_cast<VB_INDEX_TYPE>(pMeshes[meshOffset]->getVertexCount());
                    vertexMap.insert({{vertices_[i], {meshOffset, index}}});

                    // this vertex should retain non-normal data (such as texture coords)
                    vertices_[i].normal = vertex.normal;
                    vertices_[i].tangent = vertex.tangent;
                    vertices_[i].binormal = vertex.binormal;

                    pMeshes[meshOffset]->addVertex(std::move(vertices_[i]));
                }
            }

            // Always add an index for the current mesh.
            pMeshes[meshOffset]->addIndex(index);
        }
    }

    inline void transform(const glm::mat4 &t) {
        for (auto &v : vertices_) v.transform(t);
    }

   private:
    std::array<VB_INDEX_TYPE, NUM_VERTICES> indices_;
    size_t meshOffset_;
    std::array<Vertex::Complete, NUM_VERTICES> vertices_;
};

#endif  // !FACE_H