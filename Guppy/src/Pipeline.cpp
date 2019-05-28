
#include "Pipeline.h"

#include "Instance.h"
#include "Vertex.h"
// HANDLERS
#include "DescriptorHandler.h"
#include "PipelineHandler.h"
#include "ShaderHandler.h"
#include "TextureHandler.h"

void Pipeline::GetDefaultColorInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    // bindings
    Vertex::Color::getBindingDescriptions(createInfoRes.bindDescs);
    Instance::Default::DATA::getBindingDescriptions(createInfoRes.bindDescs);
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = createInfoRes.bindDescs.data();
    // attributes
    Vertex::Color::getAttributeDescriptions(createInfoRes.attrDescs);
    Instance::Default::DATA::getAttributeDescriptions(createInfoRes.attrDescs);
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = {};
    createInfoRes.inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    createInfoRes.inputAssemblyStateInfo.pNext = nullptr;
    createInfoRes.inputAssemblyStateInfo.flags = 0;
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

void Pipeline::GetDefaultTextureInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    // bindings
    Vertex::Texture::getBindingDescriptions(createInfoRes.bindDescs);
    Instance::Default::DATA::getBindingDescriptions(createInfoRes.bindDescs);
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = createInfoRes.bindDescs.data();
    // attributes
    Vertex::Texture::getAttributeDescriptions(createInfoRes.attrDescs);
    Instance::Default::DATA::getAttributeDescriptions(createInfoRes.attrDescs);
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = {};
    createInfoRes.inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    createInfoRes.inputAssemblyStateInfo.pNext = nullptr;
    createInfoRes.inputAssemblyStateInfo.flags = 0;
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

// **********************
//      BASE
// **********************

Pipeline::Base::Base(Pipeline::Handler& handler, const PIPELINE&& type, const VkPipelineBindPoint&& bindPoint,
                     const std::string&& name, const std::set<SHADER>&& shaderTypes,
                     const std::vector<PUSH_CONSTANT>&& pushConstantTypes, std::vector<DESCRIPTOR_SET>&& descriptorSets)
    : Handlee(handler),
      BIND_POINT(bindPoint),
      DESCRIPTOR_SET_TYPES(descriptorSets),
      NAME(name),
      PUSH_CONSTANT_TYPES(pushConstantTypes),
      SHADER_TYPES(shaderTypes),
      TYPE(type),
      status_(STATUS::PENDING),
      layout_(VK_NULL_HANDLE),
      pipeline_(VK_NULL_HANDLE),
      subpassId_(0) {
    for (const auto& type : PUSH_CONSTANT_TYPES) assert(type != PUSH_CONSTANT::DONT_CARE);
}

void Pipeline::Base::init() {
    validatePipelineDescriptorSets();
    createPipelineLayout();
}

void Pipeline::Base::updateStatus() {
    assert(status_ != STATUS::READY);
    auto it = pendingTexturesOffsets_.begin();
    while (it != pendingTexturesOffsets_.end()) {
        // check texture
        const auto& pTexture = handler().textureHandler().getTexture(*it);
        assert(pTexture != nullptr && "Couldn't find texture.");
        if (pTexture->status == STATUS::READY)
            it = pendingTexturesOffsets_.erase(it);
        else
            ++it;
    }
    if (pendingTexturesOffsets_.empty()) status_ = STATUS::READY;
}

void Pipeline::Base::validatePipelineDescriptorSets() {
    for (const auto& setType : DESCRIPTOR_SET_TYPES) {
        const auto& descSet = handler().descriptorHandler().getDescriptorSet(setType);
        for (const auto& keyValue : descSet.BINDING_MAP) {
            if (std::get<0>(keyValue.second) == DESCRIPTOR::SAMPLER_PIPELINE_COMBINED ||
                std::get<2>(keyValue.second).size()) {
                // validate tuple
                assert(std::get<0>(keyValue.second) == DESCRIPTOR::SAMPLER_PIPELINE_COMBINED &&
                       std::get<2>(keyValue.second).size());
                // check texture
                const auto& pTexture = handler().textureHandler().getTextureByName(std::get<2>(keyValue.second));
                assert(pTexture != nullptr && "Couldn't find texture.");
                if (pTexture->status != STATUS::READY) pendingTexturesOffsets_.push_back(pTexture->OFFSET);
            }
        }
    }
    if (pendingTexturesOffsets_.size()) status_ = STATUS::PENDING_TEXTURE;
}

