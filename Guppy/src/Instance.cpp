
#include "Instance.h"

void Instance::Default::DATA::getBindingDescriptions(std::vector<VkVertexInputBindingDescription>& descs) {
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().stride = sizeof(Instance::Default::DATA);
    descs.back().inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
}

void Instance::Default::DATA::getAttributeDescriptions(std::vector<VkVertexInputAttributeDescription>& descs) {
    // model
    auto modelOffset = offsetof(Instance::Default::DATA, model);

    // column 0
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().location = static_cast<uint32_t>(descs.size() - 1);
    descs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    descs.back().offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 0);
    // column 1
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().location = static_cast<uint32_t>(descs.size() - 1);
    descs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    descs.back().offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 1);
    // column 2
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().location = static_cast<uint32_t>(descs.size() - 1);
    descs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    descs.back().offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 2);
    // column 3
    descs.push_back({});
    descs.back().binding = BINDING;
    descs.back().location = static_cast<uint32_t>(descs.size() - 1);
    descs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    descs.back().offset = static_cast<uint32_t>(modelOffset + sizeof(glm::vec4) * 3);
}

Instance::Default::Base::Base(const Buffer::Info&& info, Default::DATA* pData)
    : Instance::Base(),  //
      Buffer::DataItem<Default::DATA>(pData),
      Buffer::Item(std::forward<const Buffer::Info>(info)) {
    DIRTY = true;
}
