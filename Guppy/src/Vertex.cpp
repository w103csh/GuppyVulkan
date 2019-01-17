
#include <vector>

#include "Vertex.h"

VkVertexInputBindingDescription Vertex::getColorBindDesc() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = BINDING;
    bindingDescription.stride = sizeof(Vertex::Color);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return std::move(bindingDescription);
}

std::vector<VkVertexInputAttributeDescription> Vertex::getColorAttrDesc() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

    // position
    attributeDescriptions[0].binding = BINDING;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[0].offset = offsetof(Vertex::Color, position);

    // normal
    attributeDescriptions[1].binding = BINDING;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[1].offset = offsetof(Vertex::Color, normal);

    // color
    attributeDescriptions[2].binding = BINDING;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    attributeDescriptions[2].offset = offsetof(Vertex::Color, color);

    return attributeDescriptions;
}

VkVertexInputBindingDescription Vertex::getTexBindDesc() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = BINDING;
    bindingDescription.stride = sizeof(Vertex::Texture);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription> Vertex::getTexAttrDesc() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(5);

    // position
    attributeDescriptions[0].binding = BINDING;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[0].offset = offsetof(Vertex::Texture, position);

    // normal
    attributeDescriptions[1].binding = BINDING;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[1].offset = offsetof(Vertex::Texture, normal);

    // texture coordinate
    attributeDescriptions[2].binding = BINDING;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;  // vec2
    attributeDescriptions[2].offset = offsetof(Vertex::Texture, texCoord);

    // image space tangent
    attributeDescriptions[3].binding = BINDING;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    attributeDescriptions[3].offset = offsetof(Vertex::Texture, tangent);

    // image space bitangent
    attributeDescriptions[4].binding = BINDING;
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