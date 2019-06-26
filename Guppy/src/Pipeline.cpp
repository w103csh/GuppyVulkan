
#include "Pipeline.h"

#include <algorithm>

#include "ConstantsAll.h"
#include "Instance.h"
#include "Vertex.h"
// HANDLERS
#include "DescriptorHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"
#include "ShaderHandler.h"
#include "TextureHandler.h"

void Pipeline::GetDefaultColorInputAssemblyInfoResources(CreateInfoVkResources& createInfoRes) {
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

void Pipeline::GetDefaultTextureInputAssemblyInfoResources(CreateInfoVkResources& createInfoRes) {
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

//  BASE

Pipeline::Base::Base(Pipeline::Handler& handler, const Pipeline::CreateInfo* pCreateInfo)
    : Handlee(handler),
      BIND_POINT(pCreateInfo->bindPoint),
      DESCRIPTOR_SET_TYPES(pCreateInfo->descriptorSets),
      NAME(pCreateInfo->name),
      PUSH_CONSTANT_TYPES(pCreateInfo->pushConstantTypes),
      SHADER_TYPES(pCreateInfo->shaderTypes),
      TYPE(pCreateInfo->type),
      status_(STATUS::PENDING),
      descriptorOffsets_(pCreateInfo->uniformOffsets) {
    assert(TYPE != PIPELINE::ALL_ENUM);
    for (const auto& type : PUSH_CONSTANT_TYPES) assert(type != PUSH_CONSTANT::DONT_CARE);
}

void Pipeline::Base::init() {
    validatePipelineDescriptorSets();
    makePipelineLayouts();
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
            bool hasPipelineSampler = std::visit(Descriptor::IsCombinedSamplerPipeline{}, std::get<0>(keyValue.second));
            if (hasPipelineSampler || std::get<1>(keyValue.second).size()) {
                // validate tuple
                assert(hasPipelineSampler && std::get<1>(keyValue.second).size());
                // check texture
                const auto& pTexture = handler().textureHandler().getTextureByName(std::get<1>(keyValue.second));
                assert(pTexture != nullptr && "Couldn't find texture.");
                if (pTexture->status != STATUS::READY) pendingTexturesOffsets_.push_back(pTexture->OFFSET);
            }
        }
    }
    if (pendingTexturesOffsets_.size()) status_ = STATUS::PENDING_TEXTURE;
}

void Pipeline::Base::prepareDescriptorSetInfo() {
    // Get a list of active passes for this pipeline type.
    const auto passTypes = handler().passHandler().getActivePassTypes(TYPE);

    // Gather a culled list of resources.
    auto helpers = handler().descriptorHandler().getResourceHelpers(passTypes, TYPE, DESCRIPTOR_SET_TYPES);

    for (auto resourceIndex = 0; resourceIndex < helpers.front().size(); resourceIndex++) {
        // Loop through each layer of descriptor set resources at resourceIndex
        for (auto setIndex = 0; setIndex < helpers.size(); setIndex++) {
            auto& resourceTuple = helpers[setIndex][resourceIndex];
            const auto& descSet = handler().descriptorHandler().getDescriptorSet(std::get<3>(resourceTuple));

            // Add the culled sets to a shader replace map, for later shader text replacement.
            if (shaderTextReplaceInfoMap_.count(std::get<0>(resourceTuple)) == 0) {
                shaderTextReplaceInfoMap_.insert({std::get<0>(resourceTuple), {}});
            }
            shaderTextReplaceInfoMap_.at(std::get<0>(resourceTuple))
                .push_back({
                    descSet.MACRO_NAME,
                    setIndex,
                    descSet.getDescriptorOffsets(std::get<2>(resourceTuple)),
                    std::get<4>(resourceTuple),
                });

            // Add the culled sets to a layouts data structure that can be used to create
            // the pipeline layouts.
            if (layoutsMap_.count(std::get<0>(resourceTuple)) == 0) {
                layoutsMap_.insert(std::pair<std::set<RENDER_PASS>, Layouts>{
                    std::get<4>(resourceTuple),
                    {VK_NULL_HANDLE, {}},
                });
            }
            layoutsMap_.at(std::get<4>(resourceTuple)).descSetLayouts.push_back(std::get<1>(resourceTuple)->layout);
        }
    }

    for (const auto& keyValue : shaderTextReplaceInfoMap_) assert(keyValue.second.size() == DESCRIPTOR_SET_TYPES.size());
    assert(layoutsMap_.size() == helpers.front().size());
    for (const auto& keyValue : layoutsMap_) assert(keyValue.second.descSetLayouts.size() == DESCRIPTOR_SET_TYPES.size());
}

