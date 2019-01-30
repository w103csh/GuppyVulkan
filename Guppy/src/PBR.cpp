
#include "PBR.h"

#include "PipelineHandler.h"
#include "ShaderHandler.h"

// **********************
//      Color Shader
// **********************

// **********************
//      Color Pipeline
// **********************

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
    handler_.shaderHandler().getStagesInfo({SHADER::COLOR_VERT, SHADER::PBR_COLOR_FRAG}, createInfoRes.stagesInfo);
}
