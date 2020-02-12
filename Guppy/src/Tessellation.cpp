/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Tessellation.h"

#include "Deferred.h"
// HANDLERS
#include "PipelineHandler.h"

namespace Uniform {
namespace Tessellation {

Default::Base::Base(const Buffer::Info&& info, DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM::TESS_DEFAULT),
      Buffer::DataItem<DATA>(pData) {
    dirty = true;
}

}  // namespace Tessellation
}  // namespace Uniform

namespace UniformDynamic {
namespace Tessellation {
namespace Phong {
Base::Base(const Buffer::Info&& info, DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM_DYNAMIC::TESS_PHONG),
      Buffer::DataItem<DATA>(pData) {
    dirty = true;
}
}  // namespace Phong
}  // namespace Tessellation
}  // namespace UniformDynamic

namespace Descriptor {
namespace Set {
namespace Tessellation {
const CreateInfo DEFAULT_CREATE_INFO = {
    DESCRIPTOR_SET::TESS_DEFAULT,
    "_DS_UNI_TESS_DEF",
    {{{0, 0}, {UNIFORM::TESS_DEFAULT}}},
};
const CreateInfo PHONG_CREATE_INFO = {
    DESCRIPTOR_SET::TESS_PHONG,
    "_DS_TESS_PHONG",
    {{{0, 0}, {UNIFORM_DYNAMIC::TESS_PHONG}}},
};
}  // namespace Tessellation
}  // namespace Set
}  // namespace Descriptor

namespace Pipeline {
namespace Tessellation {

Base::Base(Handler& handler, const CreateInfo* pCreateInfo, const uint32_t patchControlPoints, bool isDeferred)
    : Graphics(handler, pCreateInfo), IS_DEFERRED(isDeferred), PATCH_CONTROL_POINTS(patchControlPoints) {}

void Base::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED)
        Deferred::GetBlendInfoResources(createInfoRes);
    else
        Graphics::getBlendInfoResources(createInfoRes);
}

void Base::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    Graphics::getInputAssemblyInfoResources(createInfoRes);
    createInfoRes.inputAssemblyStateInfo.topology = vk::PrimitiveTopology::ePatchList;
}

void Base::getShaderStageInfoResources(CreateInfoResources& createInfoRes) {
    // This doesn't work right...

    // createInfoRes.specializationMapEntries.push_back({{}});

    //// Use specialization constants to pass number of samples to the shader (used for MSAA resolve)
    // createInfoRes.specializationMapEntries.back().back().constantID = 0;
    // createInfoRes.specializationMapEntries.back().back().offset = 0;
    // createInfoRes.specializationMapEntries.back().back().size = sizeof(PATCH_CONTROL_POINTS);

    // createInfoRes.specializationInfo.push_back({});
    // createInfoRes.specializationInfo.back().mapEntryCount =
    //    static_cast<uint32_t>(createInfoRes.specializationMapEntries.back().size());
    // createInfoRes.specializationInfo.back().pMapEntries = createInfoRes.specializationMapEntries.back().data();
    // createInfoRes.specializationInfo.back().dataSize = sizeof(handler().shell().context().samples);
    // createInfoRes.specializationInfo.back().pData = &handler().shell().context().samples;

    // assert(createInfoRes.shaderStageInfos.size() == 4 &&
    //       createInfoRes.shaderStageInfos[1].stage == vk::ShaderStageFlagBits::eTessellationControl &&
    //       createInfoRes.shaderStageInfos[2].stage == vk::ShaderStageFlagBits::eTessellationEvaluation);
    //// Add the specialization to the tessellation shader infos.
    // createInfoRes.shaderStageInfos[1].pSpecializationInfo = &createInfoRes.specializationInfo.back();
    // createInfoRes.shaderStageInfos[2].pSpecializationInfo = &createInfoRes.specializationInfo.back();
}

void Base::getTesselationInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.useTessellationInfo = true;
    createInfoRes.tessellationStateInfo = vk::PipelineTessellationStateCreateInfo{};
    createInfoRes.tessellationStateInfo.patchControlPoints = PATCH_CONTROL_POINTS;
}

// BEZIER 4
const Pipeline::CreateInfo BEZIER_4_CREATE_INFO = {
    GRAPHICS::TESS_BEZIER_4_DEFERRED,
    "Tessellation Bezier 4 Control Point (Deferred) Pipeline",
    {
        SHADER::TESS_COLOR_VERT,
        SHADER::BEZIER_4_TESC,
        SHADER::BEZIER_4_TESE,
        SHADER::DEFERRED_MRT_COLOR_FRAG,
    },
    {
        DESCRIPTOR_SET::UNIFORM_DEFERRED_MRT,
        DESCRIPTOR_SET::TESS_DEFAULT,
    },
    {},
    {PUSH_CONSTANT::DEFERRED},
};
Bezier4Deferred::Bezier4Deferred(Handler& handler) : Base(handler, &BEZIER_4_CREATE_INFO, 4) {}

// PHONG TRIANGLE (DEFERRED)
const Pipeline::CreateInfo PHONG_TRI_DEFERRED_CREATE_INFO = {
    GRAPHICS::TESS_PHONG_TRI_COLOR_DEFERRED,
    "Tessellation Phong Triangle Color (Deferred) Pipeline",
    {
        SHADER::TESS_COLOR_VERT,
        SHADER::PHONG_TRI_COLOR_TESC,
        SHADER::PHONG_TRI_COLOR_TESE,
        SHADER::DEFERRED_MRT_COLOR_FRAG,
    },
    {DESCRIPTOR_SET::UNIFORM_DEFERRED_MRT, DESCRIPTOR_SET::TESS_PHONG},
    {},
    {PUSH_CONSTANT::DEFERRED},
};
// PHONG TRIANGLE COLOR (DEFERRED)
PhongTriColorDeferred::PhongTriColorDeferred(Handler& handler) : Base(handler, &PHONG_TRI_DEFERRED_CREATE_INFO, 3) {}

// PHONG TRIANGLE COLOR WIREFRAME (DEFERRED)
std::unique_ptr<Pipeline::Base> MakePhongTriColorWireframeDeferred(Pipeline::Handler& handler) {
    CreateInfo info = PHONG_TRI_DEFERRED_CREATE_INFO;
    info.name = "Tessellation Phong Triangle Color Wireframe (Deferred) Pipeline";
    info.type = GRAPHICS::TESS_PHONG_TRI_COLOR_WF_DEFERRED;
    return std::make_unique<PhongTriColorWireframeDeferred>(handler, &info);
}
PhongTriColorWireframeDeferred::PhongTriColorWireframeDeferred(Handler& handler, const CreateInfo* pCreateInfo)
    : PhongTriColorDeferred(handler, pCreateInfo) {}

void PhongTriColorWireframeDeferred::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    Base::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.polygonMode = vk::PolygonMode::eLine;
}

}  // namespace Tessellation
}  // namespace Pipeline