std::shared_ptr<Pipeline::BindData> Pipeline::Base::makeBindData(const VkPipelineLayout& layout) {
    return std::shared_ptr<Pipeline::BindData>(new Pipeline::BindData{
        BIND_POINT,
        layout,
        VK_NULL_HANDLE,
        0,
        PUSH_CONSTANT_TYPES,
    });
}

void Pipeline::Base::makePipelineLayouts() {
    // TODO: destroy layouts if they already exist

    prepareDescriptorSetInfo();

    for (auto& keyValue : layoutsMap_) {
        // PUSH CONSTANTS
        pushConstantRanges_ = handler().getPushConstantRanges(TYPE, PUSH_CONSTANT_TYPES);

        VkPipelineLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges_.size());
        layoutInfo.pPushConstantRanges = pushConstantRanges_.data();
        layoutInfo.setLayoutCount = static_cast<uint32_t>(keyValue.second.descSetLayouts.size());
        layoutInfo.pSetLayouts = keyValue.second.descSetLayouts.data();

        vk::assert_success(
            vkCreatePipelineLayout(handler().shell().context().dev, &layoutInfo, nullptr, &keyValue.second.pipelineLayout));

        if (handler().settings().enable_debug_markers) {
            std::string markerName = NAME + " pipeline layout";
            ext::DebugMarkerSetObjectName(handler().shell().context().dev, (uint64_t)&keyValue.second.pipelineLayout,
                                          VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, markerName.c_str());
        }
    }
}

void Pipeline::Base::setInfo(CreateInfoVkResources& createInfoRes, VkGraphicsPipelineCreateInfo& pipelineCreateInfo) {
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
}

const std::shared_ptr<Pipeline::BindData>& Pipeline::Base::getBindData(const RENDER_PASS& passType) {
    std::set<RENDER_PASS> tempSet;
    const auto& pPass = handler().passHandler().getPass(passType);

    // Find the layout map element for the pass type.
    auto itLayoutMap = layoutsMap_.begin();
    // Look for non-default.
    for (; itLayoutMap != layoutsMap_.end(); ++itLayoutMap)
        if (itLayoutMap->first.find(passType) != itLayoutMap->first.end()) break;
    // Look for default if a non-default wasn't found.
    if (itLayoutMap == layoutsMap_.end()) {
        itLayoutMap = layoutsMap_.begin();
        for (; itLayoutMap != layoutsMap_.end(); ++itLayoutMap, 1)
            if (itLayoutMap->first == Uniform::RENDER_PASS_ALL_SET) break;
    }
    assert(itLayoutMap != layoutsMap_.end());

    // Get or create the bind data for the pass type.
    auto itBindData = bindDataMap_.begin();
    for (; itBindData != bindDataMap_.end(); std::advance(itBindData, 1)) {
        // Get the set difference of the iterator pass type keys.
        std::set_difference(itBindData->first.begin(), itBindData->first.end(), itLayoutMap->first.begin(),
                            itLayoutMap->first.end(), std::inserter(tempSet, tempSet.begin()));

        // If the size of the difference is not the same as the bind data key then the
        // pass type needed is already in the bind data map.
        if (tempSet.size() != itBindData->first.size()) {
            // Compare the pipeline data to see if the pipeline bind data is compatible.
            bool isCompatible = true;
            for (const auto& bindDataPassType : tempSet) {
                // The set difference should filter out the ALL_ENUM. If it doesn't
                // I am not sure if the algorithm will work right.
                assert(bindDataPassType != RENDER_PASS::ALL_ENUM);
                if (!handler().passHandler().getPass(bindDataPassType)->comparePipelineData(pPass)) {
                    isCompatible = false;
                    break;
                }
            }

            if (isCompatible) {
                // If the pass type is already in the key then return the value
                if (itBindData->first.count(passType) == 0) return itBindData->second;
                // The bind data is compatible so extract the bind data, update the key with the new pass type, and
                // add the extracted element back into the map.
                assert(false);
                auto nh = bindDataMap_.extract(itBindData->first);
                nh.key().insert(passType);
                auto& key = nh.key();
                return bindDataMap_.insert(std::move(nh)).node.mapped();
            }
        }
    }

    // No compatible bind data was found so make a new one
    auto key = itLayoutMap->first;
    key.insert(passType);
    bindDataMap_.insert({key, makeBindData(itLayoutMap->second.pipelineLayout)});
    return bindDataMap_.at(key);
}

