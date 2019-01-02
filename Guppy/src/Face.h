
#ifndef FACE_H
#define FACE_H

#include <algorithm>
#include <array>
#include <unordered_map>

#include "Constants.h"
#include "Vertex.h"

class Mesh;

/*  This is used to store unique vertex information when loading a mesh from a file. The key is a
    Vertex::Complete, and the value is a list of mesh offset to mesh vertex index. So, if you are
    creating more than one mesh based on the vertices, you can smooth vertices and attempt to
    index as many of the vertices as possible.
*/
typedef std::unordered_multimap<Vertex::Complete, std::pair<size_t, VB_INDEX_TYPE>,
                                Vertex::Complete::hash_vertex_complete_smoothing,
                                Vertex::Complete::hash_vertex_complete_smoothing>
    unique_vertices_map_smoothing;

typedef std::unordered_multimap<Vertex::Complete, std::pair<size_t, VB_INDEX_TYPE>,
                                Vertex::Complete::hash_vertex_complete_non_smoothing,
                                Vertex::Complete::hash_vertex_complete_non_smoothing>
    unique_vertices_map_non_smoothing;

class Face {
   public:
    static const uint8_t NUM_VERTICES = 3;

    Face() : indices_(), meshOffset_(), vertices_() {}
    Face(Vertex::Complete va, Vertex::Complete vb, Vertex::Complete vc, VB_INDEX_TYPE ia, VB_INDEX_TYPE ib, VB_INDEX_TYPE ic,
         size_t meshOffset)
        : indices_{ia, ib, ic}, meshOffset_(meshOffset), vertices_{va, vb, vc} {}

    Vertex::Complete &operator[](uint8_t index) {
        assert(index >= 0 && index < 3);
        return vertices_[index];
    }

    inline void reverse() { std::reverse(std::begin(vertices_), std::end(vertices_)); }
    inline void setSmoothingGroup(uint32_t id) {
        for (auto &v : vertices_) v.smoothingGroupId = id;
    }

    inline VB_INDEX_TYPE getIndex(uint8_t offset) { return indices_[offset]; }

    void calculateNormal();
    void calculateTangentSpaceVectors();

    template <typename TMap>
    void indexVertices(TMap &vertexMap, Mesh *pMesh, bool calcTangentSpace = true) {
        std::vector<Mesh *> pMeshes = {pMesh};
        indexVertices(vertexMap, pMeshes, 0, calcTangentSpace);
    }

    template <typename TMap, class TVertex>
    void indexVertices(TMap &vertexMap, std::vector<TVertex> &pMeshes, uint8_t meshOffset, bool calcNormals = true) {
        // Calculate per face data.
        if (calcNormals) calculateNormal();
        // Check if there is a normal map...
        bool normalMapped = false;
        for (auto &pMesh : pMeshes)
            if (pMesh->getMaterial().getFlags() & Texture::FLAGS::NORMAL) normalMapped = true;
        if (normalMapped) calculateTangentSpaceVectors();

        for (size_t i = 0; i < 3; ++i) {
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
                        (pMeshes[mOffset]->getVertexType() == Vertex::TYPE::COLOR ||
                         // If the vertex is in going to be in a texture mesh then check tex coords
                         (pMeshes[mOffset]->getVertexType() == Vertex::TYPE::TEXTURE &&
                          vertex.compareTexCoords(vertices_[i])))
                        //
                    ) {
                        index = vIndex;
                    }

                    // Update the vertex for all meshes.
                    pMeshes[mOffset]->addVertex(vertex, vIndex);
                });

                // If the vertex is indexed for other meshes but no the current mesh then add
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

   private:
    std::array<VB_INDEX_TYPE, 3> indices_;
    size_t meshOffset_;
    std::array<Vertex::Complete, 3> vertices_;
};

#endif  // !FACE_H