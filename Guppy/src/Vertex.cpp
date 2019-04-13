
#include <vector>

#include "Vertex.h"

void Vertex::Color::getBindingDescriptions(std::vector<VkVertexInputBindingDescription>& descs) {
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().stride = sizeof(Vertex::Color);
    descs.back().inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void Vertex::Color::getAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& descs) {
    // position
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().location = 0;
    descs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    descs.back().offset = offsetof(Vertex::Color, position);

    // normal
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().location = 1;
    descs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    descs.back().offset = offsetof(Vertex::Color, normal);

    // color
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().location = 2;
    descs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    descs.back().offset = offsetof(Vertex::Color, color);
}

void Vertex::Texture::getBindingDescriptions(std::vector<VkVertexInputBindingDescription>& descs) {
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().stride = sizeof(Vertex::Texture);
    descs.back().inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
}

void Vertex::Texture::getAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& descs) {
    // position
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().location = 0;
    descs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    descs.back().offset = offsetof(Vertex::Texture, position);

    // normal
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().location = 1;
    descs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    descs.back().offset = offsetof(Vertex::Texture, normal);

    // texture coordinate
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().location = 2;
    descs.back().format = VK_FORMAT_R32G32_SFLOAT;  // vec2
    descs.back().offset = offsetof(Vertex::Texture, texCoord);

    // image space tangent
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().location = 3;
    descs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    descs.back().offset = offsetof(Vertex::Texture, tangent);

    // image space bitangent
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().location = 4;
    descs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    descs.back().offset = offsetof(Vertex::Texture, bitangent);
}
