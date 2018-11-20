
#ifndef VERTEX_H
#define VERTEX_H

#include <array>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <vector>
#include <vulkan/vulkan.h>

namespace Vertex {

enum class TYPE { COLOR = 0, TEXTURE };

struct Base {
    Base() : pos(), normal(){};
    Base(glm::vec3 p, glm::vec3 n) : pos(p), normal(n){};
    bool operator==(const Base& other) const { return pos == other.pos && normal == other.normal; }
    glm::vec3 pos;
    glm::vec3 normal;
};

struct Color : Base {
    Color() : Base(), color(1.0f, 1.0f, 1.0f, 1.0f){};
    Color(glm::vec3 p, glm::vec3 n, glm::vec4 c) : Base(p, n), color(c){};
    bool operator==(const Color& other) const { return pos == other.pos && normal == other.normal && color == other.color; }
    glm::vec4 color;
};

struct Texture : Base {
    Texture() : Base(), texCoord(), tangent(), bitangent(){};
    Texture(glm::vec3 p, glm::vec3 n, glm::vec2 tc, glm::vec3 t, glm::vec3 b)
        : Base(p, n), texCoord(tc), tangent(t), bitangent(b){};
    bool operator==(const Texture& other) const { return pos == other.pos && normal == other.normal && texCoord == other.texCoord; }
    glm::vec2 texCoord;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

// color
VkVertexInputBindingDescription getColorBindDesc();
std::vector<VkVertexInputAttributeDescription> getColorAttrDesc();
// texture
VkVertexInputBindingDescription getTexBindDesc();
std::vector<VkVertexInputAttributeDescription> getTexAttrDesc();

std::string getTypeName(TYPE type);

}  // namespace Vertex

// **********************
// Hash functions
// **********************

namespace std {
// Hash function for Color
template <>
struct hash<Vertex::Color> {
    size_t operator()(Vertex::Color const& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.color) << 1);
    }
};
// Hash function for Texture
template <>
struct hash<Vertex::Texture> {
    size_t operator()(Vertex::Texture const& vertex) const {
        return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
               (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};
}  // namespace std

#endif  // !VERTEX_H