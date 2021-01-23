/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Pipeline.h"

#include <algorithm>
#include <variant>

#include <Common/Helpers.h>

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
    Vertex::Color::getInputDescriptions(createInfoRes);
    Instance::Obj3d::DATA::getInputDescriptions(createInfoRes);
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

void Pipeline::GetDefaultTextureInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    Vertex::Texture::getInputDescriptions(createInfoRes);
    Instance::Obj3d::DATA::getInputDescriptions(createInfoRes);
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

void Pipeline::GetDefaultScreenQuadInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    Vertex::Texture::getScreenQuadInputDescriptions(createInfoRes);
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

// BASE

Pipeline::Base::Base(Handler& handler, const vk::PipelineBindPoint&& bindPoint, const CreateInfo* pCreateInfo)
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
    assert(!std::visit(IsAll{}, TYPE));
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
                    if (!(foundTexture || i > 1)) {
                        std::string sMsg = "Could not find texture(s) \"" + textureId + "\" for descriptor set validation";
                        handler().shell().log(Shell::LogPriority::LOG_WARN, sMsg.c_str());
                    }
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
            auto it = layoutsMap_.find(helper.passTypes1);
            if (it == layoutsMap_.end()) {
                auto insertPair = layoutsMap_.insert(std::pair<std::set<PASS>, Layouts>{
                    helper.passTypes1,
                    {{}, {}},
                });
                assert(insertPair.second);
                it = insertPair.first;
            }
            it->second.descSetLayouts.push_back(helper.pResource->layout);
        }
    }

    for (const auto& keyValue : shaderTextReplaceInfoMap_) assert(keyValue.second.size() == DESCRIPTOR_SET_TYPES.size());
    assert(layoutsMap_.size() == helpers.front().size());
    for (const auto& keyValue : layoutsMap_) assert(keyValue.second.descSetLayouts.size() == DESCRIPTOR_SET_TYPES.size());
}

std::shared_ptr<Pipeline::BindData> Pipeline::Base::makeBindData(const vk::PipelineLayout& layout) {
    vk::ShaderStageFlags pushConstantStages = {};
    for (const auto& range : pushConstantRanges_) pushConstantStages |= range.stageFlags;
    return std::shared_ptr<BindData>(new BindData{
        TYPE,
        BIND_POINT,
        layout,
        {},
        pushConstantStages,
        PUSH_CONSTANT_TYPES,
        false,
    });
}

void Pipeline::Base::makePipelineLayouts() {
    const auto& ctx = handler().shell().context();
    // TODO: destroy layouts if they already exist

    prepareDescriptorSetInfo();

    for (auto& keyValue : layoutsMap_) {
        // PUSH CONSTANTS
        if (PUSH_CONSTANT_TYPES.size()) pushConstantRanges_ = handler().getPushConstantRanges(TYPE, PUSH_CONSTANT_TYPES);

        vk::PipelineLayoutCreateInfo layoutInfo = {};
        layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges_.size());
        layoutInfo.pPushConstantRanges = pushConstantRanges_.data();
        layoutInfo.setLayoutCount = static_cast<uint32_t>(keyValue.second.descSetLayouts.size());
        layoutInfo.pSetLayouts = keyValue.second.descSetLayouts.data();

        keyValue.second.pipelineLayout = ctx.dev.createPipelineLayout(layoutInfo, ctx.pAllocator);
        // ctx.dbg.setMarkerName(keyValue.second.pipelineLayout, NAME.c_str());
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
        for (; itLayoutMap != layoutsMap_.end(); ++itLayoutMap)
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
                auto nh = bindDataMap_.extract(itBindData->first);
                nh.key().insert(passType);
                auto inserted_return_type = bindDataMap_.insert(std::move(nh));
                assert(inserted_return_type.inserted);
                return inserted_return_type.position->second;
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
    const auto& ctx = handler().shell().context();
    for (const auto& [passTypes, bindData] : bindDataMap_) {
        if (bindData->pipeline)  //
            ctx.dev.destroyPipeline(bindData->pipeline, ctx.pAllocator);
    }
    bindDataMap_.clear();
    for (auto& [passTypes, layouts] : layoutsMap_) {
        if (layouts.pipelineLayout)  //
            ctx.dev.destroyPipelineLayout(layouts.pipelineLayout, ctx.pAllocator);
    }
    layoutsMap_.clear();
}

