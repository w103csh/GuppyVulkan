#ifndef PIPELINE_HANDLER_H
#define PIPELINE_HANDLER_H

#include <set>
#include <vector>
#include <vulkan/vulkan.h>

#include "Material.h"
#include "Pipeline.h"
#include "Singleton.h"
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
    // GENERAL (this is just for convenience...)
    const Shell::Context *pCtx;
    const Game::Settings *pSettings;
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

class Handler : public Singleton {
   public:
    static void init(Shell *sh, const Game::Settings &settings);
    static inline void destroy() {
        inst_.reset();
        inst_.cleanup();
    }

    Handler(const Handler &) = delete;             // Prevent construction by copying
    Handler &operator=(const Handler &) = delete;  // Prevent assignment

    // CACHE
    static const VkPipelineCache &getPipelineCache() { return inst_.cache_; }

    // DESCRIPTOR
    static const VkDescriptorPool &getDescriptorPool() { return inst_.descPool_; }
    static const VkDescriptorSetLayout &getDescriptorSetLayouts(const std::set<DESCRIPTOR> descTypeSet) {
        return inst_.descriptorMap_[descTypeSet].layout;
    }
    static void makeDescriptorSets(const PIPELINE &type, DescriptorSetsReference &descSetsRef,
                                   const std::shared_ptr<Texture::Data> &pTexture = nullptr);

    // PUSH CONSTANT
    static std::vector<VkPushConstantRange> getPushConstantRanges(const PIPELINE &pipelineType,
                                                                  const std::vector<PUSH_CONSTANT> &pushConstantTypes);

    // PIPELINES
    static void createPipelines(const std::unique_ptr<RenderPass::Base> &pPass, bool remake = false);
    static const VkPipeline &createPipeline(const PIPELINE &type, const std::unique_ptr<RenderPass::Base> &pPass = nullptr,
                                            bool setBase = false);

    static inline const std::unique_ptr<Pipeline::Base> &getPipeline(const PIPELINE &type) {
        assert(type != PIPELINE::DONT_CARE);
        return inst_.pPipelines_[static_cast<uint64_t>(type)];
    }

    // CLEAN UP
    static void needsUpdate(const std::vector<SHADER> types);
    static void cleanup(int frameIndex = -1);
    static void update();

   private:
    Handler();     // Prevent construction
    ~Handler(){};  // Prevent destruction
    static Handler inst_;
    void reset() override;

    Shell *sh_;
    Shell::Context ctx_;       // TODO: shared_ptr
    Game::Settings settings_;  // TODO: shared_ptr

    // CACHE
    VkPipelineCache cache_;  // TODO: what is this for???

    // DESCRIPTOR
    void createDescriptorPool();
    void createDescriptorSetLayouts();
    void allocateDescriptorSets(const std::set<DESCRIPTOR> types, const VkDescriptorSetLayout &layout,
                                DescriptorMapItem::Resource &resource, const std::shared_ptr<Texture::Data> &pTexture,
                                uint32_t count = 1);
    void validateDescriptorTypeKey(const std::set<DESCRIPTOR> descTypes, const std::shared_ptr<Texture::Data> &pTexture);
    VkDescriptorPool descPool_;
    descriptor_resource_map descriptorMap_;

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

    // DEFAULT
    VkGraphicsPipelineCreateInfo defaultPipelineInfo_;  // TODO: use this better through creation...
    CreateInfoResources defaultCreateInfoResources_;    // TODO: use this better through creation...
    std::vector<std::unique_ptr<Pipeline::Base>> pPipelines_;

    // CLEAN UP
    std::set<PIPELINE> needsUpdateSet_;
    std::vector<std::pair<int, VkPipeline>> oldPipelines_;
};

}  // namespace Pipeline

#endif  // !PIPELINE_HANDLER_H
