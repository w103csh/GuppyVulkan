
#include "Face.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/normal.hpp>

void Face::calcNormal() {
    glm::vec3 normal = glm::triangleNormal(vertices_[0].position, vertices_[1].position, vertices_[2].position);
}

void Face::calcImageSpaceData() {
    glm::vec3 tangent = {}, bitangent = {};
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
    auto deltaPos1 = vertices_[1].position - vertices_[0].position;
    auto deltaPos2 = vertices_[2].position - vertices_[0].position;
    auto deltaUV1 = vertices_[1].texCoord - vertices_[0].texCoord;
    auto deltaUV2 = vertices_[2].texCoord - vertices_[0].texCoord;
    auto denom = (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);       // divisor
    tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.x) / denom;    // T
    bitangent = (deltaUV1.x * deltaPos2 - deltaPos2 * deltaUV2.x) / denom;  // B
}