
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
    bool operator==(const TinyObjVertex &other) const {
        return pos == other.pos && tex == other.tex && other.smoothingGroupId == smoothingGroupId;
    }
};

namespace std {
// Hash function for TinyObjVertex
template <>
struct hash<TinyObjVertex> {
    size_t operator()(TinyObjVertex const &vertex) const {
        // TODO: wtf is this doing?
        return (hash<glm::vec3>()(vertex.pos) ^ (hash<int>()(vertex.smoothingGroupId) << 1)) ^ (hash<glm::vec2>()(vertex.tex) << 1);
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

    bool useColors = !attrib.colors.empty();
    bool useNormals = !attrib.normals.empty();
    bool useTexCoords = !attrib.texcoords.empty();
    bool hasNormalMap = false;

    size_t faceOffset = 0;
    int materialOffset = -1;
    tinyobj::index_t idx0, idx1, idx2;

    for (const auto &shape : shapes) {
        uniqueVertices.clear();

        // Increment by 3 for each face.
        for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++) {
            // A face has 3 vertices. There is a param for forcing this in LoadObj.
            TinyObjVertex face[3] = {};

            // Smoothing groups are per face
            auto smoothingGroupId = shape.mesh.smoothing_group_ids[f];
            face[0].smoothingGroupId = smoothingGroupId;
            face[1].smoothingGroupId = smoothingGroupId;
            face[2].smoothingGroupId = smoothingGroupId;

            // Materials are per face
            hasNormalMap = false;
            materialOffset = shape.mesh.material_ids[f];
            if (materialOffset >= 0) {
                // TODO: do this right...
                auto &material = materials[materialOffset];
                hasNormalMap = !material.bump_texname.empty() || !material.normal_texname.empty();
            }

            // The offset of the face into the vertex list
            faceOffset = f * 3;
            idx0 = shape.mesh.indices[faceOffset + 0];
            idx1 = shape.mesh.indices[faceOffset + 1];
            idx2 = shape.mesh.indices[faceOffset + 2];

            // Component x, then y, then z per face
            for (size_t k = 0; k < 3; k++) {
                // vertex components
                face[0].pos[k] = attrib.vertices[3 * static_cast<size_t>(idx0.vertex_index) + k];
                face[1].pos[k] = attrib.vertices[3 * static_cast<size_t>(idx1.vertex_index) + k];
                face[2].pos[k] = attrib.vertices[3 * static_cast<size_t>(idx2.vertex_index) + k];

                // Color indices share integer offsets with vertex???
                if (useColors) {
                    // color components
                    face[0].color[k] = attrib.colors[3 * static_cast<size_t>(idx0.vertex_index) + k];
                    face[1].color[k] = attrib.colors[3 * static_cast<size_t>(idx1.vertex_index) + k];
                    face[2].color[k] = attrib.colors[3 * static_cast<size_t>(idx2.vertex_index) + k];
                }

                if (useNormals) {
                    // normal components
                    face[0].norm[k] = attrib.normals[3 * static_cast<size_t>(idx0.normal_index) + k];
                    face[1].norm[k] = attrib.normals[3 * static_cast<size_t>(idx1.normal_index) + k];
                    face[2].norm[k] = attrib.normals[3 * static_cast<size_t>(idx2.normal_index) + k];
                }

                // Only x, and y component
                if (useTexCoords && k < 2) {
                    // texture coordinate components
                    if (k == 0) {
                        face[0].tex[k] = attrib.texcoords[2 * static_cast<size_t>(idx0.texcoord_index) + k];
                        face[1].tex[k] = attrib.texcoords[2 * static_cast<size_t>(idx1.texcoord_index) + k];
                        face[2].tex[k] = attrib.texcoords[2 * static_cast<size_t>(idx2.texcoord_index) + k];
                    }
                    /*  Vulkan texture coordinates are different from .obj format, so you
                        have to convert the v.
                             (0,1)--------------------------->(1,1)    (0,0)--------------------------->(1,0)
                               ^                                ^        |                                |
                               |                                |        |                                |
                        from   |                                |   ->   |                                |
                               |                                |        |                                |
                               |                                |        |                                |
                               |                                |        V                                V
                             (0,0)--------------------------->(1,0)    (0,1)--------------------------->(1,1)
                    */
                    if (k == 1) {
                        face[0].tex[k] = 1.0f - attrib.texcoords[2 * static_cast<size_t>(idx0.texcoord_index) + k];
                        face[1].tex[k] = 1.0f - attrib.texcoords[2 * static_cast<size_t>(idx1.texcoord_index) + k];
                        face[2].tex[k] = 1.0f - attrib.texcoords[2 * static_cast<size_t>(idx2.texcoord_index) + k];
                    }
                }
            }

            // Compute the normal of the face
            glm::vec3 normal = glm::triangleNormal(face[0].pos, face[1].pos, face[2].pos);

            glm::vec3 tangent = {}, bitangent = {};
            if (hasNormalMap) {
                /*  Create a linear map from tangent space to model space. This is done using
                    Cramer's rule.

                    Solve:
                        deltaPos1 = deltaUV1.x * T + deltaUV1.y * B
                        deltaPos2 = deltaUV2.x * T + deltaUV2.y * B

                        ...where T is the  tangent, and B is the bitangent.

                    Cramer's rule:
                            | deltaPos1 deltaUV1.y  |      | deltaUV1.x deltaPos1  |
                            | deltaPos2 deltaUV2.y  |      | deltaUV2.x deltaPos2  |
                        T=  _________________________  B=  _________________________
                            | deltaUV1.x deltaUV1.y |      | deltaUV1.x deltaUV1.y |
                            | deltaUV2.x deltaUV2.y |      | deltaUV2.x deltaUV2.y |
                */

                // glm::vec3 test = glm::vec3(1.0f, 6.0f, -3.4f) * glm::vec2(2.0f, 4.5f);

                // TODO: make this a function of a face...
                auto deltaPos1 = face[1].pos - face[0].pos;
                auto deltaPos2 = face[2].pos - face[0].pos;
                auto deltaUV1 = face[1].tex - face[0].tex;
                auto deltaUV2 = face[2].tex - face[0].tex;
                auto denom = (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);       // divisor
                tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.x) / denom;    // T
                bitangent = (deltaUV1.x * deltaPos2 - deltaPos2 * deltaUV2.x) / denom;  // B
            }

            for (size_t i = 0; i < 3; ++i) {
                if (uniqueVertices.count(face[i]) == 0) {
                    uniqueVertices[face[i]] = static_cast<uint32_t>(vertices.size());
                    /*  The face vertex is unique base on TinyObjVertex operator= and hash.
                        Currently this should be based on vertex position and face smoothing group.
                    */
                    face[i].norm = normal;
                    face[i].tan = tangent;
                    face[i].bitan = bitangent;
                    vertices.push_back(std::move(face[i]));
                } else {
                    auto offset = uniqueVertices[face[i]];
                    // Averge vertex attributes
                    vertices[offset].norm = glm::normalize(vertices[offset].norm + normal);
                    vertices[offset].tan += tangent;
                    vertices[offset].bitan += bitangent;
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