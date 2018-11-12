
#include "Mesh.h"
#include "Plane.h"

// **********************
// Plane
// **********************

void Plane::createIndices(std::vector<VB_INDEX_TYPE>& indices, bool doubleSided) {
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

ColorPlane::ColorPlane(std::unique_ptr<Material> pMaterial, glm::mat4 model, bool doubleSided)
    : ColorMesh(std::move(pMaterial), "", model) {
    markerName_ = "ColorPlane";
    createVertices();
    updateBoundingBox(vertices_);
    Plane::createIndices(indices_, doubleSided);
}

void ColorPlane::createVertices() {
    float width, height;
    width = height = 1.0f;
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

TexturePlane::TexturePlane(std::shared_ptr<Texture::Data> pTexture, glm::mat4 model, bool doubleSided)
    : TextureMesh(std::make_unique<Material>(pTexture), "", model) {
    markerName_ = "TexturePlane";
    createVertices();
    updateBoundingBox(vertices_);
    Plane::createIndices(indices_, doubleSided);
}

TexturePlane::TexturePlane(std::unique_ptr<Material> pMaterial, glm::mat4 model, bool doubleSided)
    : TextureMesh(std::move(pMaterial), "", model) {
    markerName_ = "TexturePlane";
    createVertices();
    updateBoundingBox(vertices_);
    Plane::createIndices(indices_, doubleSided);
}

void TexturePlane::createVertices() {
    float width, height;
    width = height = 1.0f;
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