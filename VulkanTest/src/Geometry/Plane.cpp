
#include "Geometry/Plane.h"

Plane::Plane()
{
    createVertices();
    createIndices();
}

Plane::Plane(float width, float height, bool doubleSided, glm::vec3 pos, glm::mat4 rot)
{
    createVertices(width, height);

    for (auto& vertex : m_vertices)
    {
        // TODO: determine when to use 1 or 0 as the homogeneous coord
        vertex.pos = glm::vec4(vertex.pos, 1.0) * rot;
        vertex.pos += pos;
    }

    createIndices(doubleSided);
}


void Plane::createVertices(float width, float height)
{
    m_vertices.resize(PLANE_VERTEX_SIZE);
    float l = (width / 2 * -1), r = (width / 2);
    float b = (height / 2 * -1), t = (height / 2);
    m_vertices = {
        {
            // bottom left
            { l, 0.0f, b },
            { 1.0f, 0.0f, 0.0f }, // red
            { 0.0f, 0.0f }
        }, {
            // top left
            { l, 0.0f, t},
            { 0.0f, 1.0f, 0.0f }, // blue
            { 0.0f, 1.0f }
        }, {
            // bottom right
            { r, 0.0f,  b },
            { 0.0f, 0.0f, 1.0f }, // green
            { 1.0f, 0.0f }
        }, {
            // top right
            { r, 0.0f,  t },
            { 1.0f, 1.0f, 0.0f }, // yellow
            { 1.0f, 1.0f }
        },
    };
}

void Plane::createIndices(bool doubleSided)
{
    int indexSize = doubleSided ? (PLANE_INDEX_SIZE * 2) : PLANE_INDEX_SIZE;
    m_indices.resize(indexSize);
    m_indices = { 0, 1, 2, 2, 1, 3 };

    if (doubleSided)
    {
        for (int i = 2; i < PLANE_INDEX_SIZE; i += 3)
            for (int j = i; j > (i - 3); j--)
                m_indices.push_back(m_indices.at(j));
    }
}


