
#include "Pipeline.h"

#include <algorithm>

#include "ConstantsAll.h"
#include "Instance.h"
#include "Vertex.h"
// HANDLERS
#include "ComputeHandler.h"
#include "DescriptorHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"
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

void Pipeline::GetDefaultScreenQuadInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    GetDefaultTextureInputAssemblyInfoResources(createInfoRes);
    // attributes
    createInfoRes.attrDescs.clear();
    Vertex::Texture::getScreenQuadAttributeDescriptions(createInfoRes.attrDescs);
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
}

// BASE

Pipeline::Base::Base(Handler& handler, const VkPipelineBindPoint&& bindPoint, const CreateInfo* pCreateInfo)
    : Handlee(handler),
      BIND_POINT(bindPoint),
      DESCRIPTOR_SET_TYPES(pCreateInfo->descriptorSets),
      NAME(pCreateInfo->name),
      PUSH_CONSTANT_TYPES(pCreateInfo->pushConstantTypes),
      TYPE(pCreateInfo->type),
      status_(STATUS::PENDING),
      descriptorOffsets_(pCreateInfo->uniformOffsets),
      isInitialized_(false),
      shaderTypes_(pCreateInfo->shaderTypes) {
    assert(TYPE != PIPELINE::ALL_ENUM);
    for (const auto& type : PUSH_CONSTANT_TYPES) assert(type != PUSH_CONSTANT::DONT_CARE);
}

