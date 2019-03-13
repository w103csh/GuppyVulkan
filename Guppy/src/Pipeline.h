#ifndef PIPELINE_H
#define PIPELINE_H

#include <list>
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
    const std::list<DESCRIPTOR_SET> DESCRIPTOR_SET_TYPES;
    const std::set<SHADER> SHADER_TYPES;
    const std::list<PUSH_CONSTANT> PUSH_CONSTANT_TYPES;
    const VkPipelineBindPoint BIND_POINT;
    const std::string NAME;

    virtual void init();

    inline const VkPipelineLayout &getLayout() const { return layout_; }
    inline const VkPipeline &getPipeline() const { return pipeline_; }
    inline uint32_t getSubpassId() const { return subpassId_; }

    // TODO: if this ever gets overriden then this should take a struct
    virtual inline uint32_t getDescriptorSetOffset(const std::shared_ptr<Texture::DATA> &pTexture) const { return 0; }

   protected:
    Base(Pipeline::Handler &handler, const PIPELINE &&type, const VkPipelineBindPoint &&bindPoint, const std::string &&name,
         const std::set<SHADER> &&shaderTypes, const std::list<PUSH_CONSTANT> &&pushConstantTypes = {},
         std::list<DESCRIPTOR_SET> &&descriptorSets = {})
        : Handlee(handler),
          BIND_POINT(bindPoint),
          DESCRIPTOR_SET_TYPES(descriptorSets),
          NAME(name),
          PUSH_CONSTANT_TYPES(pushConstantTypes),
          SHADER_TYPES(shaderTypes),
          TYPE(type),
          layout_(VK_NULL_HANDLE),
          pipeline_(VK_NULL_HANDLE),
          subpassId_(0),
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
    uint32_t subpassId_;
    // DESCRIPTOR
    bool descSetTypeInit_;              // TODO: initialize value???
    std::set<DESCRIPTOR> descTypeSet_;  // TODO: initialize value???
    std::vector<VkDescriptorSet> descriptorSets_;
    // PUSH CONSTANT
    std::vector<VkPushConstantRange> pushConstantRanges_;
};

namespace Default {

struct PushConstant {
    glm::mat4 model;
};

// **********************
//      Triangle List Color
// **********************
class TriListColor : public Base {
   public:
    TriListColor(Pipeline::Handler &handler);

    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
    void getShaderInfoResources(CreateInfoResources &createInfoRes) override;
};

// **********************
//      Line
// **********************
class Line : public Base {
   public:
    Line(Pipeline::Handler &handler);

    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
    void getShaderInfoResources(CreateInfoResources &createInfoRes) override;
};

// **********************
//      Triangle List Texture
// **********************
class TriListTexture : public Base {
   public:
    TriListTexture(Pipeline::Handler &handler);

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