// COMPUTE

Pipeline::Compute::Compute(Pipeline::Handler& handler, const Pipeline::CreateInfo* pCreateInfo)
    : Base(handler, vk::PipelineBindPoint::eCompute, pCreateInfo), localSize_(pCreateInfo->localSize) {
    assert(std::visit(IsCompute{}, TYPE));
}

void Pipeline::Compute::setInfo(CreateInfoResources& createInfoRes, vk::GraphicsPipelineCreateInfo* pGraphicsInfo,
                                vk::ComputePipelineCreateInfo* pComputeInfo) {
    // Gather info from derived classes...
    getShaderStageInfoResources(createInfoRes);

    assert(pGraphicsInfo == nullptr && pComputeInfo != nullptr);
    pComputeInfo->flags = vk::PipelineCreateFlagBits::eAllowDerivatives;
    // SHADER
    // I believe these asserts should always be the case. Not sure though.
    assert(createInfoRes.shaderStageInfos.size() == 1);
    assert(createInfoRes.shaderStageInfos.front().stage == vk::ShaderStageFlagBits::eCompute);
    pComputeInfo->stage = createInfoRes.shaderStageInfos.front();
}

//  GRAPHICS

Pipeline::Graphics::Graphics(Pipeline::Handler& handler, const Pipeline::CreateInfo* pCreateInfo)
    : Base(handler, vk::PipelineBindPoint::eGraphics, pCreateInfo) {
    assert(std::visit(IsGraphics{}, TYPE));
}

void Pipeline::Graphics::setInfo(CreateInfoResources& createInfoRes, vk::GraphicsPipelineCreateInfo* pGraphicsInfo,
                                 vk::ComputePipelineCreateInfo* pComputeInfo) {
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
    // createInfoRes.vertexInputBindDivDescs.clear();

    // Gather info from derived classes...
    getBlendInfoResources(createInfoRes);
    getDepthInfoResources(createInfoRes);
    getInputAssemblyInfoResources(createInfoRes);
    getMultisampleStateInfoResources(createInfoRes);
    getRasterizationStateInfoResources(createInfoRes);
    getShaderStageInfoResources(createInfoRes);
    getTesselationInfoResources(createInfoRes);
    getViewportStateInfoResources(createInfoRes);
    getDynamicStateInfoResources(createInfoRes);

    // PIPELINE
    pGraphicsInfo->flags = vk::PipelineCreateFlagBits::eAllowDerivatives;
    // INPUT ASSEMBLY
    pGraphicsInfo->pInputAssemblyState = &createInfoRes.inputAssemblyStateInfo;
    // if (createInfoRes.vertexInputBindDivDescs.size()) {
    //    createInfoRes.vertexInputDivInfo = {};
    //    createInfoRes.vertexInputDivInfo.vertexBindingDivisorCount =
    //        static_cast<uint32_t>(createInfoRes.vertexInputBindDivDescs.size());
    //    createInfoRes.vertexInputDivInfo.pVertexBindingDivisors = createInfoRes.vertexInputBindDivDescs.data();
    //    createInfoRes.vertexInputStateInfo.pNext = &createInfoRes.vertexInputDivInfo;
    //}
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
    pGraphicsInfo->pTessellationState = createInfoRes.useTessellationInfo ? &createInfoRes.tessellationStateInfo : nullptr;
    pGraphicsInfo->pVertexInputState = &createInfoRes.vertexInputStateInfo;
    pGraphicsInfo->pViewportState = &createInfoRes.viewportStateInfo;
}

