#ifndef FILELOADER_H
#define FILELOADER_H

#include <tiny_obj_loader.h>
#include <string>
#include <unordered_map>

#include "Face.h"
#include "Helpers.h"
#include "Vertex.h"

class MyShell;

namespace FileLoader {

std::string readFile(std::string filepath);

typedef struct {
    std::string filename;
    std::string mtl_basedir;
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
} tinyobj_data;

void getObjData(MyShell *sh, tinyobj_data &data);

/* BECUASE TEMPLATES ARE STUPID ALL OF THIS CODE NEEDS TO BE IN THE HEADER */
template <class T>
void loadObjData(const tinyobj_data &data, T &pMeshes) {
    auto pMesh = pMeshes.back();

    // Map of unique vertices for indexed drawing, and calculating smoothing groups.
    std::unordered_map<Vertex::Complete, size_t> uniqueVertices;
    // std::vector<std::pair<Vertex::Complete, size_t>> uniqueVertices1;
    // std::vector<std::pair<Vertex::Complete, size_t>> uniqueVertices2;

    bool useColors = !data.attrib.colors.empty();
    bool useNormals = !data.attrib.normals.empty();
    bool useTexCoords = !data.attrib.texcoords.empty();
    bool hasNormalMap = false;

    Face face;
    size_t faceOffset = 0;
    int materialOffset = -1;
    tinyobj::index_t index0, index1, index2;

    for (const auto &shape : data.shapes) {
        uniqueVertices.clear();

        // Increment by 3 for each face. (A face has 3 vertices. There is a param for forcing
        // this in the LoadObj function)
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
            // TODO: bring back the model class that loads all of these things in the right order...
            if (pMesh->getMaterial().hasTexture() && pMesh->getMaterial().getTexture().flags & Texture::FLAGS::NORMAL) {
                hasNormalMap = true;
            } else if (materialOffset >= 0) {
                // TODO: do this right...
                auto &material = data.materials[materialOffset];
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
                face[0].position[k] = data.attrib.vertices[3 * static_cast<size_t>(index0.vertex_index) + k];
                face[1].position[k] = data.attrib.vertices[3 * static_cast<size_t>(index1.vertex_index) + k];
                face[2].position[k] = data.attrib.vertices[3 * static_cast<size_t>(index2.vertex_index) + k];

                // Color indices share integer offsets with vertex???
                if (useColors) {
                    // color components
                    face[0].color[k] = data.attrib.colors[3 * static_cast<size_t>(index0.vertex_index) + k];
                    face[1].color[k] = data.attrib.colors[3 * static_cast<size_t>(index1.vertex_index) + k];
                    face[2].color[k] = data.attrib.colors[3 * static_cast<size_t>(index2.vertex_index) + k];
                }

                if (useNormals) {
                    // normal components
                    face[0].normal[k] = data.attrib.normals[3 * static_cast<size_t>(index0.normal_index) + k];
                    face[1].normal[k] = data.attrib.normals[3 * static_cast<size_t>(index1.normal_index) + k];
                    face[2].normal[k] = data.attrib.normals[3 * static_cast<size_t>(index2.normal_index) + k];
                }

                // Only x, and y component
                if (useTexCoords && k < 2) {
                    // texture coordinate components
                    face[0].texCoord[k] = data.attrib.texcoords[2 * static_cast<size_t>(index0.texcoord_index) + k];
                    face[1].texCoord[k] = data.attrib.texcoords[2 * static_cast<size_t>(index1.texcoord_index) + k];
                    face[2].texCoord[k] = data.attrib.texcoords[2 * static_cast<size_t>(index2.texcoord_index) + k];
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

            helpers::indexVertices(face, uniqueVertices, true, pMesh);
        }
    }

    pMesh->setStatusPendingBuffers();
}

};  // namespace FileLoader

#endif  // !FILELOADER_H