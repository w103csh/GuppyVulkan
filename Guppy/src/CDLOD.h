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
#include "Deferred.h"
#include "Pipeline.h"

// SHADER
namespace Shader {
namespace CDLOD {
extern const CreateInfo VERT_CREATE_INFO;
}  // namespace CDLOD
}  // namespace Shader

// UNIFORM
namespace Uniform {
namespace CDLOD {
namespace QuadTree {
using DATA = CDLODRenderer::PerQuadTreeData;
class Base : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData);
    auto& getData() { return *pData_; }
};
}  // namespace QuadTree
}  // namespace CDLOD
}  // namespace Uniform

namespace CDLOD {
using PushConstant = CDLODRendererBatchInfo::PerDrawData;
}  // namespace Deferred

//// UNIFORM DYNAMIC
// namespace UniformDynamic {
// namespace CDLOD {
// namespace Grid {
// using DATA = CDLODRendererBatchInfo::UniformDynamicData;
// class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
//   public:
//    Base(const Buffer::Info&& info, DATA* pData, const Buffer::CreateInfo* pCreateInfo);
//    void updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) override;
//};
// using Manager = Descriptor::Manager<Descriptor::Base, Base, std::shared_ptr>;
//}  // namespace Grid
//}  // namespace CDLOD
//}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
extern const CreateInfo CDLOD_DEFAULT_CREATE_INFO;
}  // namespace Set
}  // namespace Descriptor

namespace Pipeline {
class Handler;
namespace CDLOD {
// MRT (COLOR WIREFRAME)
class Wireframe : public Deferred::MRTColor {
   public:
    Wireframe(Handler& handler);

   private:
    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
};
}  // namespace CDLOD
}  // namespace Pipeline

#endif  //! CDLOD_H