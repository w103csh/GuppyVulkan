
#include "PBR.h"

#include "PipelineHandler.h"
#include "ShaderHandler.h"

// **********************
//      Light
// **********************

Light::PBR::Positional::Base::Base(const Buffer::Info&& info, DATA* pData, CreateInfo* pCreateInfo)
    : Light::Base<DATA>(pData, pCreateInfo),  //
      Buffer::Item(std::forward<const Buffer::Info>(info)),
      position(getWorldSpacePosition()) {}

void Light::PBR::Positional::Base::update(glm::vec3&& position) {
    pData_->position = position;
    DIRTY = true;
}

// **********************
//      Material
// **********************

Material::PBR::Base::Base(const Buffer::Info&& info, DATA* pData, PBR::CreateInfo* pCreateInfo)
    : Material::Base(pCreateInfo),    //
      Buffer::DataItem<DATA>(pData),  //
      Buffer::Item(std::forward<const Buffer::Info>(info)) {
    pData_->color = pCreateInfo->diffuseCoeff;
    pData_->flags = pCreateInfo->flags;
    pData_->roughness = pCreateInfo->roughness;
    setData();
}

void Material::PBR::Base::setTextureData() {
    assert(false);
    // float xRepeat, yRepeat;
    // xRepeat = yRepeat = repeat_;

    // if (pTexture_ != nullptr) {
    //    // REPEAT
    //    // Deal with non-square images
    //    auto aspect = pTexture_->aspect;
    //    if (aspect > 1.0f) {
    //        yRepeat *= aspect;
    //    } else if (aspect < 1.0f) {
    //        xRepeat *= (1 / aspect);
    //    }
    //    // FLAGS
    //    pData_->texFlags = pTexture_->flags;
    //}

    // pData_->xRepeat = xRepeat;
    // pData_->yRepeat = yRepeat;
    setData();
}

void Material::PBR::Base::setTinyobjData(const tinyobj::material_t& m) {
    // pData_->shininess = m.shininess;
    // pData_->Ka = {m.ambient[0], m.ambient[1], m.ambient[2]};
    pData_->color = {m.diffuse[0], m.diffuse[1], m.diffuse[2]};
    // pData_->Ks = {m.specular[0], m.specular[1], m.specular[2]};
    setData();
}

// **********************
//      Descriptor Set
// **********************

Descriptor::Set::PBR::Uniform::Uniform()
    : Set::Base{DESCRIPTOR_SET::UNIFORM_PBR,  //
                {
                    {{0, 0, 0}, {DESCRIPTOR::CAMERA_PERSPECTIVE_DEFAULT, {OFFSET_SINGLE}}},
                    {{0, 1, 0}, {DESCRIPTOR::MATERIAL_PBR, {OFFSET_SINGLE}}},
                    {{0, 2, 0}, {DESCRIPTOR::LIGHT_POSITIONAL_DEFAULT, {OFFSET_ALL}}}  //
                }} {}

// **********************
//      Shader
// **********************

Shader::PBR::ColorFrag::ColorFrag(Shader::Handler& handler)
    : Base{
          handler,                       //
          SHADER::PBR_COLOR_FRAG,        //
          "color.pbr.frag",              //
          VK_SHADER_STAGE_FRAGMENT_BIT,  //
          "PBR Color Fragment Shader"    //
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

void Pipeline::PBR::Color::getShaderInfoResources(Pipeline::CreateInfoResources& createInfoRes) {
    createInfoRes.stagesInfo.clear();  // TODO: faster way?
    handler().shaderHandler().getStagesInfo({SHADER::COLOR_VERT, SHADER::PBR_COLOR_FRAG}, createInfoRes.stagesInfo);
}
