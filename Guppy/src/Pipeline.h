/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef PIPELINE_H
#define PIPELINE_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "ConstantsAll.h"
#include "DescriptorSet.h"
#include "Handlee.h"
#include "Material.h"
#include "RenderPass.h"

#include "RenderPassConstants.h"

namespace Pipeline {

struct CreateInfoResources;
class Handler;

void GetDefaultColorInputAssemblyInfoResources(CreateInfoResources &createInfoRes);
void GetDefaultTextureInputAssemblyInfoResources(CreateInfoResources &createInfoRes);
void GetDefaultScreenQuadInputAssemblyInfoResources(CreateInfoResources &createInfoRes);

struct Layouts {
    vk::PipelineLayout pipelineLayout;
    std::vector<vk::DescriptorSetLayout> descSetLayouts;
};

// BASE

class Base : public Handlee<Pipeline::Handler> {
    friend class Handler;

   public:
    virtual ~Base() = default;

    // INFOS
    virtual void getShaderStageInfoResources(CreateInfoResources &createInfoRes) {}

    const vk::PipelineBindPoint BIND_POINT;
    const std::vector<DESCRIPTOR_SET> DESCRIPTOR_SET_TYPES;
    const std::string NAME;
    const std::vector<PUSH_CONSTANT> PUSH_CONSTANT_TYPES;
    const PIPELINE TYPE;

    virtual void init();
    virtual void updateStatus();

    constexpr const auto &getShaderTypes() const { return shaderTypes_; }
    constexpr const auto &getStatus() const { return status_; }
    constexpr const auto &getShaderTextReplaceInfoMap() { return shaderTextReplaceInfoMap_; }
    constexpr const auto &getDescriptorOffsets() { return descriptorOffsets_; }

    // I can't think of anything better atm.
    virtual void setInfo(CreateInfoResources &createInfoRes, vk::GraphicsPipelineCreateInfo *pGraphicsInfo,
                         vk::ComputePipelineCreateInfo *pComputeInfo) = 0;

   protected:
    Base(Pipeline::Handler &handler, const vk::PipelineBindPoint &&bindPoint, const Pipeline::CreateInfo *pCreateInfo);

    const std::shared_ptr<Pipeline::BindData> &getBindData(const PASS &passType);

    // SHADER
    void replaceShaderType(const SHADER type, const SHADER replaceWithType);

    // DESCRIPTOR SET
    bool checkTextureStatus(const std::string &id);
    void validatePipelineDescriptorSets();
    void prepareDescriptorSetInfo();

    void makePipelineLayouts();

    void destroy();

    STATUS status_;
    Descriptor::OffsetsMap descriptorOffsets_;
    std::map<std::set<PASS>, Layouts> layoutsMap_;
    std::map<std::set<PASS>, std::shared_ptr<Pipeline::BindData>> bindDataMap_;

   private:
    std::shared_ptr<Pipeline::BindData> makeBindData(const vk::PipelineLayout &layout);

    bool isInitialized_;

    // SHADER
    std::set<SHADER> shaderTypes_;
    // DESCRIPTOR SET
    shaderTextReplaceInfoMap shaderTextReplaceInfoMap_;
    // PUSH CONSTANT
    std::vector<vk::PushConstantRange> pushConstantRanges_;
    // TEXTURE
    std::vector<uint32_t> pendingTexturesOffsets_;
};

// COMPUTE

class Compute : public Base {
   public:
    void setInfo(CreateInfoResources &createInfoRes, vk::GraphicsPipelineCreateInfo *pGraphicsInfo,
                 vk::ComputePipelineCreateInfo *pComputeInfo) override final;

    virtual_inline auto getLocalSize() const { return localSize_; }

   protected:
    Compute(Pipeline::Handler &handler, const Pipeline::CreateInfo *pCreateInfo);

   private:
    glm::uvec3 localSize_;
};

// GRAPHICS

class Graphics : public Base {
   public:
    void setInfo(CreateInfoResources &createInfoRes, vk::GraphicsPipelineCreateInfo *pGraphicsInfo,
                 vk::ComputePipelineCreateInfo *pComputeInfo) override final;

    Graphics(Pipeline::Handler &handler, const Pipeline::CreateInfo *pCreateInfo);