void Pipeline::Graphics::getDynamicStateInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.dynamicStateInfo = vk::PipelineDynamicStateCreateInfo{};
    createInfoRes.dynamicStateInfo.pDynamicStates = createInfoRes.dynamicStates.data();
    createInfoRes.dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(createInfoRes.dynamicStates.size());
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
    createInfoRes.rasterizationStateInfo = vk::PipelineRasterizationStateCreateInfo{};
    createInfoRes.rasterizationStateInfo.polygonMode = vk::PolygonMode::eFill;
    createInfoRes.rasterizationStateInfo.cullMode = vk::CullModeFlagBits::eBack;
    createInfoRes.rasterizationStateInfo.frontFace = vk::FrontFace::eCounterClockwise;
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

    createInfoRes.multisampleStateInfo = vk::PipelineMultisampleStateCreateInfo{};
    createInfoRes.multisampleStateInfo.rasterizationSamples = ctx.samples;
    // enable sample shading in the pipeline (sampling for fragment interiors)
    createInfoRes.multisampleStateInfo.sampleShadingEnable = settings.enableSampleShading;
    // min fraction for sample shading; closer to one is smooth
    createInfoRes.multisampleStateInfo.minSampleShading = settings.enableSampleShading ? MIN_SAMPLE_SHADING : 0.0f;
    createInfoRes.multisampleStateInfo.pSampleMask = nullptr;             // Optional
    createInfoRes.multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
    createInfoRes.multisampleStateInfo.alphaToOneEnable = VK_FALSE;       // Optional
}

void Pipeline::Graphics::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.blendAttachmentStates.push_back({});
    createInfoRes.blendAttachmentStates.back().colorWriteMask =
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
        vk::ColorComponentFlagBits::eA;
    // blend_attachment.blendEnable = VK_FALSE;
    // blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;              // Optional
    // blend_attachment.colorBlendOp = vk::BlendOp::eAdd;              // Optional
    // blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;   // Optional
    // blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;  // Optional
    // blend_attachment.srcColorBlendFactor = vk::BlendFactor::eOne;   // Optional
    // blend_attachment.dstColorBlendFactor = vk::BlendFactor::eZero;  // Optional
    // common setup
    createInfoRes.blendAttachmentStates.back().blendEnable = VK_TRUE;
    createInfoRes.blendAttachmentStates.back().srcColorBlendFactor = vk::BlendFactor::eSrcAlpha;
    createInfoRes.blendAttachmentStates.back().dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    createInfoRes.blendAttachmentStates.back().colorBlendOp = vk::BlendOp::eAdd;
    createInfoRes.blendAttachmentStates.back().srcAlphaBlendFactor = vk::BlendFactor::eOne;
    createInfoRes.blendAttachmentStates.back().dstAlphaBlendFactor = vk::BlendFactor::eZero;
    createInfoRes.blendAttachmentStates.back().alphaBlendOp = vk::BlendOp::eAdd;
    // createInfoRes.blendAttach.srcAlphaBlendFactor = vk::BlendFactor::eSrcAlpha;
    // createInfoRes.blendAttach.dstAlphaBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
    // createInfoRes.blendAttach.alphaBlendOp = vk::BlendOp::eAdd;

    createInfoRes.colorBlendStateInfo = vk::PipelineColorBlendStateCreateInfo{};
    createInfoRes.colorBlendStateInfo.attachmentCount = static_cast<uint32_t>(createInfoRes.blendAttachmentStates.size());
    createInfoRes.colorBlendStateInfo.pAttachments = createInfoRes.blendAttachmentStates.data();
    createInfoRes.colorBlendStateInfo.logicOpEnable = VK_FALSE;
    createInfoRes.colorBlendStateInfo.logicOp = vk::LogicOp::eCopy;  // What does this do?
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
    createInfoRes.viewportStateInfo = vk::PipelineViewportStateCreateInfo{};