void Pipeline::Base::init() {
    validatePipelineDescriptorSets();
    makePipelineLayouts();
    isInitialized_ = true;
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

bool Pipeline::Base::checkTextureStatus(const std::string& id) {
    const auto& pTexture = handler().textureHandler().getTexture(id);
    if (pTexture == nullptr) return false;
    if (pTexture->status != STATUS::READY) pendingTexturesOffsets_.push_back(pTexture->OFFSET);
    return true;
}

void Pipeline::Base::validatePipelineDescriptorSets() {
    for (const auto& setType : DESCRIPTOR_SET_TYPES) {
        const auto& descSet = handler().descriptorHandler().getDescriptorSet(setType);
        for (const auto& [key, bindingInfo] : descSet.getBindingMap()) {
            bool hasPipelineSampler = std::visit(Descriptor::IsPipelineImage{}, bindingInfo.descType);
            // Validate tuple
            if (hasPipelineSampler || bindingInfo.textureId.size()) {
                if (!hasPipelineSampler && !std::visit(Descriptor::IsSwapchainStorageImage{}, bindingInfo.descType)) {
                    // Makes sure its a number. Will be used as texture sampler offset.
                    assert(helpers::isNumber(bindingInfo.textureId));
                } else {
                    // Make sure its not empty. Will be used as texture id.
                    assert(bindingInfo.textureId.size());

                    // Check texture
                    const auto textureId = std::string(bindingInfo.textureId);
                    bool foundTexture = checkTextureStatus(textureId);
                    uint32_t i = 0;
                    if (!foundTexture) {
                        // If the texture was not found by the normal id then try indexed ids
                        do {
                            auto textureIdWithSuffix = textureId + Texture::Handler::getIdSuffix(i++);
                            foundTexture = checkTextureStatus(textureIdWithSuffix);
                        } while (foundTexture);
                    }
                    assert((foundTexture || i > 1) && "Couldn't find texture.");
                }
            }
        }
    }
    if (pendingTexturesOffsets_.size()) status_ = STATUS::PENDING_TEXTURE;
}

void Pipeline::Base::prepareDescriptorSetInfo() {
    // Get a list of active passes for the pipeline type.
    std::set<PASS> passTypes;
    handler().passHandler().getActivePassTypes(passTypes, TYPE);
    handler().computeHandler().getActivePassTypes(passTypes, TYPE);

    // Gather a culled list of resources.
    auto helpers = handler().descriptorHandler().getResourceHelpers(passTypes, TYPE, DESCRIPTOR_SET_TYPES);

    for (auto resourceIndex = 0; resourceIndex < helpers.front().size(); resourceIndex++) {
        // Loop through each layer of descriptor set resources at resourceIndex
        for (auto setIndex = 0; setIndex < helpers.size(); setIndex++) {
            auto& helper = helpers[setIndex][resourceIndex];
            const auto& descSet = handler().descriptorHandler().getDescriptorSet(helper.setType);

            // Add the culled sets to a shader replace map, for later shader text replacement.
            if (shaderTextReplaceInfoMap_.count(helper.passTypes1) == 0) {
                shaderTextReplaceInfoMap_.insert({helper.passTypes1, {}});
            }
            shaderTextReplaceInfoMap_.at(helper.passTypes1)
                .push_back({
                    descSet.MACRO_NAME,
                    setIndex,
                    descSet.getDescriptorOffsets(helper.resourceOffset),
                    helper.passTypes2,
                });

            // Add the culled sets to a layouts data structure that can be used to create
            // the pipeline layouts.
            if (layoutsMap_.count(helper.passTypes1) == 0) {
                layoutsMap_.insert(std::pair<std::set<PASS>, Layouts>{
                    helper.passTypes2,
                    {VK_NULL_HANDLE, {}},
                });
            }
            layoutsMap_.at(helper.passTypes2).descSetLayouts.push_back(helper.pResource->layout);
        }
    }

    for (const auto& keyValue : shaderTextReplaceInfoMap_) assert(keyValue.second.size() == DESCRIPTOR_SET_TYPES.size());
    assert(layoutsMap_.size() == helpers.front().size());
    for (const auto& keyValue : layoutsMap_) assert(keyValue.second.descSetLayouts.size() == DESCRIPTOR_SET_TYPES.size());
}

std::shared_ptr<Pipeline::BindData> Pipeline::Base::makeBindData(const VkPipelineLayout& layout) {
    VkPipelineStageFlags pushConstantStages = 0;
    for (const auto& range : pushConstantRanges_) pushConstantStages |= range.stageFlags;
    return std::shared_ptr<BindData>(new BindData{
        TYPE,
        BIND_POINT,
        layout,
        VK_NULL_HANDLE,
        pushConstantStages,
        PUSH_CONSTANT_TYPES,
        false,
    });
}

void Pipeline::Base::makePipelineLayouts() {
    // TODO: destroy layouts if they already exist

    prepareDescriptorSetInfo();

    for (auto& keyValue : layoutsMap_) {
        // PUSH CONSTANTS
        if (PUSH_CONSTANT_TYPES.size()) pushConstantRanges_ = handler().getPushConstantRanges(TYPE, PUSH_CONSTANT_TYPES);

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

const std::shared_ptr<Pipeline::BindData>& Pipeline::Base::getBindData(const PASS& passType) {
    std::set<PASS> tempSet;

    // Find the layout map element for the pass type.
    auto itLayoutMap = layoutsMap_.begin();
    // Look for non-default.
    for (; itLayoutMap != layoutsMap_.end(); ++itLayoutMap)
        if (itLayoutMap->first.find(passType) != itLayoutMap->first.end()) break;
    // Look for default if a non-default wasn't found.
    if (itLayoutMap == layoutsMap_.end()) {
        itLayoutMap = layoutsMap_.begin();
        for (; itLayoutMap != layoutsMap_.end(); ++itLayoutMap, 1)
            if (itLayoutMap->first == Uniform::PASS_ALL_SET) break;
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
                assert(bindDataPassType != PASS::ALL_ENUM);
                if (RenderPass::ALL.count(passType)) {
                    const auto& pPass = handler().passHandler().getPass(passType);
                    if (!handler().passHandler().getPass(bindDataPassType)->comparePipelineData(pPass)) {
                        isCompatible = false;
                        break;
                    }
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
    auto it = bindDataMap_.insert({key, makeBindData(itLayoutMap->second.pipelineLayout)});
    assert(it.second);
    return it.first->second;
}

void Pipeline::Base::replaceShaderType(const SHADER type, const SHADER replaceWithType) {
    assert(!isInitialized_ && type != replaceWithType);
    auto it = shaderTypes_.find(type);
    assert(it != shaderTypes_.end());
    shaderTypes_.erase(it);
    shaderTypes_.insert(replaceWithType);
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

// COMPUTE

void Pipeline::Compute::setInfo(CreateInfoResources& createInfoRes, VkGraphicsPipelineCreateInfo* pGraphicsInfo,
                                VkComputePipelineCreateInfo* pComputeInfo) {
    assert(pGraphicsInfo == nullptr && pComputeInfo != nullptr);
    pComputeInfo->sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pComputeInfo->flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    // SHADER
    // I believe these asserts should always be the case. Not sure though.
    assert(createInfoRes.shaderStageInfos.size() == 1);
    assert(createInfoRes.shaderStageInfos.front().stage == VK_SHADER_STAGE_COMPUTE_BIT);
    pComputeInfo->stage = createInfoRes.shaderStageInfos.front();
}

//  GRAPHICS

void Pipeline::Graphics::setInfo(CreateInfoResources& createInfoRes, VkGraphicsPipelineCreateInfo* pGraphicsInfo,
                                 VkComputePipelineCreateInfo* pComputeInfo) {
    assert(pGraphicsInfo != nullptr && pComputeInfo == nullptr);
    /*
        The idea here is that this can be overridden in a bunch of ways, or you
        can just use the easier and slower way of calling this function from
        derivers.
    */

    // TODO: where should this go????
    createInfoRes.blendAttachmentStates.clear();
    createInfoRes.bindDescs.clear();
    createInfoRes.attrDescs.clear();

    // Gather info from derived classes...
    getBlendInfoResources(createInfoRes);
    getDepthInfoResources(createInfoRes);
    getDynamicStateInfoResources(createInfoRes);
    getInputAssemblyInfoResources(createInfoRes);
    getMultisampleStateInfoResources(createInfoRes);
    getRasterizationStateInfoResources(createInfoRes);
    getShaderStageInfoResources(createInfoRes);
    getTesselationInfoResources(createInfoRes);
    getViewportStateInfoResources(createInfoRes);

    // PIPELINE
    pGraphicsInfo->sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pGraphicsInfo->flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    // INPUT ASSEMBLY
    pGraphicsInfo->pInputAssemblyState = &createInfoRes.inputAssemblyStateInfo;
    pGraphicsInfo->pVertexInputState = &createInfoRes.vertexInputStateInfo;
    // SHADER
    pGraphicsInfo->stageCount = static_cast<uint32_t>(createInfoRes.shaderStageInfos.size());
    pGraphicsInfo->pStages = createInfoRes.shaderStageInfos.data();
    // FIXED FUNCTION
    pGraphicsInfo->pColorBlendState = &createInfoRes.colorBlendStateInfo;
    pGraphicsInfo->pDepthStencilState = &createInfoRes.depthStencilStateInfo;
    pGraphicsInfo->pDynamicState = &createInfoRes.dynamicStateInfo;
    pGraphicsInfo->pInputAssemblyState = &createInfoRes.inputAssemblyStateInfo;
    pGraphicsInfo->pMultisampleState = &createInfoRes.multisampleStateInfo;
    pGraphicsInfo->pRasterizationState = &createInfoRes.rasterizationStateInfo;
    if (createInfoRes.tessellationStateInfo.sType == VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO)
        pGraphicsInfo->pTessellationState = &createInfoRes.tessellationStateInfo;
    else
        pGraphicsInfo->pTessellationState = nullptr;
    pGraphicsInfo->pVertexInputState = &createInfoRes.vertexInputStateInfo;
    pGraphicsInfo->pViewportState = &createInfoRes.viewportStateInfo;
}

void Pipeline::Graphics::getDynamicStateInfoResources(CreateInfoResources& createInfoRes) {
    // TODO: this is weird
    createInfoRes.dynamicStateInfo = {};
    memset(createInfoRes.dynamicStates, 0, sizeof(createInfoRes.dynamicStates));
    createInfoRes.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    createInfoRes.dynamicStateInfo.pNext = nullptr;
    createInfoRes.dynamicStateInfo.pDynamicStates = createInfoRes.dynamicStates;
    createInfoRes.dynamicStateInfo.dynamicStateCount = 0;
}

void Pipeline::Graphics::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
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
    range = VERTEX_MAP.equal_range(VERTEX::SCREEN_QUAD);
    if (range.first != range.second && range.first->second.count(TYPE)) {
        GetDefaultScreenQuadInputAssemblyInfoResources(createInfoRes);
        return;
    }
    assert(false);
    throw std::runtime_error("Unhandled type for input assembly. Override or add to this function?");
}

void Pipeline::Graphics::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.rasterizationStateInfo = {};
    createInfoRes.rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    createInfoRes.rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    createInfoRes.rasterizationStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    createInfoRes.rasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    /* If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far
     *  planes are clamped to them as opposed to discarding them. This is useful in some special
     *  cases like shadow maps. Using this requires enabling a GPU feature.
     */
    createInfoRes.rasterizationStateInfo.depthClampEnable = VK_FALSE;
    createInfoRes.rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
    createInfoRes.rasterizationStateInfo.depthBiasEnable = VK_FALSE;
    createInfoRes.rasterizationStateInfo.depthBiasConstantFactor = 0.0f;
    createInfoRes.rasterizationStateInfo.depthBiasClamp = 0.0f;
    createInfoRes.rasterizationStateInfo.depthBiasSlopeFactor = 0.0f;
    /* The lineWidth member is straightforward, it describes the thickness of lines in terms of
     *  number of fragments. The maximum line width that is supported depends on the hardware and
     *  any line thicker than 1.0f requires you to enable the wideLines GPU feature.
     */
    createInfoRes.rasterizationStateInfo.lineWidth = 1.0f;
}

void Pipeline::Graphics::getMultisampleStateInfoResources(CreateInfoResources& createInfoRes) {
    auto& settings = handler().settings();
    auto& ctx = handler().shell().context();

    createInfoRes.multisampleStateInfo = {};
    createInfoRes.multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    createInfoRes.multisampleStateInfo.rasterizationSamples = ctx.samples;
    // enable sample shading in the pipeline (sampling for fragment interiors)
    createInfoRes.multisampleStateInfo.sampleShadingEnable = settings.enable_sample_shading;
    // min fraction for sample shading; closer to one is smooth
    createInfoRes.multisampleStateInfo.minSampleShading = settings.enable_sample_shading ? MIN_SAMPLE_SHADING : 0.0f;
    createInfoRes.multisampleStateInfo.pSampleMask = nullptr;             // Optional
    createInfoRes.multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
    createInfoRes.multisampleStateInfo.alphaToOneEnable = VK_FALSE;       // Optional
}

void Pipeline::Graphics::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.blendAttachmentStates.push_back({});
    createInfoRes.blendAttachmentStates.back().colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // blend_attachment.blendEnable = VK_FALSE;
    // blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional
    // blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
    // blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    // blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    // blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    // blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    // common setup
    createInfoRes.blendAttachmentStates.back().blendEnable = VK_TRUE;
    createInfoRes.blendAttachmentStates.back().srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    createInfoRes.blendAttachmentStates.back().dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    createInfoRes.blendAttachmentStates.back().colorBlendOp = VK_BLEND_OP_ADD;
    createInfoRes.blendAttachmentStates.back().srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    createInfoRes.blendAttachmentStates.back().dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    createInfoRes.blendAttachmentStates.back().alphaBlendOp = VK_BLEND_OP_ADD;
    // createInfoRes.blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    // createInfoRes.blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // createInfoRes.blendAttach.alphaBlendOp = VK_BLEND_OP_ADD;

    createInfoRes.colorBlendStateInfo = {};
    createInfoRes.colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    createInfoRes.colorBlendStateInfo.attachmentCount = static_cast<uint32_t>(createInfoRes.blendAttachmentStates.size());
    createInfoRes.colorBlendStateInfo.pAttachments = createInfoRes.blendAttachmentStates.data();
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

void Pipeline::Graphics::getViewportStateInfoResources(CreateInfoResources& createInfoRes) {
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

void Pipeline::Graphics::getDepthInfoResources(CreateInfoResources& createInfoRes) {
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

void Pipeline::Graphics::getTesselationInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.tessellationStateInfo = {};
}

//  LINE
void Pipeline::Default::Line::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    GetDefaultColorInputAssemblyInfoResources(createInfoRes);
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
}

// CUBE
void Pipeline::Default::Cube::getDepthInfoResources(CreateInfoResources& createInfoRes) {
    Graphics::getDepthInfoResources(createInfoRes);
    createInfoRes.depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
}

// BLINN PHONG TEXTURE CULL NONE
void Pipeline::BP::TextureCullNone::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    Graphics::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.cullMode = VK_CULL_MODE_NONE;
}
