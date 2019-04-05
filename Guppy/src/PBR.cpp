
#include "PBR.h"

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
        pData_->flags &= Material::FLAG::PER_TEXTURE_COLOR;
        pData_->texFlags = pTexture_->flags;
    } else {
        pData_->texFlags = Texture::FLAG::NONE;
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
    : Base{
          handler,                                          //
          SHADER::PBR_COLOR_FRAG,                           //
          "color.pbr.frag",                                 //
          VK_SHADER_STAGE_FRAGMENT_BIT,                     //
          "PBR Color Fragment Shader",                      //
          {SHADER_LINK::COLOR_FRAG, SHADER_LINK::PBR_FRAG}  //
      } {}

Shader::PBR::TextureFragment::TextureFragment(Shader::Handler& handler)
    : Base{
          handler,                                        //
          SHADER::PBR_TEX_FRAG,                           //
          "texture.pbr.frag",                             //
          VK_SHADER_STAGE_FRAGMENT_BIT,                   //
          "PBR Texture Fragment Shader",                  //
          {SHADER_LINK::TEX_FRAG, SHADER_LINK::PBR_FRAG}  //
      } {}

Shader::Link::PBR::Fragment::Fragment(Shader::Handler& handler)
    : Shader::Link::Base{
          handler,                //
          SHADER_LINK::PBR_FRAG,  //
          "link.pbr.frag",        //
          "PBR Link Fragment"     //
      } {}

// **********************
//      Pipeline
// **********************

Pipeline::PBR::Color::Color(Pipeline::Handler& handler)
    : Base{
          handler,
          PIPELINE::PBR_COLOR,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          "Default Triangle List Texture",
          {SHADER::COLOR_VERT, SHADER::PBR_COLOR_FRAG},
          {PUSH_CONSTANT::DEFAULT},
          {DESCRIPTOR_SET::UNIFORM_PBR}  //
      } {};

void Pipeline::PBR::Color::getInputAssemblyInfoResources(Pipeline::CreateInfoResources& createInfoRes) {
    // color vertex
    createInfoRes.bindingDesc = Vertex::getColorBindDesc();
    createInfoRes.attrDesc = Vertex::getColorAttrDesc();
    // bindings
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = 1;
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = &createInfoRes.bindingDesc;
    // attributes
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDesc.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDesc.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = {};
    createInfoRes.inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    createInfoRes.inputAssemblyStateInfo.pNext = nullptr;
    createInfoRes.inputAssemblyStateInfo.flags = 0;
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

Pipeline::PBR::Texture::Texture(Pipeline::Handler& handler)
    : Base{
          handler,
          PIPELINE::PBR_TEX,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          "Default Triangle List Texture",
          {SHADER::TEX_VERT, SHADER::PBR_TEX_FRAG},
          {PUSH_CONSTANT::DEFAULT},
          {DESCRIPTOR_SET::UNIFORM_PBR, DESCRIPTOR_SET::SAMPLER_DEFAULT}  //
      } {};

void Pipeline::PBR::Texture::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    // texture vertex
    createInfoRes.bindingDesc = Vertex::getTexBindDesc();
    createInfoRes.attrDesc = Vertex::getTexAttrDesc();
    // bindings
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = 1;
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = &createInfoRes.bindingDesc;
    // attributes
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDesc.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDesc.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = {};
    createInfoRes.inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    createInfoRes.inputAssemblyStateInfo.pNext = nullptr;
    createInfoRes.inputAssemblyStateInfo.flags = 0;
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}
