
#ifndef PIPELINE_CONSTANTS_H
#define PIPELINE_CONSTANTS_H

#include <map>
#include <set>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

#include "RenderPassConstants.h"

enum class PUSH_CONSTANT;
enum class VERTEX;

enum class PIPELINE {
    DONT_CARE = -1,
    // These are index values, so the order here
    // must match order in ALL...
    // DEFAULT
    TRI_LIST_COLOR = 0,
    LINE,
    TRI_LIST_TEX,
    CUBE,
    // PBR
    PBR_COLOR,
    PBR_TEX,
    // BLINN PHONG
    BP_TEX_CULL_NONE,
    // PARALLAX
    PARALLAX_SIMPLE,
    PARALLAX_STEEP,
    // Add new to PIPELINE_ALL and VERTEX_PIPELINE_MAP
    // in code file.
};

extern const std::vector<PIPELINE> PIPELINE_ALL;
extern const std::map<VERTEX, std::set<PIPELINE>> VERTEX_PIPELINE_MAP;

namespace Pipeline {

struct CreateInfoResources {
    // BLENDING
    VkPipelineColorBlendAttachmentState blendAttach = {};
    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = {};
    // DYNAMIC
    VkDynamicState dynamicStates[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo dynamicStateInfo = {};
    // INPUT ASSEMBLY
    std::vector<VkVertexInputBindingDescription> bindDescs;
    std::vector<VkVertexInputAttributeDescription> attrDescs;
    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = {};
    // FIXED FUNCTION
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo = {};
    VkPipelineTessellationStateCreateInfo tessellationStateInfo = {};
    VkPipelineViewportStateCreateInfo viewportStateInfo = {};
    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo = {};
    VkPipelineMultisampleStateCreateInfo multisampleStateInfo = {};
    VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = {};
    // SHADER
    std::vector<VkPipelineShaderStageCreateInfo> stagesInfo;
};

// key:     pipeline type
// value:   render pass handler offset
using pipelineMapKey = std::pair<PIPELINE, RenderPass::offset>;
using pipelineMapKeyValue = std::pair<const pipelineMapKey, VkPipeline>;
using pipelineMap = std::map<pipelineMapKey, VkPipeline>;
using pipelineMapKeys = std::vector<Pipeline::pipelineMapKey>;

struct Reference {
    VkPipelineBindPoint bindPoint;
    VkPipelineLayout layout;
    VkPipeline pipeline;
    VkShaderStageFlags pushConstantStages;
    std::vector<PUSH_CONSTANT> pushConstantTypes;
};

}  // namespace Pipeline

#endif  // !PIPELINE_CONSTANTS_H
