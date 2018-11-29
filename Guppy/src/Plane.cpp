
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
    std::unordered_map<Vertex::Complete, size_t> vertMap = {};
    Face face;

    // bottom left
    face = {};
    face[0] = {
        // geom - bottom left
        {l, b, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {1.0f, 0.0f, 0.0f, 1.0f},  // color (red)
        {0.0f, 1.0f},              // texCoorde (bottom left)
        {},                        // tangent
        {}                         // bitangent
    };
    face[1] = {
        // geom - bottom right
        {r, b, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {0.0f, 0.0f, 1.0f, 1.0f},  // color (blue)
        {1.0f, 1.0f},              // texCoorde (bottom right)
        {},                        // tangent
        {}                         // bitangent
    };
    face[2] = {
        // geom - top left
        {l, t, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {0.0f, 1.0f, 0.0f, 1.0f},  // color (green)
        {0.0f, 0.0f},              // texCoorde (top left)
        {},                        // tangent
        {}                         // bitangent
    };
    helpers::indexVertices(face, vertMap, true, pMesh);

    if (doubleSided) {
        face.reverse();
        face.setSmoothingGroup(1);
        helpers::indexVertices(face, vertMap, true, pMesh);
    }

    // top right
    face = {};
    face[0] = {
        // geom - top left
        {l, t, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {0.0f, 1.0f, 0.0f, 1.0f},  // color (green)
        {0.0f, 0.0f},              // texCoorde (top left)
        {},                        // tangent
        {}                         // bitangent
    };
    face[1] = {
        // geom - bottom right
        {r, b, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {0.0f, 0.0f, 1.0f, 1.0f},  // color (blue)
        {1.0f, 1.0f},              // texCoorde (bottom right)
        {},                        // tangent
        {}                         // bitangent
    };
    face[2] = {
        // geom - top right
        {r, t, 0.0f},              // position
        {},                        // normal
        0,                         // smoothing group id
        {1.0f, 1.0f, 0.0f, 1.0f},  // color (yellow)
        {1.0f, 0.0f},              // texCoorde (top right)
        {},                        // tangent
        {}                         // bitangent
    };
    helpers::indexVertices(face, vertMap, true, pMesh);

    if (doubleSided) {
        face.reverse();
        face.setSmoothingGroup(1);
        helpers::indexVertices(face, vertMap, true, pMesh);
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

ColorPlane::ColorPlane(std::unique_ptr<Material> pMaterial, glm::mat4 model, bool doubleSided)
    : ColorMesh(std::move(pMaterial), model) {
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

TexturePlane::TexturePlane(std::shared_ptr<Texture::Data> pTexture, glm::mat4 model, bool doubleSided)
    : TextureMesh(std::make_unique<Material>(pTexture), model) {
    markerName_ = "TexturePlane";
    createVertices(this, doubleSided);
    updateBoundingBox(vertices_);
    status_ = STATUS::PENDING;
}

TexturePlane::TexturePlane(std::unique_ptr<Material> pMaterial, glm::mat4 model, bool doubleSided)
    : TextureMesh(std::move(pMaterial), model) {
    markerName_ = "TexturePlane";
    createVertices(this, doubleSided);
    updateBoundingBox(vertices_);
    status_ = STATUS::PENDING;
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