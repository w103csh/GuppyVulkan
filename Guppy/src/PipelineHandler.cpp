/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "PipelineHandler.h"

#include "Cloth.h"
#include "ConstantsAll.h"
#include "Deferred.h"
#include "FFT.h"
#include "Geometry.h"
#include "HeightFieldFluid.h"
#include "Material.h"
#include "Mesh.h"
#include "Ocean.h"
#include "Parallax.h"
#include "Particle.h"
#include "PBR.h"
#include "Pipeline.h"
#include "ScreenSpace.h"
#include "Shadow.h"
#include "Shell.h"
#include "Tessellation.h"
// HANDLERS
#include "ComputeHandler.h"
#include "DescriptorHandler.h"
#include "TextureHandler.h"
#include "RenderPassHandler.h"
#include "SceneHandler.h"
#include "ShaderHandler.h"

namespace {

template <typename TCreateInfo>
void setBase(const vk::Pipeline& pipeline, TCreateInfo& info, bool& hasBase) {
    if (!hasBase && pipeline) {
        // Setup for derivatives...
        info.flags = vk::PipelineCreateFlagBits::eDerivative;
        info.basePipelineHandle = pipeline;
        info.basePipelineIndex = -1;
        hasBase = true;
    }
}

constexpr bool hasAdjacencyTopology(const vk::GraphicsPipelineCreateInfo& info) {
    return info.pInputAssemblyState->topology == vk::PrimitiveTopology::eLineListWithAdjacency ||
           info.pInputAssemblyState->topology == vk::PrimitiveTopology::eLineStripWithAdjacency ||
           info.pInputAssemblyState->topology == vk::PrimitiveTopology::eTriangleListWithAdjacency ||
           info.pInputAssemblyState->topology == vk::PrimitiveTopology::eTriangleStripWithAdjacency;
}

}  // namespace