#ifndef __ANDROID__
    // Viewports
    createInfoRes.viewportStateInfo.viewportCount = NUM_VIEWPORTS;
    for (auto i = 0; i < NUM_VIEWPORTS; i++)  //
        createInfoRes.dynamicStates.push_back(vk::DynamicState::eViewport);
    createInfoRes.viewportStateInfo.pViewports = nullptr;
    // Scissors
    createInfoRes.viewportStateInfo.scissorCount = NUM_SCISSORS;
    for (auto i = 0; i < NUM_SCISSORS; i++)  //
        createInfoRes.dynamicStates.push_back(vk::DynamicState::eScissor);
    createInfoRes.viewportStateInfo.pScissors = nullptr;
#else
    // TODO: this is outdated now...
    // Temporary disabling dynamic viewport on Android because some of drivers doesn't
    // support the feature.
    vk::Viewport viewports;
    vk::Viewport viewports;
    viewports.minDepth = 0.0f;
    viewports.maxDepth = 1.0f;
    viewports.x = 0;
    viewports.y = 0;
    viewports.width = info.width;
    viewports.height = info.height;
    vk::Rect2D scissor;
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
    createInfoRes.depthStencilStateInfo = vk::PipelineDepthStencilStateCreateInfo{};
    // This is set in overridePipelineCreateInfo now.
    // createInfoRes.depthStencilStateInfo.depthTestEnable = handler().settings().include_depth;
    // createInfoRes.depthStencilStateInfo.depthWriteEnable = handler().settings().include_depth;
    createInfoRes.depthStencilStateInfo.depthCompareOp = vk::CompareOp::eLess;
    createInfoRes.depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
    createInfoRes.depthStencilStateInfo.minDepthBounds = 0.0f;
    createInfoRes.depthStencilStateInfo.maxDepthBounds = 1.0f;
    createInfoRes.depthStencilStateInfo.stencilTestEnable = VK_FALSE;
    createInfoRes.depthStencilStateInfo.front = vk::StencilOpState{};
    createInfoRes.depthStencilStateInfo.back = vk::StencilOpState{};
    // dss.back.failOp = vk::StencilOp::eKeep; // ARE THESE IMPORTANT !!!
    // dss.back.passOp = vk::StencilOp::eKeep;
    // dss.back.compareOp = vk::CompareOp::eAlways;
    // dss.back.compareMask = 0;
    // dss.back.reference = 0;
    // dss.back.depthFailOp = vk::StencilOp::eKeep;
    // dss.back.writeMask = 0;
    // dss.front = ds.back;
}

void Pipeline::Graphics::getTesselationInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.useTessellationInfo = false;
    createInfoRes.tessellationStateInfo = vk::PipelineTessellationStateCreateInfo{};
}

//  LINE
void Pipeline::Default::Line::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    GetDefaultColorInputAssemblyInfoResources(createInfoRes);
    createInfoRes.inputAssemblyStateInfo.topology = vk::PrimitiveTopology::eLineList;
}

//  POINT
void Pipeline::Default::Point::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    GetDefaultColorInputAssemblyInfoResources(createInfoRes);
    createInfoRes.inputAssemblyStateInfo.topology = vk::PrimitiveTopology::ePointList;
}

// CUBE
void Pipeline::Default::Cube::getDepthInfoResources(CreateInfoResources& createInfoRes) {
    Graphics::getDepthInfoResources(createInfoRes);
    // This causes the non-skybox reflect/refact options to not work correctly. Unfortunately, you need to make two shaders
    // if you want it to work right !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    createInfoRes.depthStencilStateInfo.depthCompareOp = vk::CompareOp::eLessOrEqual;
}

// BLINN PHONG TEXTURE CULL NONE
void Pipeline::BP::TextureCullNone::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    Graphics::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.cullMode = vk::CullModeFlagBits::eNone;
}

// CUBE MAP