std::vector<VkDescriptorSetLayout> Pipeline::Base::prepareDescSetInfo() {
    std::vector<VkDescriptorSetLayout> layouts;

    uint8_t slot = 0;
    for (const auto& setType : DESCRIPTOR_SET_TYPES) {
        const auto& descSet = handler().descriptorHandler().getDescriptorSet(setType);
        descSetMacroSlotMap_[descSet.MACRO_NAME] = slot++;
        layouts.push_back(descSet.layout);
    }

    assert(layouts.size() == DESCRIPTOR_SET_TYPES.size() && "Wrong amount of set layouts");
    assert(descSetMacroSlotMap_.size() == DESCRIPTOR_SET_TYPES.size() && "Wrong amount of set slot keys");

    return layouts;
}

void Pipeline::Base::createPipelineLayout() {
    // PUSH CONSTANTS
    pushConstantRanges_ = handler().getPushConstantRanges(TYPE, PUSH_CONSTANT_TYPES);
    // DESCRIPTOR LAYOUTS
    const auto& descSetLayouts = prepareDescSetInfo();

    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges_.size());
    layoutInfo.pPushConstantRanges = pushConstantRanges_.data();
    layoutInfo.setLayoutCount = static_cast<uint32_t>(descSetLayouts.size());
    layoutInfo.pSetLayouts = descSetLayouts.data();

    vk::assert_success(vkCreatePipelineLayout(handler().shell().context().dev, &layoutInfo, nullptr, &layout_));

    if (handler().settings().enable_debug_markers) {
        std::string markerName = NAME + " pipeline layout";
        ext::DebugMarkerSetObjectName(handler().shell().context().dev, (uint64_t)layout_,
                                      VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, markerName.c_str());
    }
}

const VkPipeline& Pipeline::Base::create(const VkPipelineCache& cache, CreateInfoResources& createInfoRes,
                                         VkGraphicsPipelineCreateInfo& pipelineCreateInfo,
                                         const VkPipeline& basePipelineHandle, const int32_t basePipelineIndex) {
    /*
        The idea here is that this can be overridden in a bunch of ways, or you
        can just use the easier and slower way of calling this function from
        derivers.
    */

    // TODO: where should this go????
    createInfoRes.bindDescs.clear();
    createInfoRes.attrDescs.clear();

    // Gather info from derived classes...
    getBlendInfoResources(createInfoRes);
    getDepthInfoResources(createInfoRes);
    getDynamicStateInfoResources(createInfoRes);
    getInputAssemblyInfoResources(createInfoRes);
    getMultisampleStateInfoResources(createInfoRes);
    getRasterizationStateInfoResources(createInfoRes);
    getShaderInfoResources(createInfoRes);
    getTesselationInfoResources(createInfoRes);
    getViewportStateInfoResources(createInfoRes);

    // PIPELINE
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    // INPUT ASSEMBLY
    pipelineCreateInfo.pInputAssemblyState = &createInfoRes.inputAssemblyStateInfo;
    pipelineCreateInfo.pVertexInputState = &createInfoRes.vertexInputStateInfo;
    // SHADER
    pipelineCreateInfo.stageCount = static_cast<uint32_t>(createInfoRes.stagesInfo.size());
    pipelineCreateInfo.pStages = createInfoRes.stagesInfo.data();
    // FIXED FUNCTION
    pipelineCreateInfo.pColorBlendState = &createInfoRes.colorBlendStateInfo;
    pipelineCreateInfo.pDepthStencilState = &createInfoRes.depthStencilStateInfo;
    pipelineCreateInfo.pDynamicState = &createInfoRes.dynamicStateInfo;
    pipelineCreateInfo.pInputAssemblyState = &createInfoRes.inputAssemblyStateInfo;
    pipelineCreateInfo.pMultisampleState = &createInfoRes.multisampleStateInfo;
    pipelineCreateInfo.pRasterizationState = &createInfoRes.rasterizationStateInfo;
    pipelineCreateInfo.pTessellationState = &createInfoRes.tessellationStateInfo;
    pipelineCreateInfo.pVertexInputState = &createInfoRes.vertexInputStateInfo;
    pipelineCreateInfo.pViewportState = &createInfoRes.viewportStateInfo;
    // LAYOUT
    pipelineCreateInfo.layout = layout_;
    pipelineCreateInfo.basePipelineHandle = basePipelineHandle;
    pipelineCreateInfo.basePipelineIndex = basePipelineIndex;
    // RENDER PASS
    // pipelineCreateInfo.renderPass = renderPass;
    // pipelineCreateInfo.subpass = subpass;

    auto& oldPipeline = pipeline_;  // Save old handler for clean up if rebuilding
    vk::assert_success(
        vkCreateGraphicsPipelines(handler().shell().context().dev, cache, 1, &pipelineCreateInfo, nullptr, &pipeline_));

    if (handler().settings().enable_debug_markers) {
        std::string markerName = NAME + " pipline";
        ext::DebugMarkerSetObjectName(handler().shell().context().dev, (uint64_t)pipeline_,
                                      VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, markerName.c_str());
    }

    return oldPipeline;
}

