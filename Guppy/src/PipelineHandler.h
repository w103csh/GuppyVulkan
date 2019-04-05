#ifndef PIPELINE_HANDLER_H
#define PIPELINE_HANDLER_H

#include <list>
#include <set>
#include <vector>
#include <vulkan/vulkan.h>

#include "Game.h"
#include "Material.h"
#include "Mesh.h"
#include "Pipeline.h"
#include "PipelineReference.h"
#include "Shader.h"
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
    VkVertexInputBindingDescription bindingDesc = {};
    std::vector<VkVertexInputAttributeDescription> attrDesc;
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
                                                           const std::list<PUSH_CONSTANT> &pushConstantTypes) const;

    // PIPELINES
    void createPipelines(const std::unique_ptr<RenderPass::Base> &pPass, bool remake = false);
    const VkPipeline &createPipeline(const PIPELINE &type, const std::unique_ptr<RenderPass::Base> &pPass = nullptr,
                                     bool setBase = false);

    // DESCRIPTOR SET
    VkShaderStageFlags getDescriptorSetStages(const DESCRIPTOR_SET &setType);

    inline const std::unique_ptr<Pipeline::Base> &getPipeline(const PIPELINE &type) const {
        assert(type != PIPELINE::DONT_CARE);
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
