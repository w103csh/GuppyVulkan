
#pragma once

#include <array>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

class Vertex {
   public:
    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();

    bool operator==(const Vertex& other) const { return pos == other.pos && color == other.color && texCoord == other.texCoord; }

    glm::vec3 pos;
    glm::vec4 color;
    glm::vec2 texCoord;
};