   protected:
    virtual void getBlendInfoResources(CreateInfoResources &createInfoRes);
    virtual void getDepthInfoResources(CreateInfoResources &createInfoRes);
    virtual void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes);
    virtual void getMultisampleStateInfoResources(CreateInfoResources &createInfoRes);
    virtual void getRasterizationStateInfoResources(CreateInfoResources &createInfoRes);
    virtual void getTesselationInfoResources(CreateInfoResources &createInfoRes);
    virtual void getViewportStateInfoResources(CreateInfoResources &createInfoRes);

   private:
    void getDynamicStateInfoResources(CreateInfoResources &createInfoRes);
};

// DEFAULT
namespace Default {

// TRIANGLE LIST COLOR
class TriListColor : public Graphics {
   public:
    TriListColor(Pipeline::Handler &handler) : Graphics(handler, &TRI_LIST_COLOR_CREATE_INFO) {}
    TriListColor(Pipeline::Handler &handler, const CreateInfo *pCreateInfo) : Graphics(handler, pCreateInfo) {}
};

// LINE
class Line : public Graphics {
   public:
    Line(Pipeline::Handler &handler) : Graphics(handler, &LINE_CREATE_INFO) {}
    Line(Pipeline::Handler &handler, const CreateInfo *pCreateInfo) : Graphics(handler, pCreateInfo) {}
    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
};

// POINT
class Point : public Graphics {
   public:
    Point(Pipeline::Handler &handler) : Graphics(handler, &POINT_CREATE_INFO) {}
    Point(Pipeline::Handler &handler, const CreateInfo *pCreateInfo) : Graphics(handler, pCreateInfo) {}
    // INFOS
    void getInputAssemblyInfoResources(CreateInfoResources &createInfoRes) override;
};

// TRIANGLE LIST TEXTURE
class TriListTexture : public Graphics {
   public:
    TriListTexture(Pipeline::Handler &handler) : Graphics(handler, &TRI_LIST_TEX_CREATE_INFO) {}
    TriListTexture(Pipeline::Handler &handler, const CreateInfo *pCreateInfo) : Graphics(handler, pCreateInfo) {}
};

// CUBE
class Cube : public Graphics {
   public:
    Cube(Pipeline::Handler &handler) : Graphics(handler, &CUBE_CREATE_INFO) {}
    // INFOS
    void getDepthInfoResources(CreateInfoResources &createInfoRes) override;
};

// CUBE MAP

// COLOR
std::unique_ptr<Pipeline::Base> MakeCubeMapColor(Pipeline::Handler &handler);
class TriListColorCube : public TriListColor {
   public:
    TriListColorCube(Pipeline::Handler &handler, const CreateInfo *pCreateInfo);
    void getRasterizationStateInfoResources(CreateInfoResources &createInfoRes) override;
};
// LINE
std::unique_ptr<Pipeline::Base> MakeCubeMapLine(Pipeline::Handler &handler);
class LineCube : public Line {
   public:
    LineCube(Pipeline::Handler &handler, const CreateInfo *pCreateInfo);
    void getRasterizationStateInfoResources(CreateInfoResources &createInfoRes) override;
};
// POINT
std::unique_ptr<Pipeline::Base> MakeCubeMapPoint(Pipeline::Handler &handler);
class PointCube : public Point {
   public:
    PointCube(Pipeline::Handler &handler, const CreateInfo *pCreateInfo);
    void getRasterizationStateInfoResources(CreateInfoResources &createInfoRes) override;
};
// TEXTURE
std::unique_ptr<Pipeline::Base> MakeCubeMapTexture(Pipeline::Handler &handler);
class TriListTextureCube : public TriListTexture {
   public:
    TriListTextureCube(Pipeline::Handler &handler, const CreateInfo *pCreateInfo);
    void getRasterizationStateInfoResources(CreateInfoResources &createInfoRes) override;
};

}  // namespace Default

// BLINN PHONG
namespace BP {

// TEXTURE CULL NONE
class TextureCullNone : public Graphics {
   public:
    TextureCullNone(Pipeline::Handler &handler) : Graphics(handler, &TEX_CULL_NONE_CREATE_INFO) {}
    // INFOS
    void getRasterizationStateInfoResources(CreateInfoResources &createInfoRes) override;
};

}  // namespace BP

}  // namespace Pipeline

#endif  // !PIPELINE_H
