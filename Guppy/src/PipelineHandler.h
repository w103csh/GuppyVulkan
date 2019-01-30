#ifndef PIPELINE_HANDLER_H
#define PIPELINE_HANDLER_H

#include <set>
#include <vector>
#include <vulkan/vulkan.h>

#include "Game.h"
#include "Material.h"
#include "Pipeline.h"
#include "Shader.h"
#include "Vertex.h"

namespace Pipeline {

// Change name to pipeline reference or something...
struct DescriptorSetsReference {
    VkPipeline pipeline;
    VkPipelineBindPoint bindPoint;
    VkPipelineLayout layout;
    uint32_t firstSet;
    uint32_t descriptorSetCount;
    VkDescriptorSet *pDescriptorSets;
    std::vector<uint32_t> dynamicOffsets;
    //
    std::vector<PUSH_CONSTANT> pushConstantTypes;
    VkShaderStageFlags pushConstantStages;
};

struct CreateInfoResources {
    // BLENDING
    VkPipelineColorBlendAttachmentState blendAttach;
    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo;
    // DYNAMIC
    VkDynamicState dynamicStates[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
    // INPUT ASSEMBLY
    VkVertexInputBindingDescription bindingDesc;
    std::vector<VkVertexInputAttributeDescription> attrDesc;
    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo;
    // FIXED FUNCTION
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo;
    VkPipelineTessellationStateCreateInfo tessellationStateInfo;
    VkPipelineViewportStateCreateInfo viewportStateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo;
    VkPipelineMultisampleStateCreateInfo multisampleStateInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo;
    // SHADER
    std::vector<VkPipelineShaderStageCreateInfo> stagesInfo;
};

class Handler : public Game::Handler {
    friend class Base;

   public:
    Handler(Game *pGame);

    void init() override;
    inline void destroy() {
        reset();
        cleanup();
    }

    // CACHE
    inline const VkPipelineCache &getPipelineCache() { return cache_; }

    // DESCRIPTOR
    inline const VkDescriptorPool &getDescriptorPool() { return descPool_; }
    inline const VkDescriptorSetLayout &getDescriptorSetLayouts(const std::set<DESCRIPTOR> descTypeSet) const {
        return descriptorMap_.at(descTypeSet).layout;  // this can throw
    }
    void makeDescriptorSets(const PIPELINE &type, DescriptorSetsReference &descSetsRef,
                            const std::shared_ptr<Texture::DATA> &pTexture = nullptr);

    // PUSH CONSTANT
    std::vector<VkPushConstantRange> getPushConstantRanges(const PIPELINE &pipelineType,
                                                           const std::vector<PUSH_CONSTANT> &pushConstantTypes) const;

    // PIPELINES
    void createPipelines(const std::unique_ptr<RenderPass::Base> &pPass, bool remake = false);
    const VkPipeline &createPipeline(const PIPELINE &type, const std::unique_ptr<RenderPass::Base> &pPass = nullptr,
                                     bool setBase = false);

    inline const std::unique_ptr<Pipeline::Base> &getPipeline(const PIPELINE &type) const {
        assert(type != PIPELINE::DONT_CARE);
        return pPipelines_[static_cast<uint64_t>(type)];
    }

    // CLEAN UP
    void needsUpdate(const std::vector<SHADER> types);
    void cleanup(int frameIndex = -1);
    void update();

   private:
    void reset() override;

    // CACHE
    VkPipelineCache cache_;  // TODO: what is this for???

    // DESCRIPTOR
    void createDescriptorPool();
    void createDescriptorSetLayouts();
    void allocateDescriptorSets(const std::set<DESCRIPTOR> types, const VkDescriptorSetLayout &layout,
                                DescriptorMapItem::Resource &resource, const std::shared_ptr<Texture::DATA> &pTexture,
                                uint32_t count = 1);
    void validateDescriptorTypeKey(const std::set<DESCRIPTOR> descTypes, const std::shared_ptr<Texture::DATA> &pTexture);
    VkDescriptorPool descPool_;
    descriptor_resource_map descriptorMap_;
    std::vector<std::pair<uint32_t, UNIFORM>> BINDING_UNIFORM_LIST;

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
