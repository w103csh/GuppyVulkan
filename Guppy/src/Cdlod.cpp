/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Cdlod.h"

#include <Common/Helpers.h>

// HANDLERS
#include "PipelineHandler.h"

// SHADER
namespace Shader {
namespace Link {
namespace Cdlod {
const CreateInfo CREATE_INFO = {
    SHADER_LINK::CDLOD,
    "link.cdlod.glsl",
};
}  // namespace Cdlod
}  // namespace Link
namespace Cdlod {
const CreateInfo VERT_CREATE_INFO = {
    SHADER::CDLOD_VERT,
    "Cdlod Vertex Shader",
    "vert.cdlod.glsl",
    vk::ShaderStageFlagBits::eVertex,  //
    {SHADER_LINK::CDLOD},
};
const CreateInfo VERT_TEX_CREATE_INFO = {
    SHADER::CDLOD_TEX_VERT,
    "Cdlod Texture Vertex Shader",
    "vert.cdlod.texture.glsl",
    vk::ShaderStageFlagBits::eVertex,  //
    {SHADER_LINK::CDLOD},
};
}  // namespace Cdlod
}  // namespace Shader

// UNIFORM
namespace Uniform {
namespace Cdlod {
namespace QuadTree {
Base::Base(const Buffer::Info&& info, DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Descriptor::Base(UNIFORM::CDLOD_QUAD_TREE),
      Buffer::DataItem<DATA>(pData) {
    dirty = true;
}
}  // namespace QuadTree
}  // namespace Cdlod
}  // namespace Uniform

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
const CreateInfo CDLOD_DEFAULT_CREATE_INFO = {
    DESCRIPTOR_SET::CDLOD_DEFAULT,
    "_DS_CDLOD",
    {{{0, 0}, {UNIFORM::CDLOD_QUAD_TREE}}},
};
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {
namespace Cdlod {

void GetCdlodInputAssemblyInfoResource(Pipeline::CreateInfoResources& createInfoRes) {
    {  // description
        const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
        createInfoRes.bindDescs.push_back({});
        createInfoRes.bindDescs.back().binding = BINDING;
        createInfoRes.bindDescs.back().stride = sizeof(glm::vec3);
        createInfoRes.bindDescs.back().inputRate = vk::VertexInputRate::eVertex;

        // position
        createInfoRes.attrDescs.push_back({});
        createInfoRes.attrDescs.back().binding = BINDING;
        createInfoRes.attrDescs.back().location = 0;
        createInfoRes.attrDescs.back().format = vk::Format::eR32G32Sfloat;  // vec2
        createInfoRes.attrDescs.back().offset = 0;
    }

    // bindings
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = createInfoRes.bindDescs.data();
    // attributes
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = vk::PipelineInputAssemblyStateCreateInfo{};
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = vk::PrimitiveTopology::eTriangleList;
}

const Pipeline::CreateInfo WF_CREATE_INFO = {
    GRAPHICS::CDLOD_WF_DEFERRED,
    "Cdlod Deferred Wireframe Pipeline",
    {
        SHADER::CDLOD_VERT,
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
void Wireframe::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    GetCdlodInputAssemblyInfoResource(createInfoRes);
}
void Wireframe::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    MRTColor::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.polygonMode = vk::PolygonMode::eLine;
    createInfoRes.rasterizationStateInfo.cullMode = vk::CullModeFlagBits::eNone;
}

const Pipeline::CreateInfo TEX_CREATE_INFO = {
    GRAPHICS::CDLOD_TEX_DEFERRED,
    "Cdlod Deferred Texture Pipeline",
    {
        SHADER::CDLOD_TEX_VERT,
        SHADER::DEFERRED_MRT_TEX_FRAG,
    },
    {
        {DESCRIPTOR_SET::UNIFORM_DEFAULT, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)},
        {DESCRIPTOR_SET::SAMPLER_DEFAULT, vk::ShaderStageFlagBits::eFragment},
        {DESCRIPTOR_SET::CDLOD_DEFAULT, (vk::ShaderStageFlagBits::eVertex)},
    },
    {},
    {PUSH_CONSTANT::CDLOD},
};
Texture::Texture(Handler& handler) : MRTTexture(handler, &TEX_CREATE_INFO) {}
void Texture::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    GetCdlodInputAssemblyInfoResource(createInfoRes);
}
void Texture::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    MRTTexture::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.frontFace = vk::FrontFace::eClockwise;
}

}  // namespace Cdlod
}  // namespace Pipeline
