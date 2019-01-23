
#ifndef INSTANCE_DATA_H
#define INSTANCE_DATA_H

#include <array>
#include <glm/glm.hpp>
#include <vector>
#include <vulkan/vulkan.h>

#include "Material.h"

namespace Instance {

const uint32_t BINDING = 1;

struct Data {
    glm::mat4 model = glm::mat4(1.0f);
    Material::Base::DATA materialData;
};

// color (instanced)
std::array<VkVertexInputBindingDescription, 2> getInstColorBindDesc();
std::vector<VkVertexInputAttributeDescription> getInstColorAttrDesc();
// texture (instanced)
std::array<VkVertexInputBindingDescription, 2> getInstTexBindDesc();
std::vector<VkVertexInputAttributeDescription> getInstTexAttrDesc();

}  // namespace Instance

#endif  // !INSTANCE_DATA_H