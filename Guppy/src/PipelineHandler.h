#ifndef PIPELINE_RESOURCES_H
#define PIPELINE_RESOURCES_H

#include <vector>
#include <vulkan/vulkan.h>

#include "MyShell.h"
#include "Vertex.h"

struct DescriptorResources {
    uint32_t colorCount;
    uint32_t texCount;
    bool updatePool;
    VkDescriptorPool pool;
    std::vector<VkDescriptorSet> sharedSets;
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
    static inline void destroy() { inst_.reset(); }

    PipelineHandler(const PipelineHandler &) = delete;             // Prevent construction by copying
    PipelineHandler &operator=(const PipelineHandler &) = delete;  // Prevent assignment

    enum class TOPOLOGY { TRI_LIST_COLOR = 0, LINE, TRI_LIST_TEX };

    struct DefaultAttachementReferences {
        VkAttachmentReference color;
        VkAttachmentReference colorResolve;
        VkAttachmentReference depth;
    };

    static void create_shader_module(const std::string &shaderText, VkShaderStageFlagBits stage, ShaderResources &shaderResources,
                                     bool initGlslang = true, std::string markerName = "");
    static void create_descriptor_pool(DescriptorResources &descResources);
    static void create_pipeline_cache(VkPipelineCache &cache);
    // static VkSubpassDescription PipelineHandler::create_default_subpass();
    static void create_pipeline_resources(PipelineResources &resources);
    static void create_render_pass(PipelineResources &resources);

    static inline const DefaultAttachementReferences &get_def_attach_refs() { return inst_.defAttachRefs_; }
    static void PipelineHandler::getDescriptorLayouts(uint32_t image_count, Vertex::TYPE type,
                                                      std::vector<VkDescriptorSetLayout> &layouts);
    // static inline VkPipelineLayout &get_default_pipeline_layout() { return inst_.layout_; }
    static inline const VkPipelineLayout &getPipelineLayout(Vertex::TYPE type) {
        return inst_.pipelineLayouts_[static_cast<int>(type)];
    }

    static void destroy_pipeline_resources(PipelineResources &resources);

   private:
    PipelineHandler();  // Prevent construction

    static PipelineHandler inst_;

    void reset();
    void create_shader_modules();
    void create_descriptor_set_layout(Vertex::TYPE type, VkDescriptorSetLayout &setLayout);
    void create_default_attachments(bool clear = true, VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    void create_subpasses(PipelineResources &resources);
    void create_dependencies(PipelineResources &resources);
    void create_pipeline_layout(Vertex::TYPE type, const VkDescriptorSetLayout &setLayout, std::string markerName = "");
    // maybe make something like below public ...
    void create_input_state_create_info(Vertex::TYPE type, PipelineCreateInfoResources &resources);
    void create_base_pipeline(Vertex::TYPE vertexType, TOPOLOGY pipelineType, const ShaderResources &vs, const ShaderResources &fs,
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
};

#endif  // !PIPELINE_RESOURCES_H