Pipeline::Handler::Handler(Game* pGame) : Game::Handler(pGame), cache_(), maxPushConstantsSize_(UINT32_MAX) {
    for (const auto& type : ALL) {
        assert(pPipelines_.count(type) == 0);
        std::pair<std::map<PIPELINE, std::unique_ptr<Pipeline::Base>>::iterator, bool> insertPair;
        if (std::visit(IsGraphics{}, type)) {
            // clang-format off
            switch (std::visit(GetGraphics{}, type)) {
                case GRAPHICS::TRI_LIST_COLOR:                  insertPair = pPipelines_.insert({type, std::make_unique<Default::TriListColor>(std::ref(*this))}); break;
                case GRAPHICS::LINE:                            insertPair = pPipelines_.insert({type, std::make_unique<Default::Line>(std::ref(*this))}); break;
                case GRAPHICS::POINT:                           insertPair = pPipelines_.insert({type, std::make_unique<Default::Point>(std::ref(*this))}); break;
                case GRAPHICS::TRI_LIST_TEX:                    insertPair = pPipelines_.insert({type, std::make_unique<Default::TriListTexture>(std::ref(*this))}); break;
                case GRAPHICS::CUBE:                            insertPair = pPipelines_.insert({type, std::make_unique<Default::Cube>(std::ref(*this))}); break;
                case GRAPHICS::CUBE_MAP_COLOR:                  insertPair = pPipelines_.insert({type, Default::MakeCubeMapColor(std::ref(*this))}); break;
                case GRAPHICS::CUBE_MAP_LINE:                   insertPair = pPipelines_.insert({type, Default::MakeCubeMapLine(std::ref(*this))}); break;
                case GRAPHICS::CUBE_MAP_PT:                     insertPair = pPipelines_.insert({type, Default::MakeCubeMapPoint(std::ref(*this))}); break;
                case GRAPHICS::CUBE_MAP_TEX:                    insertPair = pPipelines_.insert({type, Default::MakeCubeMapTexture(std::ref(*this))}); break;
                case GRAPHICS::PBR_COLOR:                       insertPair = pPipelines_.insert({type, std::make_unique<PBR::Color>(std::ref(*this))}); break;
                case GRAPHICS::PBR_TEX:                         insertPair = pPipelines_.insert({type, std::make_unique<PBR::Texture>(std::ref(*this))}); break;
                case GRAPHICS::BP_TEX_CULL_NONE:                insertPair = pPipelines_.insert({type, std::make_unique<BP::TextureCullNone>(std::ref(*this))}); break;
                case GRAPHICS::PARALLAX_SIMPLE:                 insertPair = pPipelines_.insert({type, std::make_unique<Parallax::Simple>(std::ref(*this))}); break;
                case GRAPHICS::PARALLAX_STEEP:                  insertPair = pPipelines_.insert({type, std::make_unique<Parallax::Steep>(std::ref(*this))}); break;
                case GRAPHICS::SCREEN_SPACE_DEFAULT:            insertPair = pPipelines_.insert({type, std::make_unique<ScreenSpace::Default>(std::ref(*this))}); break;
                case GRAPHICS::SCREEN_SPACE_HDR_LOG:            insertPair = pPipelines_.insert({type, std::make_unique<ScreenSpace::HdrLog>(std::ref(*this))}); break;
                case GRAPHICS::SCREEN_SPACE_BRIGHT:             insertPair = pPipelines_.insert({type, std::make_unique<ScreenSpace::Bright>(std::ref(*this))}); break;
                case GRAPHICS::SCREEN_SPACE_BLUR_A:             insertPair = pPipelines_.insert({type, std::make_unique<ScreenSpace::BlurA>(std::ref(*this))}); break;
                case GRAPHICS::SCREEN_SPACE_BLUR_B:             insertPair = pPipelines_.insert({type, std::make_unique<ScreenSpace::BlurB>(std::ref(*this))}); break;
                case GRAPHICS::DEFERRED_MRT_TEX:                insertPair = pPipelines_.insert({type, std::make_unique<Deferred::MRTTexture>(std::ref(*this))}); break;
                case GRAPHICS::DEFERRED_MRT_COLOR:              insertPair = pPipelines_.insert({type, std::make_unique<Deferred::MRTColor>(std::ref(*this))}); break;
                case GRAPHICS::DEFERRED_MRT_WF_COLOR:           insertPair = pPipelines_.insert({type, std::make_unique<Deferred::MRTColorWireframe>(std::ref(*this))}); break;
                case GRAPHICS::DEFERRED_MRT_PT:                 insertPair = pPipelines_.insert({type, std::make_unique<Deferred::MRTPoint>(std::ref(*this))}); break;
                case GRAPHICS::DEFERRED_MRT_LINE:               insertPair = pPipelines_.insert({type, std::make_unique<Deferred::MRTLine>(std::ref(*this))}); break;
                case GRAPHICS::DEFERRED_MRT_COLOR_RFL_RFR:      insertPair = pPipelines_.insert({type, std::make_unique<Deferred::MRTColorReflectRefract>(std::ref(*this))}); break;
                case GRAPHICS::DEFERRED_MRT_SKYBOX:             insertPair = pPipelines_.insert({type, std::make_unique<Deferred::MRTSkybox>(std::ref(*this))}); break;
                case GRAPHICS::DEFERRED_COMBINE:                insertPair = pPipelines_.insert({type, std::make_unique<Deferred::Combine>(std::ref(*this))}); break;
                case GRAPHICS::DEFERRED_SSAO:                   insertPair = pPipelines_.insert({type, std::make_unique<Deferred::SSAO>(std::ref(*this))}); break;
                case GRAPHICS::SHADOW_COLOR:                    insertPair = pPipelines_.insert({type, std::make_unique<Shadow::Color>(std::ref(*this))}); break;
                case GRAPHICS::SHADOW_TEX:                      insertPair = pPipelines_.insert({type, std::make_unique<Shadow::Texture>(std::ref(*this))}); break;
                case GRAPHICS::TESS_BEZIER_4_DEFERRED:          insertPair = pPipelines_.insert({type, std::make_unique<Tessellation::Bezier4Deferred>(std::ref(*this))}); break;
                case GRAPHICS::TESS_PHONG_TRI_COLOR_DEFERRED:   insertPair = pPipelines_.insert({type, std::make_unique<Tessellation::PhongTriColorDeferred>(std::ref(*this))}); break;
                case GRAPHICS::TESS_PHONG_TRI_COLOR_WF_DEFERRED:insertPair = pPipelines_.insert({type, Tessellation::MakePhongTriColorWireframeDeferred(std::ref(*this))}); break;
                case GRAPHICS::GEOMETRY_SILHOUETTE_DEFERRED:    insertPair = pPipelines_.insert({type, std::make_unique<Geometry::Silhouette>(std::ref(*this))}); break;
                case GRAPHICS::PRTCL_WAVE_DEFERRED:             insertPair = pPipelines_.insert({type, std::make_unique<Particle::Wave>(std::ref(*this))}); break;
                case GRAPHICS::PRTCL_FOUNTAIN_DEFERRED:         insertPair = pPipelines_.insert({type, std::make_unique<Particle::Fountain>(std::ref(*this))}); break;
                case GRAPHICS::PRTCL_FOUNTAIN_EULER_DEFERRED:   insertPair = pPipelines_.insert({type, std::make_unique<Particle::FountainEuler>(std::ref(*this))}); break;
                case GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER:       insertPair = pPipelines_.insert({type, std::make_unique<Particle::ShadowFountainEuler>(std::ref(*this))}); break;
                case GRAPHICS::PRTCL_ATTR_PT_DEFERRED:          insertPair = pPipelines_.insert({type, std::make_unique<Particle::AttractorPoint>(std::ref(*this))}); break;
                case GRAPHICS::PRTCL_CLOTH_DEFERRED:            insertPair = pPipelines_.insert({type, std::make_unique<Particle::Cloth>(std::ref(*this))}); break;
                case GRAPHICS::HFF_CLMN_DEFERRED:               insertPair = pPipelines_.insert({type, std::make_unique<HeightFieldFluid::Column>(std::ref(*this))}); break;
                case GRAPHICS::HFF_WF_DEFERRED:                 insertPair = pPipelines_.insert({type, std::make_unique<HeightFieldFluid::Wireframe>(std::ref(*this))}); break;
                case GRAPHICS::HFF_OCEAN_DEFERRED:              insertPair = pPipelines_.insert({type, std::make_unique<HeightFieldFluid::Ocean>(std::ref(*this))}); break;
                case GRAPHICS::OCEAN_WF_DEFERRED:               insertPair = pPipelines_.insert({type, std::make_unique<Ocean::Wireframe>(std::ref(*this))}); break;
                case GRAPHICS::OCEAN_SURFACE_DEFERRED:          insertPair = pPipelines_.insert({type, std::make_unique<Ocean::Surface>(std::ref(*this))}); break;
                default: assert(false);  // add new pipelines here
            }
            // clang-format on
        } else if (std::visit(IsCompute{}, type)) {
            // clang-format off
            switch (std::visit(GetCompute{}, type)) {
                case COMPUTE::SCREEN_SPACE_DEFAULT:     insertPair = pPipelines_.insert({type, std::make_unique<ScreenSpace::ComputeDefault>(std::ref(*this))}); break;
                case COMPUTE::PRTCL_EULER:              insertPair = pPipelines_.insert({type, std::make_unique<Particle::Euler>(std::ref(*this))}); break;
                case COMPUTE::PRTCL_ATTR:               insertPair = pPipelines_.insert({type, std::make_unique<Particle::AttractorCompute>(std::ref(*this))}); break;
                case COMPUTE::PRTCL_CLOTH:              insertPair = pPipelines_.insert({type, std::make_unique<Particle::ClothCompute>(std::ref(*this))}); break;
                case COMPUTE::PRTCL_CLOTH_NORM:         insertPair = pPipelines_.insert({type, std::make_unique<Particle::ClothNormalCompute>(std::ref(*this))}); break;
                case COMPUTE::HFF_HGHT:                 insertPair = pPipelines_.insert({type, std::make_unique<HeightFieldFluid::Height>(std::ref(*this))}); break;
                case COMPUTE::HFF_NORM:                 insertPair = pPipelines_.insert({type, std::make_unique<HeightFieldFluid::Normal>(std::ref(*this))}); break;
                case COMPUTE::FFT_ONE:                  insertPair = pPipelines_.insert({type, std::make_unique<FFT::OneComponent>(std::ref(*this))}); break;
                case COMPUTE::OCEAN_DISP:               insertPair = pPipelines_.insert({type, std::make_unique<Ocean::Dispersion>(std::ref(*this))}); break;
                case COMPUTE::OCEAN_FFT:                insertPair = pPipelines_.insert({type, std::make_unique<Ocean::FFT>(std::ref(*this))}); break;
                default: assert(false);  // add new pipelines here
            }
            // clang-format on
        }
        assert(insertPair.second);
        assert(insertPair.first->second != nullptr);
        assert(insertPair.first->first == type && insertPair.first->second->TYPE == type);
    }
    // Validate the list of instantiated pipelines.
    assert(pPipelines_.size() == ALL.size());
}

