#pragma once

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Vertex.h"

constexpr auto PLANE_VERTEX_SIZE = 4;
constexpr auto PLANE_INDEX_SIZE = 6;

// TODO: deal with color

class Plane {
   public:
    Plane();
    Plane(float width, float height, bool doubleSided = false, glm::vec3 pos = glm::vec3(), glm::mat4 rot = glm::mat4(1.0f));

    inline const std::vector<Vertex>& getVertices() const { return vertices_; }

    inline const std::vector<VB_INDEX_TYPE>& getIndices() const { return indices_; }

   private:
    void createVertices(float width = 2.0f, float height = 2.0f);
    void createIndices(bool doubleSided = false);
    std::vector<Vertex> vertices_;
    std::vector<VB_INDEX_TYPE> indices_;
};