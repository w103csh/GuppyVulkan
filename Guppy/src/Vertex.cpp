
#include <vector>

#include "Vertex.h"

namespace {  // Utility functions
void getBaseAttrDesc(std::vector<VkVertexInputAttributeDescription> &attributeDescriptions) {
    assert(attributeDescriptions.capacity() > 1);

    // position
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[0].offset = offsetof(Vertex::Base, pos);

    // normal
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[1].offset = offsetof(Vertex::Base, normal);
}
}  // namespace

VkVertexInputBindingDescription Vertex::getColorBindDesc() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex::Color);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return std::move(bindingDescription);
}

std::vector<VkVertexInputAttributeDescription> Vertex::getColorAttrDesc() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

    // base descriptions
    getBaseAttrDesc(attributeDescriptions);

    // color
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    attributeDescriptions[2].offset = offsetof(Vertex::Color, color);

    return attributeDescriptions;
}

VkVertexInputBindingDescription Vertex::getTexBindDesc() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex::Texture);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> Vertex::getTexAttrDesc() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

    // base descriptions
    getBaseAttrDesc(attributeDescriptions);

    // texture coordinate
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;  // vec2
    attributeDescriptions[2].offset = offsetof(Vertex::Texture, texCoord);

    // image space tangent
    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[3].offset = offsetof(Vertex::Texture, tangent);

    // image space bitangent
    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[4].offset = offsetof(Vertex::Texture, bitangent);

    return attributeDescriptions;
}

std::string Vertex::getTypeName(TYPE type) {
    std::string name;
    switch (type) {
        case Vertex::TYPE::COLOR:
            name = "COLOR";
            break;
        case Vertex::TYPE::TEXTURE:
            name = "TEXTURE";
            break;
    }
    return name;
}