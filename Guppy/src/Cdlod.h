/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef CDLOD_H
#define CDLOD_H

#include <CDLOD/CDLODQuadTree.h>
#include <CDLOD/CDLODRenderer.h>

#include "BufferItem.h"
#include "Descriptor.h"
#include "DescriptorManager.h"
#include "Deferred.h"
#include "Pipeline.h"

// SHADER
namespace Shader {
namespace Link {
namespace Cdlod {
extern const CreateInfo CREATE_INFO;
}  // namespace Cdlod
}  // namespace Link
namespace Cdlod {
extern const CreateInfo VERT_CREATE_INFO;
extern const CreateInfo VERT_TEX_CREATE_INFO;
}  // namespace Cdlod
}  // namespace Shader

// UNIFORM
namespace UniformDynamic {
namespace Cdlod {
namespace QuadTree {
using DATA = CDLODRenderer::PerQuadTreeData;
class Base : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData);
    auto& getData() { return *pData_; }
};
using Manager = Descriptor::Manager<Descriptor::Base, Base, std::shared_ptr>;
}  // namespace QuadTree
}  // namespace Cdlod
}  // namespace UniformDynamic

// PUSH CONSTANT
namespace Cdlod {
using PushConstant = CDLODRendererBatchInfo::PerDrawData;
}  // namespace Cdlod

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
extern const CreateInfo CDLOD_DEFAULT_CREATE_INFO;
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {
class Handler;
namespace Cdlod {

static void GetCdlodInputAssemblyInfoResource(Pipeline::CreateInfoResources& createInfoRes);

class Wireframe : public Deferred::MRTColor {
   public:
    Wireframe(Handler& handler);

   private:
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
};

class Texture : public Deferred::MRTTexture {
   public:
    Texture(Handler& handler);

   private:
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
};

}  // namespace Cdlod
}  // namespace Pipeline

#endif  //! CDLOD_H