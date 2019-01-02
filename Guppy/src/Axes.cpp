
#include "Axes.h"

const glm::vec4 red(1.0f, 0.0f, 0.0f, 1.0f);
const glm::vec4 green(0.0f, 1.0f, 0.0f, 1.0f);
const glm::vec4 blue(0.0f, 0.0f, 1.0f, 1.0f);
const glm::vec4 white(1.0f);

Axes::Axes(MeshCreateInfo* pCreateInfo, float lineSize, bool showNegative) : LineMesh(pCreateInfo) {
    isIndexed_ = false;

    float max_ = lineSize;
    float min_ = max_ * -1;

    vertices_ = {
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
        vertices_.insert(vertices_.end(), nvs.begin(), nvs.end());
    }

    status_ = STATUS::PENDING_BUFFERS;
}