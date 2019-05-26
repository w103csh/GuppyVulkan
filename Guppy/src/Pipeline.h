#ifndef PIPELINE_H
#define PIPELINE_H

#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Handlee.h"
#include "Helpers.h"
#include "Obj3d.h"
#include "Material.h"
#include "RenderPass.h"

namespace Pipeline {

struct CreateInfoResources;
class Handler;

void GetDefaultColorInputAssemblyInfoResources(CreateInfoResources &createInfoRes);
void GetDefaultTextureInputAssemblyInfoResources(CreateInfoResources &createInfoRes);

// **********************
//      BASE
// **********************

class Base : public Handlee<Pipeline::Handler> {
    friend class Handler;

   public:
    virtual ~Base() = default;

    const VkPipelineBindPoint BIND_POINT;
    const std::vector<DESCRIPTOR_SET> DESCRIPTOR_SET_TYPES;
    const std::string NAME;
    const std::vector<PUSH_CONSTANT> PUSH_CONSTANT_TYPES;
    const std::set<SHADER> SHADER_TYPES;
    const PIPELINE TYPE;

    virtual void init();
    virtual void updateStatus();

    inline const STATUS &getStatus() const { return status_; }
    inline const VkPipelineLayout &getLayout() const { return layout_; }
    inline const VkPipeline &getPipeline() const { return pipeline_; }
    inline uint32_t getSubpassId() const { return subpassId_; }

    // DESCRIPTOR SET
    Shader::descSetMacroSlotMap getDescSetMacroSlotMap() { return descSetMacroSlotMap_; }

   protected:
    Base(Pipeline::Handler &handler, const PIPELINE &&type, const VkPipelineBindPoint &&bindPoint, const std::string &&name,
         const std::set<SHADER> &&shaderTypes, const std::vector<PUSH_CONSTANT> &&pushConstantTypes = {},
         std::vector<DESCRIPTOR_SET> &&descriptorSets = {});

    // INFOS
    virtual void getBlendInfoResources(CreateInfoResources &createInfoRes);
    virtual void getDepthInfoResources(CreateInfoResources &createInfoRes);
    virtual void getDynamicStateInfoResources(CreateInfoResources &createInfoRes);
    virtual void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes);
    virtual void getMultisampleStateInfoResources(CreateInfoResources &createInfoRes);
    virtual void getRasterizationStateInfoResources(CreateInfoResources &createInfoRes);
    virtual void getShaderInfoResources(CreateInfoResources &createInfoRes);
    virtual void getTesselationInfoResources(CreateInfoResources &createInfoRes);
    virtual void getViewportStateInfoResources(CreateInfoResources &createInfoRes);

    virtual const VkPipeline &create(const VkPipelineCache &cache, CreateInfoResources &createInfoRes,
                                     VkGraphicsPipelineCreateInfo &pipelineCreateInfo,
                                     const VkPipeline &basePipelineHandle = VK_NULL_HANDLE,
                                     const int32_t basePipelineIndex = -1);

    // DESCRIPTOR SET
    void validatePipelineDescriptorSets();
    // SHADER / DESCRIPTOR SET
    std::vector<VkDescriptorSetLayout> prepareDescSetInfo();

    void createPipelineLayout();

    void destroy();

    STATUS status_;
    VkPipelineLayout layout_;
    VkPipeline pipeline_;

   private:
    uint32_t subpassId_;
    // DESCRIPTOR SET
    Shader::descSetMacroSlotMap descSetMacroSlotMap_;
    // PUSH CONSTANT
    std::vector<VkPushConstantRange> pushConstantRanges_;
    // TEXTURE
    std::vector<uint32_t> pendingTexturesOffsets_;
};

namespace Default {

struct PushConstant {
    glm::mat4 model;
};

// TRIANGLE LIST COLOR
class TriListColor : public Base {
   public:
    TriListColor(Pipeline::Handler &handler);
};

// LINE
class Line : public Base {
   public:
    Line(Pipeline::Handler &handler);
    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
};

// TRIANGLE LIST TEXTURE
class TriListTexture : public Base {
   public:
    TriListTexture(Pipeline::Handler &handler);
};

// CUBE
class Cube : public Base {
   public:
    Cube(Pipeline::Handler &handler);
    // INFOS
    void getDepthInfoResources(CreateInfoResources &createInfoRes) override;
};

}  // namespace Default

namespace BP {

// BLINN PHONG TEXTURE CULL NONE
class TextureCullNone : public Base {
   public:
    TextureCullNone(Pipeline::Handler &handler);
    // INFOS
    void getRasterizationStateInfoResources(CreateInfoResources &createInfoRes) override;
};
}  // namespace BP

}  // namespace Pipeline

#endif  // !PIPELINE_H
