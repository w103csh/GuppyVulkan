#ifndef PIPELINE_H
#define PIPELINE_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "ConstantsAll.h"
#include "DescriptorSet.h"
#include "Handlee.h"
#include "Helpers.h"
#include "Obj3d.h"
#include "Material.h"
#include "RenderPass.h"

#include "RenderPassConstants.h"

namespace Pipeline {

struct CreateInfoVkResources;
class Handler;

void GetDefaultColorInputAssemblyInfoResources(CreateInfoVkResources &createInfoRes);
void GetDefaultTextureInputAssemblyInfoResources(CreateInfoVkResources &createInfoRes);

struct Layouts {
    VkPipelineLayout pipelineLayout;
    std::vector<VkDescriptorSetLayout> descSetLayouts;
};

// BASE

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

    inline const auto &getStatus() const { return status_; }
    inline const auto &getShaderTextReplaceInfoMap() { return shaderTextReplaceInfoMap_; }
    inline const auto &getDescriptorOffsets() { return descriptorOffsets_; }

   protected:
    Base(Pipeline::Handler &handler, const Pipeline::CreateInfo *pCreateInfo);

    // INFOS
    virtual void getBlendInfoResources(CreateInfoVkResources &createInfoRes);
    virtual void getDepthInfoResources(CreateInfoVkResources &createInfoRes);
    virtual void getDynamicStateInfoResources(CreateInfoVkResources &createInfoRes);
    virtual void getInputAssemblyInfoResources(CreateInfoVkResources &createInfoRes);
    virtual void getMultisampleStateInfoResources(CreateInfoVkResources &createInfoRes);
    virtual void getRasterizationStateInfoResources(CreateInfoVkResources &createInfoRes);
    virtual void getTesselationInfoResources(CreateInfoVkResources &createInfoRes);
    virtual void getViewportStateInfoResources(CreateInfoVkResources &createInfoRes);

    virtual void setInfo(CreateInfoVkResources &createInfoRes, VkGraphicsPipelineCreateInfo &pipelineCreateInfo);

    const std::shared_ptr<Pipeline::BindData> &getBindData(const RENDER_PASS &passType);

    // DESCRIPTOR SET
    void validatePipelineDescriptorSets();
    void prepareDescriptorSetInfo();

    void makePipelineLayouts();

    void destroy();

    STATUS status_;
    Descriptor::OffsetsMap descriptorOffsets_;
    std::map<std::set<RENDER_PASS>, Layouts> layoutsMap_;
    std::map<std::set<RENDER_PASS>, std::shared_ptr<Pipeline::BindData>> bindDataMap_;

   private:
    std::shared_ptr<Pipeline::BindData> makeBindData(const VkPipelineLayout &layout);

    // DESCRIPTOR SET
    shaderTextReplaceInfoMap shaderTextReplaceInfoMap_;
    // PUSH CONSTANT
    std::vector<VkPushConstantRange> pushConstantRanges_;
    // TEXTURE
    std::vector<uint32_t> pendingTexturesOffsets_;
};

namespace Default {

// TRIANGLE LIST COLOR
class TriListColor : public Base {
   public:
    TriListColor(Pipeline::Handler &handler) : Base(handler, &TRI_LIST_COLOR_CREATE_INFO) {}
};

// LINE
class Line : public Base {
   public:
    Line(Pipeline::Handler &handler) : Base(handler, &LINE_CREATE_INFO) {}
    // INFOS
    void getInputAssemblyInfoResources(CreateInfoVkResources &createInfoRes) override;
};

// TRIANGLE LIST TEXTURE
class TriListTexture : public Base {
   public:
    TriListTexture(Pipeline::Handler &handler) : Base(handler, &TRI_LIST_TEX_CREATE_INFO) {}
};

// CUBE
class Cube : public Base {
   public:
    Cube(Pipeline::Handler &handler) : Base(handler, &CUBE_CREATE_INFO) {}
    // INFOS
    void getDepthInfoResources(CreateInfoVkResources &createInfoRes) override;
};

}  // namespace Default

// BLINN PHONG
namespace BP {

// TEXTURE CULL NONE
class TextureCullNone : public Base {
   public:
    TextureCullNone(Pipeline::Handler &handler) : Base(handler, &TEX_CULL_NONE_CREATE_INFO) {}
    // INFOS
    void getRasterizationStateInfoResources(CreateInfoVkResources &createInfoRes) override;
};

}  // namespace BP

}  // namespace Pipeline

#endif  // !PIPELINE_H
