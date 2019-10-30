
#include "Geometry.h"

#include "Deferred.h"
// HANDLERS
#include "PipelineHandler.h"

namespace Shader {

namespace Geometry {
const CreateInfo WIREFRAME_CREATE_INFO = {
    SHADER::WIREFRAME_GEOM,        //
    "Wireframe Geometry Shader",   //
    "geom.wireframe.glsl",         //
    VK_SHADER_STAGE_GEOMETRY_BIT,  //
};
const CreateInfo SILHOUETTE_CREATE_INFO = {
    SHADER::SILHOUETTE_GEOM,       //
    "Silhouette Geometry Shader",  //
    "geom.silhouette.glsl",        //
    VK_SHADER_STAGE_GEOMETRY_BIT,  //
};
}  // namespace Geometry

namespace Link {
namespace Geometry {
const CreateInfo WIREFRAME_CREATE_INFO = {
    SHADER_LINK::GEOMETRY_FRAG,  //
    "link.frag.geometry.glsl",   //
};
}  // namespace Geometry
}  // namespace Link

}  // namespace Shader

namespace Uniform {
namespace Geometry {

Default::Base::Base(const Buffer::Info&& info, DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::DataItem<DATA>(pData),                         //
      Descriptor::Base(UNIFORM::GEOMETRY_DEFAULT)            //
{
    dirty = true;
}

void Default::Base::updateLine(const glm::vec4 color, const float width) {
    pData_->color = color;
    pData_->data1[0] = width;
}

void Default::Base::updateViewport(const VkExtent2D& extent) {
    float w2 = extent.width / 2.0f;
    float h2 = extent.height / 2.0f;
    pData_->viewport = glm::mat4(glm::vec4(w2, 0.0f, 0.0f, 0.0f), glm::vec4(0.0f, h2, 0.0f, 0.0f),
                                 glm::vec4(0.0f, 0.0f, 1.0f, 0.0f), glm::vec4(w2 + 0, h2 + 0, 0.0f, 1.0f));
    dirty = true;
}

}  // namespace Geometry
}  // namespace Uniform

namespace Descriptor {
namespace Set {
namespace Geometry {

const CreateInfo DEFAULT_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_GEOMETRY_DEFAULT,
    "_DS_UNI_GEOM_DEF",
    {
        {{0, 0}, {UNIFORM::GEOMETRY_DEFAULT}},
    },
};

}  // namespace Geometry
}  // namespace Set
}  // namespace Descriptor

namespace Pipeline {
namespace Geometry {

const Pipeline::CreateInfo SILHOUETTE_CREATE_INFO = {
    PIPELINE::GEOMETRY_SILHOUETTE_DEFERRED,
    "Tessellation Bezier 4 Control Point (Deferred) Pipeline",
    {
        SHADER::DEFERRED_MRT_COLOR_CS_VERT,
        SHADER::SILHOUETTE_GEOM,
        SHADER::DEFERRED_MRT_COLOR_FRAG,
    },
    {
        DESCRIPTOR_SET::UNIFORM_DEFERRED_MRT,
        DESCRIPTOR_SET::UNIFORM_GEOMETRY_DEFAULT,
    },
    {},
    {PUSH_CONSTANT::DEFERRED},
};
Silhouette::Silhouette(Handler& handler, bool isDeferred)
    : Graphics(handler, &SILHOUETTE_CREATE_INFO), IS_DEFERRED(isDeferred) {}

void Silhouette::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED)
        Deferred::GetBlendInfoResources(createInfoRes);
    else
        Graphics::getBlendInfoResources(createInfoRes);
}

void Silhouette::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    Graphics::getInputAssemblyInfoResources(createInfoRes);
    assert(createInfoRes.inputAssemblyStateInfo.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
}

}  // namespace Geometry
}  // namespace Pipeline
