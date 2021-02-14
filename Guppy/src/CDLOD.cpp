/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "CDLOD.h"

#include <Common/Helpers.h>

// HANDLERS
#include "PipelineHandler.h"

// SHADER
namespace Shader {
namespace CDLOD {
const CreateInfo VERT_CREATE_INFO = {
    SHADER::OCEAN_VERT,  //
    "CDLOD Vertex Shader",
    "vert.cdlod.glsl",
    vk::ShaderStageFlagBits::eVertex,
};
}  // namespace CDLOD
}  // namespace Shader

// UNIFORM
namespace Uniform {
namespace CDLOD {
namespace QuadTree {
Base::Base(const Buffer::Info&& info, DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Descriptor::Base(UNIFORM::CDLOD_QUAD_TREE),
      Buffer::DataItem<DATA>(pData) {
    dirty = true;
}
}  // namespace QuadTree
}  // namespace CDLOD
}  // namespace Uniform

//// UNIFORM DYNAMIC
// namespace UniformDynamic {
// namespace CDLOD {
// namespace Grid {
// Base::Base(const Buffer::Info&& info, DATA* pData, const Buffer::CreateInfo* pCreateInfo)
//    : Buffer::Item(std::forward<const Buffer::Info>(info)),
//      Descriptor::Base(UNIFORM_DYNAMIC::OCEAN),
//      Buffer::PerFramebufferDataItem<DATA>(pData) {
//    setData();
//}
// void Base::updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) {
//    assert(false);
//    setData(frameIndex);
//}
//}  // namespace Grid
//}  // namespace CDLOD
//}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
const CreateInfo CDLOD_DEFAULT_CREATE_INFO = {
    DESCRIPTOR_SET::CDLOD_DEFAULT,
    "_DS_CDLOD",
    {
        {{0, 0}, {UNIFORM::CDLOD_QUAD_TREE}},
    },
};
}  // namespace Set
}  // namespace Descriptor

namespace Pipeline {
namespace CDLOD {
// MRT (COLOR WIREFRAME)
const Pipeline::CreateInfo WF_CREATE_INFO = {
    GRAPHICS::CDLOD_WF_DEFERRED,
    "CDLOD Deferred Wireframe Pipeline",
    {
        SHADER::VERT_COLOR,
        SHADER::DEFERRED_MRT_COLOR_FRAG,
    },
    {
        {DESCRIPTOR_SET::UNIFORM_DEFAULT, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)},
        {DESCRIPTOR_SET::CDLOD_DEFAULT, (vk::ShaderStageFlagBits::eVertex)},
    },
    {},
    {PUSH_CONSTANT::CDLOD},
};
Wireframe::Wireframe(Handler& handler) : MRTColor(handler, &WF_CREATE_INFO) {}

void Wireframe::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    MRTColor::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.polygonMode = vk::PolygonMode::eLine;
    createInfoRes.rasterizationStateInfo.cullMode = vk::CullModeFlagBits::eNone;
}
}  // namespace CDLOD
}  // namespace Pipeline
