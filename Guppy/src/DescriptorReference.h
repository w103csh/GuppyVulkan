#ifndef DESCRIPTOR_REFERENCE_H
#define DESCRIPTOR_REFERENCE_H

#include <vector>
#include <vulkan/vulkan.h>

namespace Descriptor {

struct Reference {
    uint32_t firstSet;
    std::vector<std::vector<VkDescriptorSet>> descriptorSets;
    std::vector<uint32_t> dynamicOffsets;
};

}  // namespace Descriptor

#endif  // !DESCRIPTOR_REFERENCE_H
