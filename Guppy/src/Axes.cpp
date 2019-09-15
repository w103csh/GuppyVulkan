
#include "Axes.h"

#include "ConstantsAll.h"

using namespace Mesh;

namespace {
void make(std::vector<Vertex::Color>& vertices, float lineSize, bool showNegative) {
    float max_ = lineSize;
    float min_ = max_ * -1;

    vertices = {
        {{0.0f, 0.0f, 0.0f}, glm::vec3(), COLOR_RED},    // X (RED)
        {{max_, 0.0f, 0.0f}, glm::vec3(), COLOR_RED},    // X
        {{0.0f, 0.0f, 0.0f}, glm::vec3(), COLOR_GREEN},  // Y (GREEN)
        {{0.0f, max_, 0.0f}, glm::vec3(), COLOR_GREEN},  // Y
        {{0.0f, 0.0f, 0.0f}, glm::vec3(), COLOR_BLUE},   // Z (BLUE)
        {{0.0f, 0.0f, max_}, glm::vec3(), COLOR_BLUE},   // Z

        // TEST LINES
        //{{0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // yellow through all positive
        //{{max_, max_, max_}, {1.0f, 1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},  // yellow
    };

    if (showNegative) {
        std::vector<Vertex::Color> nvs = {
            {{0.0f, 0.0f, 0.0f}, glm::vec3(), COLOR_WHITE},  // -X (WHITE)
            {{min_, 0.0f, 0.0f}, glm::vec3(), COLOR_WHITE},  // -X
            {{0.0f, 0.0f, 0.0f}, glm::vec3(), COLOR_WHITE},  // -Y (WHITE)
            {{0.0f, min_, 0.0f}, glm::vec3(), COLOR_WHITE},  // -Y
            {{0.0f, 0.0f, 0.0f}, glm::vec3(), COLOR_WHITE},  // -Z (WHITE)
            {{0.0f, 0.0f, min_}, glm::vec3(), COLOR_WHITE},  // -Z
        };
        vertices.insert(vertices.end(), nvs.begin(), nvs.end());
    }
}
}  // namespace

Axes::Axes(Handler& handler, const AxesCreateInfo* pCreateInfo, std::shared_ptr<Instance::Base>& pInstanceData,
           std::shared_ptr<Material::Base>& pMaterial)
    : Line(handler, "Axes", pCreateInfo, pInstanceData, pMaterial) {
    make(vertices_, pCreateInfo->lineSize, pCreateInfo->showNegative);
    status_ = STATUS::PENDING_BUFFERS;
}

Axes::Axes(Handler& handler, const std::string&& name, const AxesCreateInfo* pCreateInfo,
           std::shared_ptr<Instance::Base>& pInstanceData, std::shared_ptr<Material::Base>& pMaterial)
    : Line(handler, std::forward<const std::string>(name), pCreateInfo, pInstanceData, pMaterial) {
    make(vertices_, pCreateInfo->lineSize, pCreateInfo->showNegative);
    status_ = STATUS::PENDING_BUFFERS;
}