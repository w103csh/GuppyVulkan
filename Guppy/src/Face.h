
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
typedef std::unordered_multimap<Vertex::Complete, std::pair<size_t, VB_INDEX_TYPE>> unique_vertices_map;

class Face {
   public:
    Vertex::Complete &operator[](uint8_t index) {
        assert(index >= 0 && index < 3);
        return vertices_[index];
    }

    inline void reverse() { std::reverse(std::begin(vertices_), std::end(vertices_)); }
    inline void setSmoothingGroup(uint32_t id) {
        for (auto &v : vertices_) v.smoothingGroupId = id;
    }

    void calcNormal();
    void calcTangentSpaceVectors();

    void indexVertices(unique_vertices_map &vertexMap, Mesh *pMesh, bool calcTangentSpace = true);

    template <class T>
    void indexVertices(unique_vertices_map &vertexMap, std::vector<T> &pMeshes, size_t meshOffset, bool calcTangentSpace = true) {
        // Calculate per face data...
        calcNormal();
        if (calcTangentSpace) calcTangentSpaceVectors();

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
                    vertex.normal = glm::normalize(vertex.normal + vertices_[i].normal);
                    vertex.tangent += vertices_[i].tangent;
                    vertex.binormal += vertices_[i].binormal;

                    // If the vertex already exists in the current mesh then use the existing index.
                    if (mOffset == meshOffset
                        //
                        && vertex.compareTexCoords(vertices_[i])
                        //
                    ) {
                        vIndex = vIndex;
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
                    vertices_[i].setNormalData(vertex);
                    pMeshes[meshOffset]->addVertex(std::move(vertices_[i]));
                }
            }

            // Always add an index for the current mesh.
            pMeshes[meshOffset]->addIndex(index);
        }
    }

   private:
    std::array<Vertex::Complete, 3> vertices_;
};

#endif  // !FACE_H