void Pipeline::Base::destroy() {
    const auto& dev = handler().shell().context().dev;
    for (const auto& [passTypes, bindData] : bindDataMap_) {
        if (bindData->pipeline != VK_NULL_HANDLE)  //
            vkDestroyPipeline(dev, bindData->pipeline, nullptr);
    }
    bindDataMap_.clear();
    for (auto& [passTypes, layouts] : layoutsMap_) {
        if (layouts.pipelineLayout != VK_NULL_HANDLE)  //
            vkDestroyPipelineLayout(dev, layouts.pipelineLayout, nullptr);
    }
    layoutsMap_.clear();
}

void Pipeline::Base::getDynamicStateInfoResources(CreateInfoVkResources& createInfoRes) {
    // TODO: this is weird
    createInfoRes.dynamicStateInfo = {};
    memset(createInfoRes.dynamicStates, 0, sizeof(createInfoRes.dynamicStates));
    createInfoRes.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    createInfoRes.dynamicStateInfo.pNext = nullptr;
    createInfoRes.dynamicStateInfo.pDynamicStates = createInfoRes.dynamicStates;
    createInfoRes.dynamicStateInfo.dynamicStateCount = 0;
}

void Pipeline::Base::getInputAssemblyInfoResources(CreateInfoVkResources& createInfoRes) {
    auto range = VERTEX_MAP.equal_range(VERTEX::COLOR);
    if (range.first != range.second && range.first->second.count(TYPE)) {
        GetDefaultColorInputAssemblyInfoResources(createInfoRes);
        return;
    }
    range = VERTEX_MAP.equal_range(VERTEX::TEXTURE);
    if (range.first != range.second && range.first->second.count(TYPE)) {
        GetDefaultTextureInputAssemblyInfoResources(createInfoRes);
        return;
    }
    assert(false);
    throw std::runtime_error("Unhandled type for input assembly. Override or add to this function?");
}

void Pipeline::Base::getRasterizationStateInfoResources(CreateInfoVkResources& createInfoRes) {
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

void Pipeline::Base::getMultisampleStateInfoResources(CreateInfoVkResources& createInfoRes) {
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

void Pipeline::Base::getBlendInfoResources(CreateInfoVkResources& createInfoRes) {
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

void Pipeline::Base::getViewportStateInfoResources(CreateInfoVkResources& createInfoRes) {
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

void Pipeline::Base::getDepthInfoResources(CreateInfoVkResources& createInfoRes) {
    createInfoRes.depthStencilStateInfo = {};
    createInfoRes.depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    createInfoRes.depthStencilStateInfo.pNext = nullptr;
    createInfoRes.depthStencilStateInfo.flags = 0;
    // This is set in overridePipelineCreateInfo now.
    // createInfoRes.depthStencilStateInfo.depthTestEnable = handler().settings().include_depth;
    // createInfoRes.depthStencilStateInfo.depthWriteEnable = handler().settings().include_depth;
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

void Pipeline::Base::getTesselationInfoResources(CreateInfoVkResources& createInfoRes) {
    createInfoRes.tessellationStateInfo = {};
}

//  LINE
void Pipeline::Default::Line::getInputAssemblyInfoResources(CreateInfoVkResources& createInfoRes) {
    GetDefaultColorInputAssemblyInfoResources(createInfoRes);
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
}

// CUBE
void Pipeline::Default::Cube::getDepthInfoResources(CreateInfoVkResources& createInfoRes) {
    Pipeline::Base::getDepthInfoResources(createInfoRes);
    createInfoRes.depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
}

// BLINN PHONG TEXTURE CULL NONE
void Pipeline::BP::TextureCullNone::getRasterizationStateInfoResources(CreateInfoVkResources& createInfoRes) {
    Pipeline::Base::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.cullMode = VK_CULL_MODE_NONE;
}