void Pipeline::Handler::reset() {
    // PIPELINE
    pipelineBindDataMap_.clear();
    for (auto& [type, pPipeline] : pPipelines_) pPipeline->destroy();
    // CACHE
    if (cache_) shell().context().dev.destroyPipelineCache(cache_, shell().context().pAllocator);

    maxPushConstantsSize_ = 0;
}

Uniform::offsetsMap Pipeline::Handler::makeUniformOffsetsMap() {
    Uniform::offsetsMap map;
    for (const auto& [type, pPipeline] : pPipelines_) {
        for (const auto& [descType, offsets] : pPipeline->getDescriptorOffsets().map()) {
            Uniform::offsetsMapKey key = {descType, pPipeline->TYPE};
            assert(map.count(key) == 0);
            map[key] = {{offsets, Uniform::PASS_ALL_SET}};
        }
    }
    return map;
}

void Pipeline::Handler::init() {
    reset();

    // PUSH CONSTANT
    maxPushConstantsSize_ =
        shell().context().physicalDevProps[shell().context().physicalDevIndex].properties.limits.maxPushConstantsSize;
    assert(maxPushConstantsSize_ != UINT32_MAX);

    // CACHE
    createPipelineCache(cache_);

    // PIPELINES
    for (auto& [type, pPipeline] : pPipelines_) pPipeline->init();
}

