/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Geometry.h"

#include "Deferred.h"
// HANDLERS
#include "PipelineHandler.h"

namespace Shader {

namespace Geometry {
const CreateInfo WIREFRAME_CREATE_INFO = {
    SHADER::WIREFRAME_GEOM,              //
    "Wireframe Geometry Shader",         //
    "geom.wireframe.glsl",               //
    vk::ShaderStageFlagBits::eGeometry,  //
};
const CreateInfo SILHOUETTE_CREATE_INFO = {
    SHADER::SILHOUETTE_GEOM,             //
    "Silhouette Geometry Shader",        //
    "geom.silhouette.glsl",              //
    vk::ShaderStageFlagBits::eGeometry,  //
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
      Descriptor::Base(UNIFORM::GEOMETRY_DEFAULT),           //
      Buffer::DataItem<DATA>(pData) {
    dirty = true;
}

void Default::Base::updateLine(const glm::vec4 color, const float width) {
    pData_->color = color;
    pData_->data1[0] = width;
}

void Default::Base::updateViewport(const vk::Extent2D& extent) {
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
    GRAPHICS::GEOMETRY_SILHOUETTE_DEFERRED,
    "Tessellation Bezier 4 Control Point (Deferred) Pipeline",
    {
        // This probably won't work right anymore because of the position attachment change
        SHADER::DEFERRED_MRT_COLOR_CS_VERT,
        SHADER::SILHOUETTE_GEOM,
        SHADER::DEFERRED_MRT_COLOR_FRAG,
    },
    {
        {DESCRIPTOR_SET::UNIFORM_DEFERRED_MRT, vk::ShaderStageFlagBits::eVertex},
        {DESCRIPTOR_SET::UNIFORM_GEOMETRY_DEFAULT, vk::ShaderStageFlagBits::eGeometry},
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
    assert(createInfoRes.inputAssemblyStateInfo.topology == vk::PrimitiveTopology::eTriangleList);
    createInfoRes.inputAssemblyStateInfo.topology = vk::PrimitiveTopology::eTriangleListWithAdjacency;
}

}  // namespace Geometry
}  // namespace Pipeline
