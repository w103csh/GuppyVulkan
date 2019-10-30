
#include "PBR.h"

#include "Instance.h"
// HANDLERS
#include "PipelineHandler.h"
#include "ShaderHandler.h"

// **********************
//      Light
// **********************

Light::PBR::Positional::Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),                  //
      Light::Base<DATA>(UNIFORM::LIGHT_POSITIONAL_PBR, pData, pCreateInfo),  //
      position(getWorldSpacePosition()) {}

void Light::PBR::Positional::Base::update(glm::vec3&& position, const uint32_t frameIndex) {
    data_.position = position;
    setData(frameIndex);
}

// **********************
//      Material
// **********************

Material::PBR::Base::Base(const Buffer::Info&& info, PBR::DATA* pData, const PBR::CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Material::Base(UNIFORM_DYNAMIC::MATERIAL_PBR, pCreateInfo),
      Buffer::DataItem<PBR::DATA>(pData)  //
{
    pData_->color = pCreateInfo->color;
    pData_->flags = pCreateInfo->flags;
    pData_->opacity = pCreateInfo->opacity;
    setRoughness(pCreateInfo->roughness);
    setData();
}

void Material::PBR::Base::setTinyobjData(const tinyobj::material_t& m) {
    pData_->color = {m.diffuse[0], m.diffuse[1], m.diffuse[2]};
    setData();
}

void Material::PBR::Base::setRoughness(float r) {
    assert(r >= 0.0f && r <= 1.0f && "Roughness is a scalar between 0 and 1");
    pData_->roughness = r;
}

// **********************
//      Descriptor Set
// **********************

namespace Descriptor {
namespace Set {
namespace PBR {
const CreateInfo UNIFORM_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_PBR,
    "_DS_UNI_PBR",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_PBR}},
        {{2, 0}, {UNIFORM::LIGHT_POSITIONAL_PBR}},
    },
};
}  // namespace PBR
}  // namespace Set
}  // namespace Descriptor

// **********************
//      Shader
// **********************

namespace Shader {
namespace PBR {
const CreateInfo COLOR_FRAG_CREATE_INFO = {
    SHADER::PBR_COLOR_FRAG,
    "PBR Color Fragment Shader",
    "color.pbr.frag",
    VK_SHADER_STAGE_FRAGMENT_BIT,
    {
        SHADER_LINK::COLOR_FRAG,
        SHADER_LINK::PBR_FRAG,
        SHADER_LINK::PBR_MATERIAL,
    },
};
const CreateInfo TEX_FRAG_CREATE_INFO = {
    SHADER::PBR_TEX_FRAG,
    "PBR Texture Fragment Shader",
    "texture.pbr.frag",
    VK_SHADER_STAGE_FRAGMENT_BIT,
    {
        SHADER_LINK::TEX_FRAG,
        SHADER_LINK::PBR_FRAG,
        SHADER_LINK::PBR_MATERIAL,
    },
};
}  // namespace PBR
namespace Link {
namespace PBR {
const CreateInfo FRAG_CREATE_INFO = {
    SHADER_LINK::PBR_MATERIAL,  //
    "link.pbr.frag",
};
const CreateInfo MATERIAL_CREATE_INFO = {
    SHADER_LINK::PBR_MATERIAL,  //
    "link.pbr.material.glsl",
};
}  // namespace PBR
}  // namespace Link
}  // namespace Shader

//  PIPELINE

const Pipeline::CreateInfo COLOR_CREATE_INFO = {
    PIPELINE::PBR_COLOR,
    "PBR Color Pipeline",
    {SHADER::COLOR_VERT, SHADER::PBR_COLOR_FRAG},
    {DESCRIPTOR_SET::UNIFORM_PBR},
};

Pipeline::PBR::Color::Color(Pipeline::Handler& handler) : Graphics(handler, &COLOR_CREATE_INFO) {}

const Pipeline::CreateInfo TEX_CREATE_INFO = {
    PIPELINE::PBR_TEX,
    "PBR Texture Pipeline",
    {SHADER::TEX_VERT, SHADER::PBR_TEX_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_PBR,
        DESCRIPTOR_SET::SAMPLER_DEFAULT,
    },
};

Pipeline::PBR::Texture::Texture(Pipeline::Handler& handler) : Graphics(handler, &TEX_CREATE_INFO) {}