void Pipeline::Handler::tick() {
    if (needsUpdateSet_.empty()) return;

    pipelinePassSet updateSet;

    auto itUpdate = needsUpdateSet_.begin();
    auto itMap = pipelineBindDataMap_.begin();

    while (itUpdate != needsUpdateSet_.end() && itMap != pipelineBindDataMap_.end()) {
        while (itMap != pipelineBindDataMap_.end() && *itUpdate != itMap->first.first) ++itMap;
        while (itMap != pipelineBindDataMap_.end() && *itUpdate == itMap->first.first) {
            updateSet.insert({itMap->first.first, itMap->first.second});
            ++itMap;
        }
        ++itUpdate;
    }

    if (updateSet.empty()) return;

    createPipelines(updateSet);
    passHandler().updateBindData(updateSet);

    needsUpdateSet_.clear();
}

std::vector<vk::PushConstantRange> Pipeline::Handler::getPushConstantRanges(
    const PIPELINE& pipelineType, const std::vector<PUSH_CONSTANT>& pushConstantTypes) const {
    // Make the ranges...
    std::vector<vk::PushConstantRange> ranges;

    uint32_t offset = 0;
    for (auto& type : pushConstantTypes) {
        vk::PushConstantRange range = {};

        // clang-format off
        switch (type) {
            case PUSH_CONSTANT::DEFAULT:            range.size = sizeof(Pipeline::Default::PushConstant); break;
            case PUSH_CONSTANT::POST_PROCESS:       range.size = sizeof(::Compute::PostProcess::PushConstant); break;
            case PUSH_CONSTANT::DEFERRED:           range.size = sizeof(::Deferred::PushConstant); break;
            case PUSH_CONSTANT::PRTCL_EULER:        range.size = sizeof(::Particle::Euler::PushConstant); break;
            case PUSH_CONSTANT::HFF_COLUMN:         range.size = sizeof(HeightFieldFluid::Column::PushConstant); break;
            case PUSH_CONSTANT::FFT_ROW_COL_OFFSET: range.size = sizeof(::FFT::RowColumnOffset); break;
            default: assert(false && "Unknown push constant"); exit(EXIT_FAILURE);
        }
        // clang-format on

        // shader stages
        range.stageFlags |= shaderHandler().getStageFlags(getPipeline(pipelineType)->getShaderTypes());

        bool merge = false;
        for (auto& r : ranges) {
            if (r.stageFlags & range.stageFlags) {
                range.stageFlags |= r.stageFlags;
                range.size += r.size;
                merge = true;
                // TODO: try to merge ranges and stages...
            }
        }

        if (merge) {
            range.offset = 0;
            offset = range.size;
            ranges.clear();
        } else {
            range.offset = offset;
            offset += range.size;
        }

        ranges.push_back(range);

        // Validate that the range is within acceptable limits so far
        if (ranges.back().offset > maxPushConstantsSize_) assert(0 && "Too many push constants");
        // TODO: above should throw ??? or be handled !!!
    }

    return ranges;
}

