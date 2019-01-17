#ifndef PIPELINE_HANDLER_H
#define PIPELINE_HANDLER_H

#include <vector>
#include <vulkan/vulkan.h>

#include "Material.h"
#include "Pipeline.h"
#include "Singleton.h"
#include "Shader.h"
#include "Vertex.h"

namespace Pipeline {

struct DefaultPushConstants {
    glm::mat4 model;
    Material::Data material;
};

struct DescriptorSetsReference {
    VkPipeline pipeline;
    VkPipelineBindPoint bindPoint;
    VkPipelineLayout layout;
    uint32_t firstSet;
    uint32_t descriptorSetCount;
    VkDescriptorSet *pDescriptorSets;
    std::vector<uint32_t> dynamicOffsets;
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

    // GENERAL
    static const VkPipelineCache &getPipelineCache() { return inst_.cache_; }

    // PUSH CONSTANT
    static const std::vector<VkPushConstantRange> getPushConstantRanges(PUSH_CONSTANT_TYPE &&type) {
        // TODO: this need more thought
        return inst_.pushConstantRanges_;  // Just return the whole thing for now.
    }

    // DESCRIPTOR
    static const VkDescriptorPool &getDescriptorPool() { return inst_.descPool_; }
    static const VkDescriptorSetLayout &getDescriptorSetLayouts(const std::set<DESCRIPTOR_TYPE> descTypeSet) {
        return inst_.descriptorMap_[descTypeSet].layout;
    }
    static void makeDescriptorSets(const PIPELINE_TYPE &type, DescriptorSetsReference &descSetsRef,
                                   const std::shared_ptr<Texture::Data> &pTexture = nullptr);

    // PIPELINES
    static void createPipelines(const std::unique_ptr<RenderPass::Base> &pPass, bool remake = false);
    static const VkPipeline &createPipeline(const PIPELINE_TYPE &type,
                                            const std::unique_ptr<RenderPass::Base> &pPass = nullptr, bool setBase = false);
    static inline const std::unique_ptr<Pipeline::Base> &getPipeline(const PIPELINE_TYPE &type) {
        return inst_.getPipelineInternal(type);
    }
    static inline const std::unique_ptr<Pipeline::Base> &getPipeline(PIPELINE_TYPE &&type) {
        return inst_.getPipelineInternal(std::forward<PIPELINE_TYPE>(type));
    }

    static void cleanup(int frameIndex = -1);

   private:
    Handler();     // Prevent construction
    ~Handler(){};  // Prevent destruction
    static Handler inst_;
    void reset() override;

    Shell *sh_;
    Shell::Context ctx_;       // TODO: shared_ptr
    Game::Settings settings_;  // TODO: shared_ptr

    // GENERAL
    VkPipelineCache cache_;  // TODO: what is this for???

    // PUSH CONSTANT
    void createDefaultPushConstantRange(const Shell::Context &ctx);
    std::vector<VkPushConstantRange> pushConstantRanges_;  // This is just one thing atm.

    // DESCRIPTOR
    void createDescriptorPool();
    void createDescriptorSetLayouts();
    void allocateDescriptorSets(const std::set<DESCRIPTOR_TYPE> types, const VkDescriptorSetLayout &layout,
                                DescriptorMapItem::Resource &resource, const std::shared_ptr<Texture::Data> &pTexture,
                                uint32_t count = 1);
    void validateDescriptorTypeKey(const std::set<DESCRIPTOR_TYPE> descTypes,
                                   const std::shared_ptr<Texture::Data> &pTexture);
    VkDescriptorPool descPool_;
    descriptor_resource_map descriptorMap_;

    // PIPELINES
    void createPipelineCache(VkPipelineCache &cache);

    inline VkGraphicsPipelineCreateInfo &getPipelineCreateInfo(const PIPELINE_TYPE &type) {
        switch (type) {
            case PIPELINE_TYPE::TRI_LIST_COLOR:
            case PIPELINE_TYPE::LINE:
            case PIPELINE_TYPE::TRI_LIST_TEX:
            default:
                return defaultPipelineInfo_;
        }
    }

    inline std::unique_ptr<Pipeline::Base> &getPipelineInternal(const PIPELINE_TYPE &type) {
        switch (type) {
            case PIPELINE_TYPE::TRI_LIST_COLOR:
                return pDefaultTriListColor_;
            case PIPELINE_TYPE::LINE:
                return pDefaultLine_;
            case PIPELINE_TYPE::TRI_LIST_TEX:
                return pDefaultTriListTex_;
            default:
                throw std::runtime_error("pipeline type not handled!");
        }
    }

    // DEFAULT
    VkGraphicsPipelineCreateInfo defaultPipelineInfo_;  // TODO: use this better through creation...
    CreateInfoResources defaultCreateInfoResources_;    // TODO: use this better through creation...
    std::unique_ptr<Pipeline::Base> pDefaultTriListColor_;
    std::unique_ptr<Pipeline::Base> pDefaultLine_;
    std::unique_ptr<Pipeline::Base> pDefaultTriListTex_;
    // global storage for clean up
    std::vector<std::pair<int, VkPipeline>> oldPipelines_;
};

}  // namespace Pipeline

#endif  // !PIPELINE_HANDLER_H
