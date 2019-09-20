
#include "Tessellation.h"

#include "Deferred.h"
// HANDLERS
#include "PipelineHandler.h"

namespace Shader {
struct CreateInfo;
namespace Tessellation {

const CreateInfo COLOR_VERT_CREATE_INFO = {
    SHADER::TESS_COLOR_VERT,
    "Tessellation Color Vertex Shader",
    "vert.color.tess.passthrough.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,
};
const CreateInfo BEZIER_4_TESC_CREATE_INFO = {
    SHADER::BEZIER_4_TESC,
    "Bezier 4 Control Point Tesselation Control Shader",
    "tesc.bezier4.glsl",
    VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
};
const CreateInfo BEZIER_4_TESE_CREATE_INFO = {
    SHADER::BEZIER_4_TESE,
    "Bezier 4 Control Point Tesselation Evaluation Shader",
    "tese.bezier4.glsl",
    VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
};
const CreateInfo TRIANGLE_TESC_CREATE_INFO = {
    SHADER::TRIANGLE_TESC,
    "Triangle Tesselation Control Shader",
    "tesc.triangle.glsl",
    VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
};
const CreateInfo TRIANGLE_TESE_CREATE_INFO = {
    SHADER::TRIANGLE_TESE,
    "Triangle Tesselation Evaluation Shader",
    "tese.triangle.glsl",
    VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
};

}  // namespace Tessellation
}  // namespace Shader

namespace Uniform {
namespace Tessellation {

Default::Base::Base(const Buffer::Info&& info, DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::DataItem<DATA>(pData)                          //
{
    dirty = true;
}

}  // namespace Tessellation
}  // namespace Uniform

namespace Descriptor {
namespace Set {
namespace Tessellation {

const CreateInfo DEFAULT_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_TESSELLATION_DEFAULT,
    "_DS_UNI_TESS_DEF",
    {
        {{0, 0}, {UNIFORM::TESSELLATION_DEFAULT}},
    },
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
        Deferred::GetDefaultBlendInfoResources(createInfoRes);
    else
        Graphics::getBlendInfoResources(createInfoRes);
}

void Base::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    Graphics::getInputAssemblyInfoResources(createInfoRes);
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
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
    //       createInfoRes.shaderStageInfos[1].stage == VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT &&
    //       createInfoRes.shaderStageInfos[2].stage == VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT);
    //// Add the specialization to the tessellation shader infos.
    // createInfoRes.shaderStageInfos[1].pSpecializationInfo = &createInfoRes.specializationInfo.back();
    // createInfoRes.shaderStageInfos[2].pSpecializationInfo = &createInfoRes.specializationInfo.back();
}

void Base::getTesselationInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.tessellationStateInfo = {};
    createInfoRes.tessellationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    createInfoRes.tessellationStateInfo.pNext = nullptr;
    createInfoRes.tessellationStateInfo.flags = 0;
    createInfoRes.tessellationStateInfo.patchControlPoints = PATCH_CONTROL_POINTS;
}

// BEZIER 4
const Pipeline::CreateInfo BEZIER_4_CREATE_INFO = {
    PIPELINE::TESSELLATION_BEZIER_4_DEFERRED,
    "Tessellation Bezier 4 Control Point (Deferred) Pipeline",
    {
        SHADER::TESS_COLOR_VERT,
        SHADER::BEZIER_4_TESC,
        SHADER::BEZIER_4_TESE,
        SHADER::DEFERRED_MRT_COLOR_FRAG,
    },
    {
        DESCRIPTOR_SET::UNIFORM_DEFERRED_MRT,
        DESCRIPTOR_SET::UNIFORM_TESSELLATION_DEFAULT,
    },
    {},
    {PUSH_CONSTANT::DEFERRED},
};
Bezier4Deferred::Bezier4Deferred(Handler& handler) : Base(handler, &BEZIER_4_CREATE_INFO, 4) {}

// TRIANGLE
const Pipeline::CreateInfo TRIANGLE_DEFERRED_CREATE_INFO = {
    PIPELINE::TESSELLATION_TRIANGLE_DEFERRED,
    "Tessellation Triangle (Deferred) Pipeline",
    {
        SHADER::TESS_COLOR_VERT,
        SHADER::TRIANGLE_TESC,
        SHADER::TRIANGLE_TESE,
        SHADER::DEFERRED_MRT_COLOR_FRAG,
    },
    {
        DESCRIPTOR_SET::UNIFORM_DEFERRED_MRT,
        DESCRIPTOR_SET::UNIFORM_TESSELLATION_DEFAULT,
    },
    Descriptor::OffsetsMap::Type{
        {UNIFORM::TESSELLATION_DEFAULT, {1}},
    },
    {PUSH_CONSTANT::DEFERRED},
};
TriangleDeferred::TriangleDeferred(Handler& handler) : Base(handler, &TRIANGLE_DEFERRED_CREATE_INFO, 6) {}

void TriangleDeferred::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    Base::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_LINE;
    // createInfoRes.rasterizationStateInfo.cullMode = VK_CULL_MODE_NONE;
    createInfoRes.rasterizationStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
}

}  // namespace Tessellation
}  // namespace Pipeline