void Pipeline::Handler::createPipelineCache(vk::PipelineCache& cache) {
    vk::PipelineCacheCreateInfo createInfo = {};
    createInfo.initialDataSize = 0;
    createInfo.pInitialData = nullptr;
    cache = shell().context().dev.createPipelineCache(createInfo, shell().context().pAllocator);
}

void Pipeline::Handler::createPipeline(const std::string&& name, vk::GraphicsPipelineCreateInfo& createInfo,
                                       vk::Pipeline& pipeline) {
    pipeline = shell().context().dev.createGraphicsPipeline(cache_, createInfo, shell().context().pAllocator);
    // shell().context().dbg.setMarkerName(pipeline, name.c_str());
}

void Pipeline::Handler::createPipeline(const std::string&& name, vk::ComputePipelineCreateInfo& createInfo,
                                       vk::Pipeline& pipeline) {
    pipeline = shell().context().dev.createComputePipeline(cache_, createInfo, shell().context().pAllocator);
    // shell().context().dbg.setMarkerName(pipeline, name.c_str());
}

bool Pipeline::Handler::checkVertexPipelineMap(VERTEX key, PIPELINE value) const {
    for (const auto& type : Pipeline::VERTEX_MAP.at(key))
        if (type == value) return true;
    if (key == VERTEX::TEXTURE)
        for (const auto& type : Pipeline::VERTEX_MAP.at(VERTEX::SCREEN_QUAD))
            if (type == value) return true;
    return false;
}

void Pipeline::Handler::initPipelines() {
    pipelinePassSet set;
    computeHandler().addPipelinePassPairs(set);
    passHandler().addPipelinePassPairs(set);

    createPipelines(set);

    computeHandler().updateBindData(set);
    passHandler().updateBindData(set);
}

