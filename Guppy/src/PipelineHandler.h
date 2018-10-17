#ifndef PIPELINE_RESOURCES_H
#define PIPELINE_RESOURCES_H

#include <vector>
#include <vulkan/vulkan.h>

#include "MyShell.h"
#include "Vertex.h"

struct DescriptorResources {
    DescriptorResources(std::vector<VkDescriptorBufferInfo> uboInfos, size_t colorCount, size_t texCount)
        : pool(nullptr), uboInfos(uboInfos), colorCount(colorCount), texCount(texCount) {}
    VkDescriptorPool pool;
    size_t colorCount, texCount;
    std::vector<VkDescriptorBufferInfo> uboInfos;
    std::vector<VkDescriptorSet> colorSets;
    std::vector<std::vector<VkDescriptorSet>> texSets;
};

struct PipelineCreateInfoResources {
    // shader
    std::array<VkPipelineShaderStageCreateInfo, 2> stagesInfo;
    // vertex
    VkVertexInputBindingDescription bindingDesc;
    std::vector<VkVertexInputAttributeDescription> attrDesc;
    VkPipelineVertexInputStateCreateInfo vertexInputStateInfo;
    //
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateInfo;
    VkPipelineTessellationStateCreateInfo tessellationStateInfo;
    VkPipelineViewportStateCreateInfo viewportStateInfo;
    VkPipelineRasterizationStateCreateInfo rasterizationStateInfo;
    VkPipelineMultisampleStateCreateInfo multisampleStateInfo;
    VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo;
    // blending
    VkPipelineColorBlendAttachmentState blendAttach;
    VkPipelineColorBlendStateCreateInfo colorBlendStateInfo;
    // dynamic
    VkDynamicState dynamicStates[VK_DYNAMIC_STATE_RANGE_SIZE];
    VkPipelineDynamicStateCreateInfo dynamicStateInfo;
};

struct PipelineResources {
    std::string markerName;
    std::vector<VkSubpassDescription> subpasses;
    std::vector<VkSubpassDependency> dependencies;
    VkRenderPass renderPass;
    VkGraphicsPipelineCreateInfo pipelineInfo;
    // TRI_LIST_COLOR, TRI_LIST_TEX, COLOR
    std::array<VkPipeline, 3> pipelines;
};

class PipelineHandler {
   public:
    static void init(const MyShell::Context &ctx, const Game::Settings &settings);
    static inline void destroy() {
        inst_.reset();
        inst_.cleanupOldResources();
    }

    PipelineHandler(const PipelineHandler &) = delete;             // Prevent construction by copying
    PipelineHandler &operator=(const PipelineHandler &) = delete;  // Prevent assignment

    enum class TOPOLOGY { TRI_LIST_COLOR = 0, LINE, TRI_LIST_TEX };

    struct DefaultAttachementReferences {
        VkAttachmentReference color;
        VkAttachmentReference colorResolve;
        VkAttachmentReference depth;
    };

    static void createShaderModule(const std::string &shaderText, VkShaderStageFlagBits stage, ShaderResources &shaderResources,
                                     bool initGlslang = true, std::string markerName = "");

    static std::unique_ptr<DescriptorResources> createDescriptorResources(std::vector<VkDescriptorBufferInfo> uboInfos,
                                                                          size_t colorCount, size_t texCount);
    static void createTextureDescriptorSets(const VkDescriptorImageInfo &info, int offset,
                                            std::unique_ptr<DescriptorResources> &pRes);

    static void createPipelineCache(VkPipelineCache &cache);
    // static VkSubpassDescription PipelineHandler::create_default_subpass();
    static void createPipelineResources(PipelineResources &resources);
    static void createRenderPass(PipelineResources &resources);

    static inline const DefaultAttachementReferences &get_def_attach_refs() { return inst_.defAttachRefs_; }
    static void PipelineHandler::getDescriptorLayouts(uint32_t image_count, Vertex::TYPE type,
                                                      std::vector<VkDescriptorSetLayout> &layouts);
    // static inline VkPipelineLayout &get_default_pipeline_layout() { return inst_.layout_; }
    static inline const VkPipelineLayout &getPipelineLayout(Vertex::TYPE type) {
        return inst_.pipelineLayouts_[static_cast<int>(type)];
    }

    static void destroyPipelineResources(PipelineResources &resources);
    static void destroyDescriptorResources(std::unique_ptr<DescriptorResources> &resources);

    // old resources
    static inline void takeOldResources(std::unique_ptr<DescriptorResources> pRes) { inst_.oldDescRes_.push_back(std::move(pRes)); }
    static void cleanupOldResources();

   private:
    PipelineHandler();  // Prevent construction

    static PipelineHandler inst_;

    void reset();
    void createShaderModules();
    void createDescriptorPool(std::unique_ptr<DescriptorResources> &pDescResources);
    void createDescriptorSetLayout(Vertex::TYPE type, VkDescriptorSetLayout &setLayout);
    void createDefaultAttachments(bool clear = true, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    void createSubpasses(PipelineResources &resources);
    void createDependencies(PipelineResources &resources);
    void createPipelineLayout(Vertex::TYPE type, const VkDescriptorSetLayout &setLayout, std::string markerName = "");
    // maybe make something like below public ...
    void createInputStateCreateInfo(Vertex::TYPE type, PipelineCreateInfoResources &resources);
    void createBasePipeline(Vertex::TYPE vertexType, TOPOLOGY pipelineType, const ShaderResources &vs, const ShaderResources &fs,
                              PipelineCreateInfoResources &createRes, PipelineResources &pipelineRes);

    MyShell::Context ctx_;     // TODO: shared_ptr
    Game::Settings settings_;  // TODO: shared_ptr

    // Default instances ... (so that users can make pipeline derivatives from these)
    std::array<VkDescriptorSetLayout, 2> setLayouts_;  // COLOR, TEXTURE
    VkPipelineCache cache_;                            // TODO: what is this for???
    std::array<VkPipelineLayout, 2> pipelineLayouts_;  // COLOR, TEXTURE
    // attachments
    DefaultAttachementReferences defAttachRefs_;
    std::vector<VkAttachmentDescription> attachments_;
    // VERTEX SHADERS
    ShaderResources colorVS_;
    ShaderResources colorFS_;
    // FRAGMENT SHADERS
    ShaderResources texVS_;
    ShaderResources texFS_;
    // create info
    PipelineCreateInfoResources createResources;
    // global storage for clean up
    std::vector<std::unique_ptr<DescriptorResources>> oldDescRes_;
};

#endif  // !PIPELINE_RESOURCES_H
