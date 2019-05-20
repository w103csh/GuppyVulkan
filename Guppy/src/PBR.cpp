
#include "PBR.h"

#include "Instance.h"
#include "PipelineHandler.h"
#include "ShaderHandler.h"

// **********************
//      Light
// **********************

Light::PBR::Positional::Base::Base(const Buffer::Info&& info, DATA* pData, CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Light::Base<DATA>(pData, pCreateInfo),                 //
      position(getWorldSpacePosition()) {}

void Light::PBR::Positional::Base::update(glm::vec3&& position) {
    pData_->position = position;
    DIRTY = true;
}

// **********************
//      Material
// **********************

Material::PBR::Base::Base(const Buffer::Info&& info, PBR::DATA* pData, PBR::CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::DataItem<PBR::DATA>(pData),                    //
      Material::Base(MATERIAL::PBR, pCreateInfo)             //
{
    pData_->color = pCreateInfo->color;
    pData_->flags = pCreateInfo->flags;
    pData_->opacity = pCreateInfo->opacity;
    setRoughness(pCreateInfo->roughness);
    setData();
}

void Material::PBR::Base::setTextureData() {
    float xRepeat, yRepeat;
    xRepeat = yRepeat = repeat_;

    if (pTexture_ != nullptr) {
        // REPEAT
        // Deal with non-square images
        auto aspect = pTexture_->aspect;
        if (aspect > 1.0f) {
            yRepeat *= aspect;
        } else if (aspect < 1.0f) {
            xRepeat *= (1 / aspect);
        }
        // FLAGS
        pData_->flags &= ~Material::FLAG::PER_ALL;
        pData_->flags |= Material::FLAG::PER_TEXTURE_COLOR;
        pData_->texFlags = pTexture_->flags;
    } else {
        pData_->texFlags = 0;
    }

    pData_->xRepeat = xRepeat;
    pData_->yRepeat = yRepeat;
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

Descriptor::Set::PBR::Uniform::Uniform()
    : Set::Base{DESCRIPTOR_SET::UNIFORM_PBR,  //
                {
                    {{0, 0, 0}, {DESCRIPTOR::CAMERA_PERSPECTIVE_DEFAULT, {0}}},
                    {{0, 1, 0}, {DESCRIPTOR::MATERIAL_PBR, {0}}},
                    {{0, 2, 0}, {DESCRIPTOR::LIGHT_POSITIONAL_PBR, {OFFSET_ALL}}}  //
                }} {}

// **********************
//      Shader
// **********************

Shader::PBR::ColorFragment::ColorFragment(Shader::Handler& handler)
    : Base{handler,
           SHADER::PBR_COLOR_FRAG,
           "color.pbr.frag",
           VK_SHADER_STAGE_FRAGMENT_BIT,
           "PBR Color Fragment Shader",
           {
               SHADER_LINK::COLOR_FRAG,
               SHADER_LINK::PBR_FRAG,
               SHADER_LINK::PBR_MATERIAL,
           }} {}

Shader::PBR::TextureFragment::TextureFragment(Shader::Handler& handler)
    : Base{handler,
           SHADER::PBR_TEX_FRAG,
           "texture.pbr.frag",
           VK_SHADER_STAGE_FRAGMENT_BIT,
           "PBR Texture Fragment Shader",
           {
               SHADER_LINK::TEX_FRAG,
               SHADER_LINK::PBR_FRAG,
               SHADER_LINK::PBR_MATERIAL,
           }} {}

Shader::Link::PBR::Fragment::Fragment(Shader::Handler& handler)
    : Shader::Link::Base{
          handler,
          SHADER_LINK::PBR_FRAG,
          "link.pbr.frag",
          "PBR Link Fragment",
      } {}

Shader::Link::PBR::Material::Material(Shader::Handler& handler)
    : Shader::Link::Base{
          handler,
          SHADER_LINK::PBR_MATERIAL,
          "link.pbr.material.glsl",
          "PBR Link Material Shader",
      } {}

// **********************
//      Pipeline
// **********************

Pipeline::PBR::Color::Color(Pipeline::Handler& handler)
    : Base{
          handler,
          PIPELINE::PBR_COLOR,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          "PBR Color Pipeline",
          {SHADER::COLOR_VERT, SHADER::PBR_COLOR_FRAG},
          {/*PUSH_CONSTANT::DEFAULT*/},
          {DESCRIPTOR_SET::UNIFORM_PBR}  //
      } {};

Pipeline::PBR::Texture::Texture(Pipeline::Handler& handler)
    : Base{
          handler,
          PIPELINE::PBR_TEX,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          "PBR Texture Pipeline",
          {SHADER::TEX_VERT, SHADER::PBR_TEX_FRAG},
          {/*PUSH_CONSTANT::DEFAULT*/},
          {DESCRIPTOR_SET::UNIFORM_PBR, DESCRIPTOR_SET::SAMPLER_DEFAULT}  //
      } {};