// COLOR
std::unique_ptr<Pipeline::Base> Pipeline::Default::MakeCubeMapColor(Pipeline::Handler& handler) {
    CreateInfo info = TRI_LIST_COLOR_CREATE_INFO;
    info.name = "Cube Map Color";
    info.type = GRAPHICS::CUBE_MAP_COLOR;
    info.shaderTypes = {SHADER::VERT_COLOR_CUBE_MAP, SHADER::GEOM_COLOR_CUBE_MAP, SHADER::COLOR_FRAG};
    info.descriptorSets.push_back(DESCRIPTOR_SET::CAMERA_CUBE_MAP);
    return std::make_unique<TriListColorCube>(handler, &info);
}
Pipeline::Default::TriListColorCube::TriListColorCube(Pipeline::Handler& handler, const CreateInfo* pCreateInfo)
    : TriListColor(handler, pCreateInfo) {}
void Pipeline::Default::TriListColorCube::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    TriListColor::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.frontFace = vk::FrontFace::eClockwise;
}

// LINE
std::unique_ptr<Pipeline::Base> Pipeline::Default::MakeCubeMapLine(Pipeline::Handler& handler) {
    CreateInfo info = LINE_CREATE_INFO;
    info.name = "Cube Map Line";
    info.type = GRAPHICS::CUBE_MAP_LINE;
    info.shaderTypes = {SHADER::VERT_COLOR_CUBE_MAP, SHADER::GEOM_COLOR_CUBE_MAP, SHADER::LINE_FRAG};
    info.descriptorSets.push_back(DESCRIPTOR_SET::CAMERA_CUBE_MAP);
    return std::make_unique<LineCube>(handler, &info);
}
Pipeline::Default::LineCube::LineCube(Pipeline::Handler& handler, const CreateInfo* pCreateInfo)
    : Line(handler, pCreateInfo) {}
void Pipeline::Default::LineCube::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    Line::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.frontFace = vk::FrontFace::eClockwise;
}

// POINT
std::unique_ptr<Pipeline::Base> Pipeline::Default::MakeCubeMapPoint(Pipeline::Handler& handler) {
    CreateInfo info = POINT_CREATE_INFO;
    info.name = "Cube Map Point";
    info.type = GRAPHICS::CUBE_MAP_PT;
    info.shaderTypes = {SHADER::VERT_PT_CUBE_MAP, SHADER::GEOM_PT_CUBE_MAP, SHADER::LINE_FRAG};
    info.descriptorSets.push_back(DESCRIPTOR_SET::CAMERA_CUBE_MAP);
    return std::make_unique<PointCube>(handler, &info);
}
Pipeline::Default::PointCube::PointCube(Pipeline::Handler& handler, const CreateInfo* pCreateInfo)
    : Point(handler, pCreateInfo) {}
void Pipeline::Default::PointCube::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    Point::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.frontFace = vk::FrontFace::eClockwise;
}

// TEXTURE
std::unique_ptr<Pipeline::Base> Pipeline::Default::MakeCubeMapTexture(Pipeline::Handler& handler) {
    CreateInfo info = TRI_LIST_TEX_CREATE_INFO;
    info.name = "Cube Map Texture";
    info.type = GRAPHICS::CUBE_MAP_TEX;
    info.shaderTypes = {SHADER::VERT_TEX_CUBE_MAP, SHADER::GEOM_TEX_CUBE_MAP, SHADER::TEX_FRAG};
    info.descriptorSets.push_back(DESCRIPTOR_SET::CAMERA_CUBE_MAP);
    return std::make_unique<TriListTextureCube>(handler, &info);
}
Pipeline::Default::TriListTextureCube::TriListTextureCube(Pipeline::Handler& handler, const CreateInfo* pCreateInfo)
    : TriListTexture(handler, pCreateInfo) {}
void Pipeline::Default::TriListTextureCube::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    TriListTexture::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.frontFace = vk::FrontFace::eClockwise;
}