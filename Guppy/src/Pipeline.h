#ifndef PIPELINE_H
#define PIPELINE_H

#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "Material.h"
#include "RenderPass.h"

namespace Pipeline {

struct CreateInfoResources;

// **********************
//      BASE
// **********************

class Base {
    friend class Handler;

   public:
    virtual void init(const Shell::Context &ctx, const Game::Settings &settings);

    const std::vector<SHADER_TYPE> SHADER_TYPES;
    PIPELINE_TYPE TYPE;
    uint32_t SUBPASS_ID;

    inline const VkPipelineLayout &getLayout() const { return layout_; }
    inline const VkPipeline &getPipeline() const { return pipeline_; }

    // DESCRIPTOR
    const std::set<DESCRIPTOR_TYPE> getDescriptorTypeSet();
    // TODO: if this ever gets overriden then this should take a struct
    virtual inline uint32_t getDescriptorSetOffset(const std::shared_ptr<Texture::Data> &pTexture) const { return 0; }

   protected:
    Base(VkPipelineBindPoint &&bindPoint, PIPELINE_TYPE &&pipelineType, std::vector<SHADER_TYPE> &&shaderTypes,
         std::string &&name)
        : bindPoint_(bindPoint),
          TYPE(pipelineType),
          SUBPASS_ID(0),
          SHADER_TYPES(shaderTypes),
          name_(name),
          layout_(VK_NULL_HANDLE),
          pipeline_(VK_NULL_HANDLE),
          descSetTypeInit_(false) {}

    const VkPipelineBindPoint bindPoint_;

    // INFOS
    virtual void getBlendInfoResources(CreateInfoResources &createInfoRes);
    virtual void getDepthInfoResources(CreateInfoResources &createInfoRes);
    virtual void getDynamicStateInfoResources(CreateInfoResources &createInfoRes);
    virtual void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) = 0;  // no default
    virtual void getMultisampleStateInfoResources(CreateInfoResources &createInfoRes);
    virtual void getRasterizationStateInfoResources(CreateInfoResources &createInfoRes);
    virtual void getShaderInfoResources(CreateInfoResources &createInfoRes) = 0;  // no default
    virtual void getTesselationInfoResources(CreateInfoResources &createInfoRes);
    virtual void getViewportStateInfoResources(CreateInfoResources &createInfoRes);

    virtual const VkPipeline &create(const Shell::Context &ctx, const Game::Settings &settings, const VkPipelineCache &cache,
                                     CreateInfoResources &createInfoRes, VkGraphicsPipelineCreateInfo &pipelineCreateInfo,
                                     const VkPipeline &basePipelineHandle = VK_NULL_HANDLE,
                                     const int32_t basePipelineIndex = -1);

    virtual void createPipelineLayout(const VkDevice &dev, const Game::Settings &settings);
    VkPipelineLayout layout_;
    VkPipeline pipeline_;
    std::string name_;

    void destroy(const VkDevice &dev);

   private:
    bool descSetTypeInit_;
    std::set<DESCRIPTOR_TYPE> descTypeSet_;
};

// **********************
//      Default Triangle List Color
// **********************

class DefaultTriListColor : public Base {
   public:
    DefaultTriListColor()
        : Base(VK_PIPELINE_BIND_POINT_GRAPHICS, PIPELINE_TYPE::TRI_LIST_COLOR,
               {SHADER_TYPE::COLOR_VERT, SHADER_TYPE::COLOR_FRAG, SHADER_TYPE::UTIL_FRAG}, "Default Triangle List Color"){};

    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
    void getShaderInfoResources(CreateInfoResources &createInfoRes) override;
};

// **********************
//      Default Line
// **********************

class DefaultLine : public Base {
   public:
    DefaultLine()
        : Base(VK_PIPELINE_BIND_POINT_GRAPHICS, PIPELINE_TYPE::LINE, {SHADER_TYPE::COLOR_VERT, SHADER_TYPE::LINE_FRAG},
               "Default Line"){};

    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
    void getShaderInfoResources(CreateInfoResources &createInfoRes) override;
};

// **********************
//      Default Triangle List Texture
// **********************

class DefaultTriListTexture : public Base {
   public:
    DefaultTriListTexture()
        : Base(VK_PIPELINE_BIND_POINT_GRAPHICS, PIPELINE_TYPE::TRI_LIST_TEX,
               {SHADER_TYPE::TEX_VERT, SHADER_TYPE::TEX_FRAG, SHADER_TYPE::UTIL_FRAG}, "Default Triangle List Texture"){};

    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
    void getShaderInfoResources(CreateInfoResources &createInfoRes) override;

    // DESCRIPTOR
    inline uint32_t getDescriptorSetOffset(const std::shared_ptr<Texture::Data> &pTexture) const override {
        return pTexture->offset;
    }
};

}  // namespace Pipeline

#endif  // !PIPELINE_H