void Pipeline::Base::destroy() {
    const auto& dev = handler().shell().context().dev;
    if (layout_ != VK_NULL_HANDLE) vkDestroyPipelineLayout(dev, layout_, nullptr);
    if (pipeline_ != VK_NULL_HANDLE) vkDestroyPipeline(dev, pipeline_, nullptr);
}

void Pipeline::Base::getDynamicStateInfoResources(CreateInfoResources& createInfoRes) {
    // TODO: this is weird
    createInfoRes.dynamicStateInfo = {};
    memset(createInfoRes.dynamicStates, 0, sizeof(createInfoRes.dynamicStates));
    createInfoRes.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    createInfoRes.dynamicStateInfo.pNext = nullptr;
    createInfoRes.dynamicStateInfo.pDynamicStates = createInfoRes.dynamicStates;
    createInfoRes.dynamicStateInfo.dynamicStateCount = 0;
}

void Pipeline::Base::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    auto range = VERTEX_PIPELINE_MAP.equal_range(VERTEX::COLOR);
    if (range.first != range.second && range.first->second.count(TYPE)) {
        GetDefaultColorInputAssemblyInfoResources(createInfoRes);
        return;
    }
    range = VERTEX_PIPELINE_MAP.equal_range(VERTEX::TEXTURE);
    if (range.first != range.second && range.first->second.count(TYPE)) {
        GetDefaultTextureInputAssemblyInfoResources(createInfoRes);
        return;
    }
    assert(false);
    throw std::runtime_error("Unhandled type for input assembly. Override or add to this function?");
}

void Pipeline::Base::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.rasterizationStateInfo = {};
    createInfoRes.rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    createInfoRes.rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    createInfoRes.rasterizationStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    createInfoRes.rasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    /*  If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far
        planes are clamped to them as opposed to discarding them. This is useful in some special
        cases like shadow maps. Using this requires enabling a GPU feature.
    */
    createInfoRes.rasterizationStateInfo.depthClampEnable = VK_FALSE;
    createInfoRes.rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
    createInfoRes.rasterizationStateInfo.depthBiasEnable = VK_FALSE;
    createInfoRes.rasterizationStateInfo.depthBiasConstantFactor = 0;
    createInfoRes.rasterizationStateInfo.depthBiasClamp = 0;
    createInfoRes.rasterizationStateInfo.depthBiasSlopeFactor = 0;
    /*  The lineWidth member is straightforward, it describes the thickness of lines in terms of
        number of fragments. The maximum line width that is supported depends on the hardware and
        any line thicker than 1.0f requires you to enable the wideLines GPU feature.
    */
    createInfoRes.rasterizationStateInfo.lineWidth = 1.0f;
}

void Pipeline::Base::getShaderInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.stagesInfo.clear();  // TODO: faster way?
    for (const auto& shaderType : SHADER_TYPES) {
        Shader::shaderInfoMapKey key = {shaderType, descSetMacroSlotMap_};
        handler().shaderHandler().getStagesInfo(key, createInfoRes.stagesInfo);
    }
}

