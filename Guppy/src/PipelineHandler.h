#ifndef PIPELINE_HANDLER_H
#define PIPELINE_HANDLER_H

#include <set>
#include <vector>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Game.h"
#include "Material.h"
#include "Mesh.h"
#include "Pipeline.h"
#include "Vertex.h"

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

class Handler : public Game::Handler {
    friend class Base;

   public:
    Handler(Game *pGame);

    void init() override;
    inline void destroy() override {
        reset();
        cleanup();
    }

    // CACHE
    inline const VkPipelineCache &getPipelineCache() { return cache_; }

    // REFERENCE
    void getReference(Mesh::Base &mesh);

    // PUSH CONSTANT
    std::vector<VkPushConstantRange> getPushConstantRanges(const PIPELINE &pipelineType,
                                                           const std::vector<PUSH_CONSTANT> &pushConstantTypes) const;

    // PIPELINES
    void createPipelines(const std::unique_ptr<RenderPass::Base> &pPass, bool remake = false);
    const VkPipeline &createPipeline(const PIPELINE &type, const std::unique_ptr<RenderPass::Base> &pPass = nullptr,
                                     bool setBase = false);

    // DESCRIPTOR SET
    VkShaderStageFlags getDescriptorSetStages(const DESCRIPTOR_SET &setType);
    void initShaderInfoMap(Shader::shaderInfoMap &map);

    inline const std::unique_ptr<Pipeline::Base> &getPipeline(const PIPELINE &type) const {
        // Make sure the pipeline that is being accessed is available.
        assert(static_cast<int>(type) > static_cast<int>(PIPELINE::DONT_CARE) &&
               static_cast<int>(type) < pPipelines_.size());
        return pPipelines_[static_cast<uint64_t>(type)];
    }
    inline const std::vector<std::unique_ptr<Pipeline::Base>> &getPipelines() const { return pPipelines_; }

    // CLEAN UP
    void needsUpdate(const std::vector<SHADER> types);
    void cleanup(int frameIndex = -1);
    void update();

   private:
    void reset() override;

    // CACHE
    VkPipelineCache cache_;  // TODO: what is this for???

    // PUSH CONSTANT
    uint32_t maxPushConstantsSize_;

    // PIPELINES
    void createPipelineCache(VkPipelineCache &cache);

    inline VkGraphicsPipelineCreateInfo &getPipelineCreateInfo(const PIPELINE &type) {
        switch (type) {
            case PIPELINE::TRI_LIST_COLOR:
            case PIPELINE::LINE:
            case PIPELINE::TRI_LIST_TEX:
            default:
                return defaultPipelineInfo_;
        }
    }

    // DEFAULT (TODO: remove default things...)
    VkGraphicsPipelineCreateInfo defaultPipelineInfo_;  // TODO: use this better through creation...
    CreateInfoResources defaultCreateInfoResources_;    // TODO: use this better through creation...

    std::vector<std::unique_ptr<Pipeline::Base>> pPipelines_;

    // CLEAN UP
    std::set<PIPELINE> needsUpdateSet_;
    std::vector<std::pair<int, VkPipeline>> oldPipelines_;
};

}  // namespace Pipeline

#endif  // !PIPELINE_HANDLER_H
