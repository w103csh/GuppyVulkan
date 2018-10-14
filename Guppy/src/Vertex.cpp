
#include <vector>

#include "Vertex.h"

VkVertexInputBindingDescription Vertex::getColorBindDesc() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex::Color);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return std::move(bindingDescription);
}

std::vector<VkVertexInputAttributeDescription> Vertex::getColorAttrDesc() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[0].offset = offsetof(Vertex::Color, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[1].offset = offsetof(Vertex::Color, normal);

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
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[0].offset = offsetof(Vertex::Texture, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[1].offset = offsetof(Vertex::Texture, normal);

    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;  // vec2
    attributeDescriptions[2].offset = offsetof(Vertex::Texture, texCoord);

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