void Pipeline::Base::getMultisampleStateInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.multisampleStateInfo = {};
    createInfoRes.multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    createInfoRes.multisampleStateInfo.rasterizationSamples = handler().shell().context().samples;
    createInfoRes.multisampleStateInfo.sampleShadingEnable =
        handler()
            .settings()
            .enable_sample_shading;  // enable sample shading in the pipeline (sampling for fragment interiors)
    createInfoRes.multisampleStateInfo.minSampleShading =
        handler().settings().enable_sample_shading ? MIN_SAMPLE_SHADING
                                                   : 0.0f;     // min fraction for sample shading; closer to one is smooth
    createInfoRes.multisampleStateInfo.pSampleMask = nullptr;  // Optional
    createInfoRes.multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
    createInfoRes.multisampleStateInfo.alphaToOneEnable = VK_FALSE;       // Optional
}

void Pipeline::Base::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.blendAttach = {};
    createInfoRes.blendAttach.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // blend_attachment.blendEnable = VK_FALSE;
    // blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional
    // blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
    // blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    // blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    // blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    // blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    // common setup
    createInfoRes.blendAttach.blendEnable = VK_TRUE;
    createInfoRes.blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    createInfoRes.blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    createInfoRes.blendAttach.colorBlendOp = VK_BLEND_OP_ADD;
    createInfoRes.blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    createInfoRes.blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    createInfoRes.blendAttach.alphaBlendOp = VK_BLEND_OP_ADD;
    // createInfoRes.blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    // createInfoRes.blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // createInfoRes.blendAttach.alphaBlendOp = VK_BLEND_OP_ADD;

    createInfoRes.colorBlendStateInfo = {};
    createInfoRes.colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    createInfoRes.colorBlendStateInfo.attachmentCount = 1;
    createInfoRes.colorBlendStateInfo.pAttachments = &createInfoRes.blendAttach;
    createInfoRes.colorBlendStateInfo.logicOpEnable = VK_FALSE;
    createInfoRes.colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;  // What does this do?
    createInfoRes.colorBlendStateInfo.blendConstants[0] = 0.0f;
    createInfoRes.colorBlendStateInfo.blendConstants[1] = 0.0f;
    createInfoRes.colorBlendStateInfo.blendConstants[2] = 0.0f;
    createInfoRes.colorBlendStateInfo.blendConstants[3] = 0.0f;
    // createInfoRes.colorBlendStateInfo.blendConstants[0] = 0.2f;
    // createInfoRes.colorBlendStateInfo.blendConstants[1] = 0.2f;
    // createInfoRes.colorBlendStateInfo.blendConstants[2] = 0.2f;
    // createInfoRes.colorBlendStateInfo.blendConstants[3] = 0.2f;
}

void Pipeline::Base::getViewportStateInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.viewportStateInfo = {};
    createInfoRes.viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
#ifndef __ANDROID__
    createInfoRes.viewportStateInfo.viewportCount = NUM_VIEWPORTS;
    createInfoRes.dynamicStates[createInfoRes.dynamicStateInfo.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    createInfoRes.viewportStateInfo.scissorCount = NUM_SCISSORS;
    createInfoRes.dynamicStates[createInfoRes.dynamicStateInfo.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
    createInfoRes.viewportStateInfo.pScissors = nullptr;
    createInfoRes.viewportStateInfo.pViewports = nullptr;
#else
    // TODO: this is outdated now...
    // Temporary disabling dynamic viewport on Android because some of drivers doesn't
    // support the feature.
    VkViewport viewports;
    VkViewport viewports;
    viewports.minDepth = 0.0f;
    viewports.maxDepth = 1.0f;
    viewports.x = 0;
    viewports.y = 0;
    viewports.width = info.width;
    viewports.height = info.height;
    VkRect2D scissor;
    scissor.extent.width = info.width;
    scissor.extent.height = info.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vp.viewportCount = NUM_VIEWPORTS;
    vp.scissorCount = NUM_SCISSORS;
    vp.pScissors = &scissor;
    vp.pViewports = &viewports;
#endif
}

void Pipeline::Base::getDepthInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.depthStencilStateInfo = {};
    createInfoRes.depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    createInfoRes.depthStencilStateInfo.pNext = nullptr;
    createInfoRes.depthStencilStateInfo.flags = 0;
    createInfoRes.depthStencilStateInfo.depthTestEnable = handler().settings().include_depth;
    createInfoRes.depthStencilStateInfo.depthWriteEnable = handler().settings().include_depth;
    createInfoRes.depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    createInfoRes.depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
    createInfoRes.depthStencilStateInfo.minDepthBounds = 0;
    createInfoRes.depthStencilStateInfo.maxDepthBounds = 2.0f;
    createInfoRes.depthStencilStateInfo.stencilTestEnable = VK_FALSE;
    createInfoRes.depthStencilStateInfo.front = {};
    createInfoRes.depthStencilStateInfo.back = {};
    // dss.back.failOp = VK_STENCIL_OP_KEEP; // ARE THESE IMPORTANT !!!
    // dss.back.passOp = VK_STENCIL_OP_KEEP;
    // dss.back.compareOp = VK_COMPARE_OP_ALWAYS;
    // dss.back.compareMask = 0;
    // dss.back.reference = 0;
    // dss.back.depthFailOp = VK_STENCIL_OP_KEEP;
    // dss.back.writeMask = 0;
    // dss.front = ds.back;
}

