
#include "Plane.h"

Plane::Plane() {
    createVertices();
    createIndices();
}

Plane::Plane(float width, float height, bool doubleSided, glm::vec3 pos, glm::mat4 rot) {
    createVertices(width, height);

    for (auto& vertex : vertices_) {
        // TODO: determine when to use 1 or 0 as the homogeneous coord
        vertex.pos = glm::vec4(vertex.pos, 1.0) * rot;
        vertex.pos += pos;
    }

    createIndices(doubleSided);
}

void Plane::createVertices(float width, float height) {
    vertices_.resize(PLANE_VERTEX_SIZE);
    float l = (width / 2 * -1), r = (width / 2);
    float b = (height / 2 * -1), t = (height / 2);
    vertices_ = {
        {
            // geom - bottom left
            {l, b, 0.0f},              //
            {1.0f, 0.0f, 0.0f, 1.0f},  // red
            {0.0f, 1.0f}               // tex - bottom left
        },                             //
        {
            // geom - bottom right
            {r, b, 0.0f},              //
            {0.0f, 0.0f, 1.0f, 1.0f},  // blue
            {1.0f, 1.0f}               // tex - bottom right
        },                             //
        {
            // geom - top left
            {l, t, 0.0f},              //
            {0.0f, 1.0f, 0.0f, 1.0f},  // green
            {0.0f, 0.0f}               // tex - top left
        },                             //
        {
            // geom - top right
            {r, t, 0.0f},              //
            {1.0f, 1.0f, 0.0f, 1.0f},  // yellow
            {1.0f, 0.0f}               // tex - top right
        },                             //
    };
}

void Plane::createIndices(bool doubleSided) {
    int indexSize = doubleSided ? (PLANE_INDEX_SIZE * 2) : PLANE_INDEX_SIZE;
    indices_.resize(indexSize);
    indices_ = {0, 1, 2, 2, 1, 3};

    if (doubleSided) {
        for (int i = 2; i < PLANE_INDEX_SIZE; i += 3)
            for (int j = i; j > (i - 3); j--) indices_.push_back(indices_.at(j));
    }
}
