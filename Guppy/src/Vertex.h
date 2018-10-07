
#ifndef VERTEX_H
#define VERTEX_H

#include <array>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <vulkan/vulkan.h>

// **********************
// Vertex
// **********************

class Vertex {
   public:
    Vertex() : pos({}), normal({}){};
    Vertex(glm::vec3 pos, glm::vec3 normal) : pos(pos), normal(normal){};

    bool operator==(const Vertex& other) const { return pos == other.pos && normal == other.normal; }

    glm::vec3 pos;
    glm::vec3 normal;
};

// **********************
// ColorVertex
// **********************

class ColorVertex : public Vertex {
   public:
    ColorVertex() : color({}){};
    ColorVertex(glm::vec3 pos, glm::vec3 normal, glm::vec4 color) : Vertex(pos, normal), color(color){};

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();

    bool operator==(const ColorVertex& other) const { return pos == other.pos && normal == other.normal && color == other.color; }

    glm::vec4 color;
};

// **********************
// TextureVertex
// **********************

class TextureVertex : public Vertex {
   public:
    TextureVertex() : texCoord({}){};
    TextureVertex(glm::vec3 pos, glm::vec3 normal, glm::vec2 texCoord) : Vertex(pos, normal), texCoord(texCoord){};

    static VkVertexInputBindingDescription getBindingDescription();
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();

    bool operator==(const TextureVertex& other) const {
        return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
    }

    glm::vec2 texCoord;
};

// **********************
// Hash functions
// **********************

namespace std {
// Hash function for ColorVertex class
template <>
struct hash<ColorVertex> {
    size_t operator()(ColorVertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.color) << 1);
    }
};
// Hash function for TextureVertex class
template <>
struct hash<TextureVertex> {
    size_t operator()(TextureVertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
}  // namespace std

#endif  // !VERTEX_H