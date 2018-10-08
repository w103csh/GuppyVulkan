
#include "Mesh.h"
#include "Plane.h"

// **********************
// Plane
// **********************

void Plane::createIndices(std::vector<VB_INDEX_TYPE> &indices, bool doubleSided) {
    int indexSize = doubleSided ? (PLANE_INDEX_SIZE * 2) : PLANE_INDEX_SIZE;
    indices.resize(indexSize);
    indices = {0, 1, 2, 2, 1, 3};
    if (doubleSided) {
        for (int i = 2; i < PLANE_INDEX_SIZE; i += 3)
            for (int j = i; j > (i - 3); j--) indices.push_back(indices.at(j));
    }
}

// **********************
// ColorPlane
// **********************

ColorPlane::ColorPlane() : Plane(), ColorMesh() {
    createVertices();
    Plane::createIndices(indices_);
}

// ColorPlane::ColorPlane(float width, float height, bool doubleSided = false, glm::vec3 pos = glm::vec3(),
//                       glm::mat4 rot = glm::mat4(1.0f)) {
//    createVertices(width, height);
//    for (auto& vertex : vertices_) {
//        // TODO: determine when to use 1 or 0 as the homogeneous coord
//        vertex.pos = glm::vec4(vertex.pos, 1.0) * rot;
//        vertex.pos += pos;
//    }
//    createIndices(doubleSided);
//}

void ColorPlane::createVertices(float width, float height) {
    float l = (width / 2 * -1), r = (width / 2);
    float b = (height / 2 * -1), t = (height / 2);
    vertices_ = {
        {
            // geom - bottom left
            {l, b, 0.0f},              //
            {0.0f, 0.0f, 1.0f},        //
            {1.0f, 0.0f, 0.0f, 1.0f},  // red
        },                             //
        {
            // geom - bottom right
            {r, b, 0.0f},              //
            {0.0f, 0.0f, 1.0f},        //
            {0.0f, 0.0f, 1.0f, 1.0f},  // blue
        },                             //
        {
            // geom - top left
            {l, t, 0.0f},              //
            {0.0f, 0.0f, 1.0f},        //
            {0.0f, 1.0f, 0.0f, 1.0f},  // green
        },                             //
        {
            // geom - top right
            {r, t, 0.0f},              //
            {0.0f, 0.0f, 1.0f},        //
            {1.0f, 1.0f, 0.0f, 1.0f},  // yellow
        },                             //
    };
}

// **********************
// TexturePlane
// **********************

TexturePlane::TexturePlane() : TextureMesh("..\\..\\..\\images\\texture.jpg") {
    createVertices();
    Plane::createIndices(indices_);
}

// TexturePlane::TexturePlane(std::string texturePath) : TextureMesh(texturePath, "") {
//    createVertices();
//    createIndices();
//}
//
// TexturePlane::TexturePlane(float width, float height, bool doubleSided = false, glm::vec3 pos = glm::vec3(),
//                           glm::mat4 rot = glm::mat4(1.0f)) {
//    createVertices(width, height);
//    for (auto& vertex : vertices_) {
//        // TODO: determine when to use 1 or 0 as the homogeneous coord
//        vertex.pos = glm::vec4(vertex.pos, 1.0) * rot;
//        vertex.pos += pos;
//    }
//    createIndices(doubleSided);
//}

void TexturePlane::createVertices(float width, float height) {
    float l = (width / 2 * -1), r = (width / 2);
    float b = (height / 2 * -1), t = (height / 2);
    vertices_ = {
        {
            // geom - bottom left
            {l, b, 0.0f},        //
            {0.0f, 0.0f, 1.0f},  //
            {0.0f, 1.0f}         // tex - bottom left
        },                       //
        {
            // geom - bottom right
            {r, b, 0.0f},        //
            {0.0f, 0.0f, 1.0f},  //
            {1.0f, 1.0f}         // tex - bottom right
        },                       //
        {
            // geom - top left
            {l, t, 0.0f},        //
            {0.0f, 0.0f, 1.0f},  //
            {0.0f, 0.0f}         // tex - top left
        },                       //
        {
            // geom - top right
            {r, t, 0.0f},        //
            {0.0f, 0.0f, 1.0f},  //
            {1.0f, 0.0f}         // tex - top right
        },                       //
    };
}