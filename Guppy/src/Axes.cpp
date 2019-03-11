
#include "Axes.h"
#include "MeshHandler.h"

namespace {
const glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
const glm::vec4 green(0.0f, 1.0f, 0.0f, 1.0f);
const glm::vec4 blue(0.0f, 0.0f, 1.0f, 1.0f);
const glm::vec4 white(1.0f);
void make(std::vector<Vertex::Color>& vertices, float lineSize, bool showNegative) {
    float max_ = lineSize;
    float min_ = max_ * -1;

    vertices = {
        {{0.0f, 0.0f, 0.0f}, glm::vec3(), red},    // X (RED)
        {{max_, 0.0f, 0.0f}, glm::vec3(), red},    // X
        {{0.0f, 0.0f, 0.0f}, glm::vec3(), green},  // Y (GREEN)
        {{0.0f, max_, 0.0f}, glm::vec3(), green},  // Y
        {{0.0f, 0.0f, 0.0f}, glm::vec3(), blue},   // Z (BLUE)
        {{0.0f, 0.0f, max_}, glm::vec3(), blue},   // Z

        // TEST LINES
        //{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // yellow through all positive
        //{{max_, max_, max_}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // yellow
    };

    if (showNegative) {
        std::vector<Vertex::Color> nvs = {
            {{0.0f, 0.0f, 0.0f}, glm::vec3(), white},  // -X (WHITE)
            {{min_, 0.0f, 0.0f}, glm::vec3(), white},  // -X
            {{0.0f, 0.0f, 0.0f}, glm::vec3(), white},  // -Y (WHITE)
            {{0.0f, min_, 0.0f}, glm::vec3(), white},  // -Y
            {{0.0f, 0.0f, 0.0f}, glm::vec3(), white},  // -Z (WHITE)
            {{0.0f, 0.0f, min_}, glm::vec3(), white},  // -Z
        };
        vertices.insert(vertices.end(), nvs.begin(), nvs.end());
    }
}
}  // namespace

Axes::Axes(Mesh::Handler& handler, AxesCreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Line(handler, "Axes", pCreateInfo, pMaterial) {
    isIndexed_ = false;
    make(vertices_, pCreateInfo->lineSize, pCreateInfo->showNegative);
    status_ = STATUS::PENDING_BUFFERS;
}

Axes::Axes(Mesh::Handler& handler, const std::string&& name, AxesCreateInfo* pCreateInfo,
           std::shared_ptr<Material::Base>& pMaterial)
    : Mesh::Line(handler, std::forward<const std::string>(name), pCreateInfo, pMaterial) {
    isIndexed_ = false;
    make(vertices_, pCreateInfo->lineSize, pCreateInfo->showNegative);
    status_ = STATUS::PENDING_BUFFERS;
}