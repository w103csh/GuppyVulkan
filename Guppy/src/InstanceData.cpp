
#include <vector>

#include "InstanceData.h"
#include "Vertex.h"

std::array<VkVertexInputBindingDescription, 2> Instance::getInstColorBindDesc() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = BINDING;
    bindingDescription.stride = sizeof(Instance::Data);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    return {Vertex::getColorBindDesc(), bindingDescription};
}

std::vector<VkVertexInputAttributeDescription> Instance::getInstColorAttrDesc() {
    VkVertexInputAttributeDescription desc;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::getTexAttrDesc();

    // model
    auto modelOffset = offsetof(Instance::Data, model);
    // column 0
    desc.binding = BINDING;
    desc.location = static_cast<uint32_t>(attributeDescriptions.size());
    desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    desc.offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 0);
    attributeDescriptions.push_back(desc);
    // column 1
    desc.binding = BINDING;
    desc.location = static_cast<uint32_t>(attributeDescriptions.size());
    desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    desc.offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 1);
    attributeDescriptions.push_back(desc);
    // column 2
    desc.binding = BINDING;
    desc.location = static_cast<uint32_t>(attributeDescriptions.size());
    desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    desc.offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 2);
    attributeDescriptions.push_back(desc);
    // column 3
    desc.binding = BINDING;
    desc.location = static_cast<uint32_t>(attributeDescriptions.size());
    desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    desc.offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 3);
    attributeDescriptions.push_back(desc);

    return attributeDescriptions;
}

std::array<VkVertexInputBindingDescription, 2> Instance::getInstTexBindDesc() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = BINDING;
    bindingDescription.stride = sizeof(Instance::Data);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    return {Vertex::getTexBindDesc(), bindingDescription};
}

std::vector<VkVertexInputAttributeDescription> Instance::getInstTexAttrDesc() {
    VkVertexInputAttributeDescription desc;
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions = Vertex::getTexAttrDesc();

    // model
    auto modelOffset = offsetof(Instance::Data, model);
    // column 0
    desc.binding = BINDING;
    desc.location = static_cast<uint32_t>(attributeDescriptions.size());
    desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    desc.offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 0);
    attributeDescriptions.push_back(desc);
    // column 1
    desc.binding = BINDING;
    desc.location = static_cast<uint32_t>(attributeDescriptions.size());
    desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    desc.offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 1);
    attributeDescriptions.push_back(desc);
    // column 2
    desc.binding = BINDING;
    desc.location = static_cast<uint32_t>(attributeDescriptions.size());
    desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    desc.offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 2);
    attributeDescriptions.push_back(desc);
    // column 3
    desc.binding = BINDING;
    desc.location = static_cast<uint32_t>(attributeDescriptions.size());
    desc.format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    desc.offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 3);
    attributeDescriptions.push_back(desc);

    return attributeDescriptions;
}