void Pipeline::Handler::createPipelines(const pipelinePassSet& set) {
    vk::GraphicsPipelineCreateInfo graphicsCreateInfo;
    vk::ComputePipelineCreateInfo computeCreateInfo;
    CreateInfoResources createInfoRes;

    auto it = set.begin();
    while (it != set.end()) {
        // Clear out memory
        graphicsCreateInfo = vk::GraphicsPipelineCreateInfo{};
        computeCreateInfo = vk::ComputePipelineCreateInfo{};
        createInfoRes = {};

        auto pipelineType = it->first;
        auto& pPipeline = getPipeline(it->first);

        while (it != set.end() && it->first == pipelineType) {
            bool hasBase = false;

            // Get pipeline from map or create a map element
            pipelineBindDataMapKey key = {pipelineType, it->second};
            if (pipelineBindDataMap_.count(key) == 0) {
                pipelineBindDataMap_.insert({key, pPipeline->getBindData(it->second)});
            }

            const auto& pPipelineBindData = pipelineBindDataMap_.at(key);

            // SHADER
            {
                createInfoRes.shaderStageInfos.clear();
                createInfoRes.specializationMapEntries.clear();
                createInfoRes.specializationInfo.clear();
                for (const auto& shaderType : pPipeline->getShaderTypes()) {
                    shaderHandler().getStagesInfo(shaderType, pPipeline->TYPE, it->second, createInfoRes.shaderStageInfos);
                }
            }

            // TODO: This can be simplified.
            switch (pPipelineBindData->bindPoint) {
                // GRAPHICS
                case vk::PipelineBindPoint::eGraphics: {
                    const auto& pPass = passHandler().getPass(it->second);

                    setBase(pPipelineBindData->pipeline, graphicsCreateInfo, hasBase);

                    graphicsCreateInfo.renderPass = pPass->pass;
                    graphicsCreateInfo.subpass = pPass->getSubpassId(pipelineType);
                    graphicsCreateInfo.layout = pPipelineBindData->layout;
                    pPipeline->setInfo(createInfoRes, &graphicsCreateInfo, nullptr);

                    // Give the render pass a chance to override default settings
                    pPass->overridePipelineCreateInfo(pipelineType, createInfoRes);

                    if (hasAdjacencyTopology(graphicsCreateInfo)) {
                        pPipelineBindData->usesAdjacency = true;
                    }

                    // Save the old pipeline for clean up if necessary
                    if (pPipelineBindData->pipeline) oldPipelines_.push_back({-1, pPipelineBindData->pipeline});

                    createPipeline(pPipeline->NAME + " " + std::to_string(static_cast<uint32_t>(it->second)),
                                   graphicsCreateInfo, pPipelineBindData->pipeline);

                    setBase(pPipelineBindData->pipeline, graphicsCreateInfo, hasBase);

                } break;
                // COMPUTE
                case vk::PipelineBindPoint::eCompute: {
                    setBase(pPipelineBindData->pipeline, computeCreateInfo, hasBase);

                    computeCreateInfo.layout = pPipelineBindData->layout;
                    pPipeline->setInfo(createInfoRes, nullptr, &computeCreateInfo);

                    // Save the old pipeline for clean up if necessary
                    if (pPipelineBindData->pipeline) oldPipelines_.push_back({-1, pPipelineBindData->pipeline});

                    createPipeline(pPipeline->NAME + " " + std::to_string(static_cast<uint32_t>(it->second)),
                                   computeCreateInfo, pPipelineBindData->pipeline);

                    setBase(pPipelineBindData->pipeline, computeCreateInfo, hasBase);

                } break;
                default: {
                    assert(false);  // I think the only other types are ray tracing
                } break;
            }

            ++it;
        }
    }
}

void Pipeline::Handler::makeShaderInfoMap(Shader::infoMap& map) {
    for (const auto& [type, pPipeline] : pPipelines_) {
        const auto& shaderReplaceMap = pPipeline->getShaderTextReplaceInfoMap();
        for (const auto& shaderReplaceKeyValue : shaderReplaceMap) {
            for (const auto& shaderType : pPipeline->getShaderTypes()) {
                map.insert({
                    {
                        shaderType,
                        pPipeline->TYPE,
                        shaderReplaceKeyValue.first,
                    },
                    {shaderReplaceKeyValue.second, {}},
                });
            }
        }
    }
}

void Pipeline::Handler::getShaderStages(const std::set<PIPELINE>& pipelineTypes, vk::ShaderStageFlags& stages) {
    for (const auto& [type, pPipeline] : pPipelines_) {
        if (pipelineTypes.find(pPipeline->TYPE) == pipelineTypes.end()) continue;
        stages |= shaderHandler().getStageFlags(pPipeline->getShaderTypes());
    }
}

void Pipeline::Handler::needsUpdate(const std::vector<SHADER> types) {
    for (const auto& [key, pPipeline] : pPipelines_) {
        for (const auto& shaderType : pPipeline->getShaderTypes()) {
            for (const auto& type : types) {
                if (type == shaderType) needsUpdateSet_.insert(pPipeline->TYPE);
            }
        }
    }
}

// "frameIndex" of -1 means clean up regardless
void Pipeline::Handler::cleanup(int frameIndex) {
    if (oldPipelines_.empty()) return;

    for (uint8_t i = 0; i < oldPipelines_.size(); i++) {
        auto& pair = oldPipelines_[i];
        if (pair.first == frameIndex || frameIndex == -1) {
            shell().context().dev.destroyPipeline(pair.second, shell().context().pAllocator);
            oldPipelines_.erase(oldPipelines_.begin() + i);
        } else if (pair.first == -1) {
            pair.first = static_cast<int>(frameIndex);
        }
    }
}
