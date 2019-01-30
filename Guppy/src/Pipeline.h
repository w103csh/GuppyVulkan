#ifndef PIPELINE_H
#define PIPELINE_H

#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Handlee.h"
#include "Helpers.h"
#include "Object3d.h"
#include "Material.h"
#include "RenderPass.h"

namespace Pipeline {

struct CreateInfoResources;
class Handler;

// **********************
//      BASE
// **********************

class Base : public Handlee<Pipeline::Handler> {
    friend class Handler;

   public:
    const PIPELINE TYPE;
    const std::set<SHADER> SHADER_TYPES;
    const std::vector<PUSH_CONSTANT> PUSH_CONSTANT_TYPES;
    const VkPipelineBindPoint BIND_POINT;
    const std::string NAME;

    uint32_t SUBPASS_ID;

    virtual void init();

    inline const VkPipelineLayout &getLayout() const { return layout_; }
    inline const VkPipeline &getPipeline() const { return pipeline_; }

    // DESCRIPTOR
    const std::set<DESCRIPTOR> getDescriptorTypeSet();
    // TODO: if this ever gets overriden then this should take a struct
    virtual inline uint32_t getDescriptorSetOffset(const std::shared_ptr<Texture::DATA> &pTexture) const { return 0; }

   protected:
    Base(const Pipeline::Handler &handler, const PIPELINE &&type, const std::set<SHADER> &&shaderTypes,
         const std::vector<PUSH_CONSTANT> &&pushConstantTypes, const VkPipelineBindPoint &&bindPoint,
         const std::string &&name)
        : Handlee(handler),
          TYPE(type),
          SHADER_TYPES(shaderTypes),
          PUSH_CONSTANT_TYPES(pushConstantTypes),
          BIND_POINT(bindPoint),
          SUBPASS_ID(0),
          NAME(name),
          layout_(VK_NULL_HANDLE),
          pipeline_(VK_NULL_HANDLE),
          descSetTypeInit_(false) {
        for (const auto &type : PUSH_CONSTANT_TYPES) assert(type != PUSH_CONSTANT::DONT_CARE);
    }

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

    virtual const VkPipeline &create(const VkPipelineCache &cache, CreateInfoResources &createInfoRes,
                                     VkGraphicsPipelineCreateInfo &pipelineCreateInfo,
                                     const VkPipeline &basePipelineHandle = VK_NULL_HANDLE,
                                     const int32_t basePipelineIndex = -1);

    virtual void createPipelineLayout();
    VkPipelineLayout layout_;
    VkPipeline pipeline_;

    void destroy();

   private:
    // DESCRIPTOR
    bool descSetTypeInit_;              // TODO: initialize value???
    std::set<DESCRIPTOR> descTypeSet_;  // TODO: initialize value???
    std::vector<VkDescriptorSet> descriptorSets_;
    // PUSH CONSTANT
    std::vector<VkPushConstantRange> pushConstantRanges_;
};

namespace Default {

struct PushConstant {
    Object3d::DATA obj3d;
    Material::Default::DATA material;
};

// **********************
//      Triangle List Color
// **********************
class TriListColor : public Base {
   public:
    TriListColor(const Pipeline::Handler &handler)
        : Base{handler,  //
               PIPELINE::TRI_LIST_COLOR,
               {SHADER::COLOR_VERT, SHADER::COLOR_FRAG},
               {PUSH_CONSTANT::DEFAULT},
               VK_PIPELINE_BIND_POINT_GRAPHICS,
               "Default Triangle List Color"} {};

    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
    void getShaderInfoResources(CreateInfoResources &createInfoRes) override;
};

// **********************
//      Line
// **********************
class Line : public Base {
   public:
    Line(const Pipeline::Handler &handler)
        : Base{handler,  //
               PIPELINE::LINE,
               {SHADER::COLOR_VERT, SHADER::LINE_FRAG},
               {PUSH_CONSTANT::DEFAULT},
               VK_PIPELINE_BIND_POINT_GRAPHICS,
               "Default Line"} {};

    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
    void getShaderInfoResources(CreateInfoResources &createInfoRes) override;
};

// **********************
//      Triangle List Texture
// **********************
class TriListTexture : public Base {
   public:
    TriListTexture(const Pipeline::Handler &handler)
        : Base{handler,  //
               PIPELINE::TRI_LIST_TEX,
               {SHADER::TEX_VERT, SHADER::TEX_FRAG},
               {PUSH_CONSTANT::DEFAULT},
               VK_PIPELINE_BIND_POINT_GRAPHICS,
               "Default Triangle List Texture"} {};

    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
    void getShaderInfoResources(CreateInfoResources &createInfoRes) override;

    // DESCRIPTOR
    inline uint32_t getDescriptorSetOffset(const std::shared_ptr<Texture::DATA> &pTexture) const override {
        return pTexture->offset;
    }
};
}  // namespace Default

}  // namespace Pipeline

#endif  // !PIPELINE_H
