#ifndef PIPELINE_REFERENCE_H
#define PIPELINE_REFERENCE_H

#include <list>
#include <vulkan/vulkan.h>

#include "Helpers.h"

namespace Pipeline {

struct Reference {
    VkPipelineBindPoint bindPoint;
    VkPipelineLayout layout;
    VkPipeline pipeline;
    VkShaderStageFlags pushConstantStages;
    std::list<PUSH_CONSTANT> pushConstantTypes;
};

}  // namespace Pipeline

#endif  // !PIPELINE_REFERENCE_H
