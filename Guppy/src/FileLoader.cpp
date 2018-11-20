
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/normal.hpp>
#include <glm/gtx/hash.hpp>

#include <fstream>
#include <sstream>
#include <unordered_map>

#include "FileLoader.h"
#include "Helpers.h"
#include "Mesh.h"
#include "Vertex.h"

std::string FileLoader::readFile(std::string filepath) {
    std::ifstream file(ROOT_PATH + filepath, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    std::stringstream ss;
    ss << file.rdbuf();
    std::string str = ss.str();

    file.close();

    return str;
}

struct TinyObjVertex {
    glm::vec3 pos;
    glm::vec3 norm;
    glm::vec4 color;
    glm::vec2 tex;
    glm::vec3 tan;
    glm::vec3 bitan;
    int smoothingGroupId;
    bool operator==(const TinyObjVertex &other) const { return pos == other.pos && other.smoothingGroupId == smoothingGroupId; }
};

namespace std {
// Hash function for TinyObjVertex
template <>
struct hash<TinyObjVertex> {
    size_t operator()(TinyObjVertex const &vertex) const {
        return (hash<glm::vec3>()(vertex.pos) ^ (hash<int>()(vertex.smoothingGroupId) << 1));
    }
};
}  // namespace std

void FileLoader::loadObj(Mesh *pMesh) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;  // TODO: use this data
    std::string warn, err;                       // TODO: log warings

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, pMesh->getModelPath().c_str(),
                          helpers::getFilePath(pMesh->getModelPath()).c_str())) {
        throw std::runtime_error(err);
    }

    std::vector<TinyObjVertex> vertices;
    std::unordered_map<TinyObjVertex, size_t> uniqueVertices = {};

    bool useColors = (!attrib.colors.empty() && attrib.vertices.size() == attrib.colors.size());
    // Just compute them for now...
    bool useNormals = (!attrib.normals.empty() && attrib.vertices.size() == attrib.normals.size());
    bool useTexCoords = (!attrib.texcoords.empty() && attrib.vertices.size() == attrib.texcoords.size());

    for (const auto &shape : shapes) {
        uniqueVertices.clear();

        for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++) {
            // Get the three indexes of the face (all faces are triangular)
            tinyobj::index_t idx0 = shape.mesh.indices[3 * f + 0];
            tinyobj::index_t idx1 = shape.mesh.indices[3 * f + 1];
            tinyobj::index_t idx2 = shape.mesh.indices[3 * f + 2];

            // Get the three vertex indexes and coordinates
            size_t vi[3];  // indexes
            TinyObjVertex face[3] = {};

            for (size_t k = 0; k < 3; k++) {
                vi[0] = 3 * static_cast<size_t>(idx0.vertex_index) + k;
                vi[1] = 3 * static_cast<size_t>(idx1.vertex_index) + k;
                vi[2] = 3 * static_cast<size_t>(idx2.vertex_index) + k;
                assert(vi[0] >= 0);
                assert(vi[1] >= 0);
                assert(vi[2] >= 0);

                face[0].smoothingGroupId = shape.mesh.smoothing_group_ids[f];
                face[1].smoothingGroupId = shape.mesh.smoothing_group_ids[f];
                face[2].smoothingGroupId = shape.mesh.smoothing_group_ids[f];

                face[0].pos[k] = attrib.vertices[vi[0]];
                face[1].pos[k] = attrib.vertices[vi[1]];
                face[2].pos[k] = attrib.vertices[vi[2]];

                if (useNormals) {
                    face[0].norm[k] = attrib.normals[vi[0]];
                    face[1].norm[k] = attrib.normals[vi[1]];
                    face[2].norm[k] = attrib.normals[vi[2]];
                }

                if (useTexCoords) {
                    face[0].tex[k] = attrib.texcoords[vi[0]];
                    face[1].tex[k] = attrib.texcoords[vi[1]];
                }

                if (useColors) {
                    face[0].color[k] = attrib.colors[vi[0]];
                    face[1].color[k] = attrib.colors[vi[1]];
                }
            }

            // Compute the normal of the face
            glm::vec3 normal;
            normal = glm::triangleNormal(face[0].pos, face[1].pos, face[2].pos);

            for (size_t i = 0; i < 3; ++i) {
                if (uniqueVertices.count(face[i]) == 0) {
                    face[i].norm = normal;
                    uniqueVertices[face[i]] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(std::move(face[i]));
                } else {
                    // If
                    auto offset = uniqueVertices[face[i]];
                    vertices[offset].norm = glm::normalize(vertices[offset].norm + normal);
                }
                pMesh->addIndex(uniqueVertices[face[i]]);
            }
        }

        // TODO: there has to be a faster way to do all of this...
        for (auto &v : vertices) {
            pMesh->addVertex({v.pos, v.norm}, v.color, v.tex, v.tan, v.bitan);
        }
    }
}