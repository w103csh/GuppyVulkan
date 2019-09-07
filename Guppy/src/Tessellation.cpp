
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

}  // namespace Tessellation
}  // namespace Shader

namespace Uniform {
namespace Tessellation {
namespace Bezier {

Base::Base(const Buffer::Info&& info, DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::DataItem<DATA>(pData)                          //
{
    dirty = true;
}

}  // namespace Bezier
}  // namespace Tessellation
}  // namespace Uniform

namespace Descriptor {
namespace Set {
namespace Tessellation {

const CreateInfo BEZIER_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_BEZIER,
    "_DS_UNI_BEZ",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM::BEZIER}},
    },
};

}  // namespace Tessellation
}  // namespace Set
}  // namespace Descriptor

namespace Pipeline {
namespace Tessellation {

// BEZIER 4
const Pipeline::CreateInfo BEZIER_4_CREATE_INFO = {
    PIPELINE::DEFERRED_BEZIER_4,
    "Deferred Bezier 4 Control Point Pipeline",
    {
        SHADER::TESS_COLOR_VERT,
        SHADER::BEZIER_4_TESC,
        SHADER::BEZIER_4_TESE,
        SHADER::DEFERRED_MRT_COLOR_FRAG,
    },
    {DESCRIPTOR_SET::UNIFORM_BEZIER},
};
DeferredBezier4::DeferredBezier4(Handler& handler) : Graphics(handler, &BEZIER_4_CREATE_INFO) {}

void DeferredBezier4::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    Deferred::GetDefaultBlendInfoResources(createInfoRes);  //
}

void DeferredBezier4::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    Pipeline::GetDefaultColorInputAssemblyInfoResources(createInfoRes);
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
}

void DeferredBezier4::getTesselationInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.tessellationStateInfo = {};
    createInfoRes.tessellationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    createInfoRes.tessellationStateInfo.pNext = nullptr;
    createInfoRes.tessellationStateInfo.flags = 0;
    createInfoRes.tessellationStateInfo.patchControlPoints = 4;
}

}  // namespace Tessellation
}  // namespace Pipeline