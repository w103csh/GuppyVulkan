#ifndef PIPELINE_CONSTANTS_H
#define PIPELINE_CONSTANTS_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

#include "DescriptorConstants.h"

enum class DESCRIPTOR_SET;
enum class PUSH_CONSTANT;
enum class PASS : uint32_t;
enum class SHADER;
enum class VERTEX;

// These are index values, so the order here must match order in ALL...
enum class PIPELINE : uint32_t {
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
    // SCREEN SPACE
    SCREEN_SPACE_DEFAULT,
    SCREEN_SPACE_COMPUTE_DEFAULT,
    // Used to indicate bad data, and "all" in uniform offsets
    ALL_ENUM = UINT32_MAX,
    // Add new to PIPELINE_ALL and VERTEX_PIPELINE_MAP
    // in code file.
};

namespace Pipeline {

extern const std::vector<PIPELINE> ALL;
extern const std::map<VERTEX, std::set<PIPELINE>> VERTEX_MAP;

struct BindData {
    const VkPipelineBindPoint bindPoint;
    const VkPipelineLayout layout;
    VkPipeline pipeline;
    VkShaderStageFlags pushConstantStages;
    const std::vector<PUSH_CONSTANT> pushConstantTypes;
};

// Map of pipeline/pass to bind data shared pointers
using pipelineBindDataMapKey = std::pair<PIPELINE, PASS>;
using pipelineBindDataMapKeyValue = std::pair<const pipelineBindDataMapKey, const std::shared_ptr<Pipeline::BindData>>;
using pipelineBindDataMap = std::map<pipelineBindDataMapKey, const std::shared_ptr<Pipeline::BindData>>;

// Map of pass types to descriptor set text replace tuples
using shaderTextReplaceInfoMap = std::map<std::set<PASS>, Descriptor::Set::textReplaceTuples>;

struct CreateInfo {
    PIPELINE type;
    std::string name = "";
    std::set<SHADER> shaderTypes;
    std::vector<DESCRIPTOR_SET> descriptorSets;
    Descriptor::OffsetsMap uniformOffsets;
    std::vector<PUSH_CONSTANT> pushConstantTypes;
};

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

namespace Default {
struct PushConstant {
    glm::mat4 model;
};
extern const Pipeline::CreateInfo TRI_LIST_COLOR_CREATE_INFO;
extern const Pipeline::CreateInfo LINE_CREATE_INFO;
extern const Pipeline::CreateInfo TRI_LIST_TEX_CREATE_INFO;
extern const Pipeline::CreateInfo CUBE_CREATE_INFO;
}  // namespace Default

namespace BP {
extern const Pipeline::CreateInfo TEX_CULL_NONE_CREATE_INFO;
}  // namespace BP

}  // namespace Pipeline

#endif  // !PIPELINE_CONSTANTS_H
