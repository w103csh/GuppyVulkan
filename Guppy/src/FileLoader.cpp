
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <fstream>
#include <sstream>
#include <unordered_map>

#include "Face.h"
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

void FileLoader::loadObj(Mesh *pMesh) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;  // TODO: use this data
    std::string warn, err;                       // TODO: log warings

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, pMesh->getModelPath().c_str(),
                          helpers::getFilePath(pMesh->getModelPath()).c_str())) {
        throw std::runtime_error(err);
    }

    // Map of unique vertices for indexed drawing, and calculating smoothing groups.
    std::unordered_map<Vertex::Complete, size_t> uniqueVertices = {};

    bool useColors = !attrib.colors.empty();
    bool useNormals = !attrib.normals.empty();
    bool useTexCoords = !attrib.texcoords.empty();
    bool hasNormalMap = false;

    Face face;
    size_t faceOffset = 0;
    int materialOffset = -1;
    tinyobj::index_t index0, index1, index2;

    for (const auto &shape : shapes) {
        uniqueVertices.clear();

        // Increment by 3 for each face.
        for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++) {
            // A face has 3 vertices. There is a param for forcing this in LoadObj.
            face = {};

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
            index0 = shape.mesh.indices[faceOffset + 0];
            index1 = shape.mesh.indices[faceOffset + 1];
            index2 = shape.mesh.indices[faceOffset + 2];

            // Component x, then y, then z per face
            for (size_t k = 0; k < 3; k++) {
                // vertex components
                face[0].position[k] = attrib.vertices[3 * static_cast<size_t>(index0.vertex_index) + k];
                face[1].position[k] = attrib.vertices[3 * static_cast<size_t>(index1.vertex_index) + k];
                face[2].position[k] = attrib.vertices[3 * static_cast<size_t>(index2.vertex_index) + k];

                // Color indices share integer offsets with vertex???
                if (useColors) {
                    // color components
                    face[0].color[k] = attrib.colors[3 * static_cast<size_t>(index0.vertex_index) + k];
                    face[1].color[k] = attrib.colors[3 * static_cast<size_t>(index1.vertex_index) + k];
                    face[2].color[k] = attrib.colors[3 * static_cast<size_t>(index2.vertex_index) + k];
                }

                if (useNormals) {
                    // normal components
                    face[0].normal[k] = attrib.normals[3 * static_cast<size_t>(index0.normal_index) + k];
                    face[1].normal[k] = attrib.normals[3 * static_cast<size_t>(index1.normal_index) + k];
                    face[2].normal[k] = attrib.normals[3 * static_cast<size_t>(index2.normal_index) + k];
                }

                // Only x, and y component
                if (useTexCoords && k < 2) {
                    // texture coordinate components
                    face[0].texCoord[k] = attrib.texcoords[2 * static_cast<size_t>(index0.texcoord_index) + k];
                    face[1].texCoord[k] = attrib.texcoords[2 * static_cast<size_t>(index1.texcoord_index) + k];
                    face[2].texCoord[k] = attrib.texcoords[2 * static_cast<size_t>(index2.texcoord_index) + k];
                    if (k == 1) {
                        /*  Vulkan texture coordinates are different from .obj format, so you
                            have to convert the v.
                                 (0,1)--------------------------->(1,1)    (0,0)--------------------------->(1,0)
                                   ^                                ^        |                                |
                                   |                                |        |                                |
                            from   |                                |   to   |                                |
                                   |                                |        |                                |
                                   |                                |        |                                |
                                   |                                |        V                                V
                                 (0,0)--------------------------->(1,0)    (0,1)--------------------------->(1,1)
                        */
                        face[0].texCoord[k] = 1.0f - face[0].texCoord[k];
                        face[1].texCoord[k] = 1.0f - face[1].texCoord[k];
                        face[2].texCoord[k] = 1.0f - face[2].texCoord[k];
                    }
                }
            }

            face.calcNormal();
            if (hasNormalMap) {
                face.calcImageSpaceData();
            }

            for (size_t i = 0; i < 3; ++i) {
                if (uniqueVertices.count(face[i]) == 0) {
                    uniqueVertices[face[i]] = static_cast<uint32_t>(pMesh->getVertexCount());
                    pMesh->addVertex(std::move(face[i]));
                } else {
                    // Averge vertex attributes
                    auto vertex = pMesh->getVertexComplete(uniqueVertices[face[i]]);
                    vertex.normal = glm::normalize(vertex.normal + face[i].normal);
                    vertex.tangent += face[i].tangent;
                    vertex.bitangent += face[i].bitangent;
                }
                pMesh->addIndex(uniqueVertices[face[i]]);
            }
        }
    }
}