void Pipeline::Base::getTesselationInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.tessellationStateInfo = {};
}

// DEFAULT TRIANGLE LIST COLOR
Pipeline::Default::TriListColor::TriListColor(Pipeline::Handler& handler)
    : Base{
          handler,
          PIPELINE::TRI_LIST_COLOR,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          "Default Triangle List Color",
          {SHADER::COLOR_VERT, SHADER::COLOR_FRAG},
          {/*PUSH_CONSTANT::DEFAULT*/},
          {
              DESCRIPTOR_SET::UNIFORM_DEFAULT,
              DESCRIPTOR_SET::PROJECTOR_DEFAULT,
          },
      } {};

// DEFAULT LINE
Pipeline::Default::Line::Line(Pipeline::Handler& handler)
    : Base{
          handler,
          PIPELINE::LINE,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          "Default Line",
          {SHADER::COLOR_VERT, SHADER::LINE_FRAG},
          {/*PUSH_CONSTANT::DEFAULT*/},
          {DESCRIPTOR_SET::UNIFORM_DEFAULT},
      } {};

void Pipeline::Default::Line::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    GetDefaultColorInputAssemblyInfoResources(createInfoRes);
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
}

// DEFAULT TRIANGLE LIST TEXTURE
Pipeline::Default::TriListTexture::TriListTexture(Pipeline::Handler& handler)
    : Base{
          handler,
          PIPELINE::TRI_LIST_TEX,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          "Default Triangle List Texture",
          {SHADER::TEX_VERT, SHADER::TEX_FRAG},
          {/*PUSH_CONSTANT::DEFAULT*/},
          {
              DESCRIPTOR_SET::UNIFORM_DEFAULT,
              DESCRIPTOR_SET::SAMPLER_DEFAULT,
              DESCRIPTOR_SET::PROJECTOR_DEFAULT,
          },
      } {};

// CUBE
Pipeline::Default::Cube::Cube(Pipeline::Handler& handler)
    : Base{
          handler,
          PIPELINE::CUBE,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          "Cube Pipeline",
          {SHADER::CUBE_VERT, SHADER::CUBE_FRAG},
          {/*PUSH_CONSTANT::DEFAULT*/},
          {
              DESCRIPTOR_SET::UNIFORM_DEFAULT,
              DESCRIPTOR_SET::SAMPLER_CUBE_DEFAULT,
          },
      } {}

void Pipeline::Default::Cube::getDepthInfoResources(CreateInfoResources& createInfoRes) {
    Pipeline::Base::getDepthInfoResources(createInfoRes);
    createInfoRes.depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
}

// BLINN PHONG TEXTURE CULL NONE
Pipeline::BP::TextureCullNone::TextureCullNone(Pipeline::Handler& handler)
    : Base{
          handler,
          PIPELINE::BP_TEX_CULL_NONE,
          VK_PIPELINE_BIND_POINT_GRAPHICS,
          "Blinn Phong Texture Cull None",
          {SHADER::TEX_VERT, SHADER::TEX_FRAG},
          {/*PUSH_CONSTANT::DEFAULT*/},
          {
              DESCRIPTOR_SET::UNIFORM_DEFAULT,
              DESCRIPTOR_SET::SAMPLER_DEFAULT,
              DESCRIPTOR_SET::PROJECTOR_DEFAULT,
          },
      } {};

void Pipeline::BP::TextureCullNone::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    Pipeline::Base::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.cullMode = VK_CULL_MODE_NONE;
}
