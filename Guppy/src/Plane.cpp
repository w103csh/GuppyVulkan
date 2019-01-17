
#include "Face.h"
#include "Mesh.h"
#include "Plane.h"

// **********************
// Plane
// **********************

void Plane::createVertices(Mesh* pMesh, bool doubleSided) {
    float width, height;
    width = height = 1.0f;
    float l = (width / 2 * -1), r = (width / 2);
    float b = (height / 2 * -1), t = (height / 2);

    // Mimic approach in loadObj in FileLoader. This way everything is
    // using the same ideas (for testing)...
    unique_vertices_map_smoothing vertexMap = {};
    Face face;

    // bottom left
    face = {};
    face[0] = {
        // geom - bottom left
        {l, b, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {1.0f, 0.0f, 0.0f, 1.0f},  // color (red)
        {0.0f, 1.0f},              // texCoord (bottom left)
        {},                        // tangent
        {}                         // bitangent
    };
    face[1] = {
        // geom - bottom right
        {r, b, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {0.0f, 0.0f, 1.0f, 1.0f},  // color (blue)
        {1.0f, 1.0f},              // texCoord (bottom right)
        {},                        // tangent
        {}                         // bitangent
    };
    face[2] = {
        // geom - top left
        {l, t, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {0.0f, 1.0f, 0.0f, 1.0f},  // color (green)
        {0.0f, 0.0f},              // texCoord (top left)
        {},                        // tangent
        {}                         // bitangent
    };
    face.indexVertices(vertexMap, pMesh);

    if (doubleSided) {
        face.reverse();
        face.setSmoothingGroup(1);
        face.indexVertices(vertexMap, pMesh);
    }

    // top right
    face = {};
    face[0] = {
        // geom - top left
        {l, t, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {0.0f, 1.0f, 0.0f, 1.0f},  // color (green)
        {0.0f, 0.0f},              // texCoord (top left)
        {},                        // tangent
        {}                         // bitangent
    };
    face[1] = {
        // geom - bottom right
        {r, b, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {0.0f, 0.0f, 1.0f, 1.0f},  // color (blue)
        {1.0f, 1.0f},              // texCoord (bottom right)
        {},                        // tangent
        {}                         // bitangent
    };
    face[2] = {
        // geom - top right
        {r, t, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {1.0f, 1.0f, 0.0f, 1.0f},  // color (yellow)
        {1.0f, 0.0f},              // texCoord (top right)
        {},                        // tangent
        {}                         // bitangent
    };
    face.indexVertices(vertexMap, pMesh);

    if (doubleSided) {
        face.reverse();
        face.setSmoothingGroup(1);
        face.indexVertices(vertexMap, pMesh);
    }
}

// void Plane::createIndices(std::vector<VB_INDEX_TYPE>& indices, bool doubleSided) {
//    int indexSize = doubleSided ? (PLANE_INDEX_SIZE * 2) : PLANE_INDEX_SIZE;
//    indices.resize(indexSize);
//    indices = {0, 1, 2, 2, 1, 3};
//    if (doubleSided) {
//        for (int i = 2; i < PLANE_INDEX_SIZE; i += 3)
//            for (int j = i; j > (i - 3); j--) indices.push_back(indices.at(j));
//    }
//}

// **********************
// ColorPlane
// **********************

ColorPlane::ColorPlane(MeshCreateInfo* pCreateInfo, bool doubleSided) : ColorMesh(pCreateInfo) {
    markerName_ = "ColorPlane";
    createVertices(this, doubleSided);
    updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}

// void ColorPlane::createVertices() {
//    float width, height;
//    width = height = 1.0f;
//    float l = (width / 2 * -1), r = (width / 2);
//    float b = (height / 2 * -1), t = (height / 2);
//    vertices_ = {
//        {
//            // geom - bottom left
//            {l, b, 0.0f},              //
//            {0.0f, 0.0f, 1.0f},        //
//            {1.0f, 0.0f, 0.0f, 1.0f},  // red
//        },                             //
//        {
//            // geom - bottom right
//            {r, b, 0.0f},              //
//            {0.0f, 0.0f, 1.0f},        //
//            {0.0f, 0.0f, 1.0f, 1.0f},  // blue
//        },                             //
//        {
//            // geom - top left
//            {l, t, 0.0f},              //
//            {0.0f, 0.0f, 1.0f},        //
//            {0.0f, 1.0f, 0.0f, 1.0f},  // green
//        },                             //
//        {
//            // geom - top right
//            {r, t, 0.0f},              //
//            {0.0f, 0.0f, 1.0f},        //
//            {1.0f, 1.0f, 0.0f, 1.0f},  // yellow
//        },                             //
//    };
//}

// **********************
// TexturePlane
// **********************

TexturePlane::TexturePlane(MeshCreateInfo* pCreateInfo, bool doubleSided) : TextureMesh(pCreateInfo) {
    markerName_ = "TexturePlane";
    createVertices(this, doubleSided);
    updateBoundingBox(vertices_);
    status_ = STATUS::PENDING_BUFFERS;
}

// void TexturePlane::createVertices() {
//    float width, height;
//    width = height = 1.0f;
//    float l = (width / 2 * -1), r = (width / 2);
//    float b = (height / 2 * -1), t = (height / 2);
//    vertices_ = {
//        {
//            // geom - bottom left
//            {l, b, 0.0f},        //
//            {0.0f, 0.0f, 1.0f},  //
//            {0.0f, 1.0f},        // tex - bottom left
//            {},                  //
//            {}                   //
//        },                       //
//        {
//            // geom - bottom right
//            {r, b, 0.0f},        //
//            {0.0f, 0.0f, 1.0f},  //
//            {1.0f, 1.0f},        // tex - bottom right
//            {},                  //
//            {}                   //
//        },                       //
//        {
//            // geom - top left
//            {l, t, 0.0f},        //
//            {0.0f, 0.0f, 1.0f},  //
//            {0.0f, 0.0f},        // tex - top left
//            {},                  //
//            {}                   //
//        },                       //
//        {
//            // geom - top right
//            {r, t, 0.0f},        //
//            {0.0f, 0.0f, 1.0f},  //
//            {1.0f, 0.0f},        // tex - top right
//            {},                  //
//            {}                   //
//        },                       //
//    };
//}