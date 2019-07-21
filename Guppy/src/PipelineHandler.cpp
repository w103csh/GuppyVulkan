
#include "PipelineHandler.h"

#include "ConstantsAll.h"
#include "Material.h"
#include "Mesh.h"
#include "Parallax.h"
#include "PBR.h"
#include "Pipeline.h"
#include "ScreenSpace.h"
#include "Shell.h"
// HANDLERS
#include "ComputeHandler.h"
#include "DescriptorHandler.h"
#include "TextureHandler.h"
#include "RenderPassHandler.h"
#include "SceneHandler.h"
#include "ShaderHandler.h"

namespace {

void setBase(const VkPipeline& pipeline, VkGraphicsPipelineCreateInfo& info, bool& hasBase) {
    if (!hasBase && pipeline != VK_NULL_HANDLE) {
        // Setup for derivatives...
        info.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
        info.basePipelineHandle = pipeline;
        info.basePipelineIndex = -1;
        hasBase = true;
    }
}

void setBase(const VkPipeline& pipeline, VkComputePipelineCreateInfo& info, bool& hasBase) {
    if (!hasBase && pipeline != VK_NULL_HANDLE) {
        // Setup for derivatives...
        info.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
        info.basePipelineHandle = pipeline;
        info.basePipelineIndex = -1;
        hasBase = true;
    }
}

}  // namespace

Pipeline::Handler::Handler(Game* pGame) : Game::Handler(pGame), cache_(VK_NULL_HANDLE), maxPushConstantsSize_(UINT32_MAX) {
    for (const auto& type : ALL) {
        // clang-format off
        switch (type) {
            case PIPELINE::TRI_LIST_COLOR:                  pPipelines_.emplace_back(std::make_unique<Default::TriListColor>(std::ref(*this))); break;
            case PIPELINE::LINE:                            pPipelines_.emplace_back(std::make_unique<Default::Line>(std::ref(*this))); break;
            case PIPELINE::TRI_LIST_TEX:                    pPipelines_.emplace_back(std::make_unique<Default::TriListTexture>(std::ref(*this))); break;
            case PIPELINE::CUBE:                            pPipelines_.emplace_back(std::make_unique<Default::Cube>(std::ref(*this))); break;
            case PIPELINE::PBR_COLOR:                       pPipelines_.emplace_back(std::make_unique<PBR::Color>(std::ref(*this))); break;
            case PIPELINE::PBR_TEX:                         pPipelines_.emplace_back(std::make_unique<PBR::Texture>(std::ref(*this))); break;
            case PIPELINE::BP_TEX_CULL_NONE:                pPipelines_.emplace_back(std::make_unique<BP::TextureCullNone>(std::ref(*this))); break;
            case PIPELINE::PARALLAX_SIMPLE:                 pPipelines_.emplace_back(std::make_unique<Parallax::Simple>(std::ref(*this))); break;
            case PIPELINE::PARALLAX_STEEP:                  pPipelines_.emplace_back(std::make_unique<Parallax::Steep>(std::ref(*this))); break;
            case PIPELINE::SCREEN_SPACE_DEFAULT:            pPipelines_.emplace_back(std::make_unique<ScreenSpace::Default>(std::ref(*this))); break;
            case PIPELINE::SCREEN_SPACE_COMPUTE_DEFAULT:    pPipelines_.emplace_back(std::make_unique<ScreenSpace::ComputeDefault>(std::ref(*this))); break;
            default: assert(false);  // add new pipelines here
        }
        // clang-format on
        assert(pPipelines_.back()->TYPE == type);
    }
    // Validate the list of instantiated pipelines.
    assert(pPipelines_.size() == ALL.size());
    for (const auto& pPipeline : pPipelines_) assert(pPipeline != nullptr);
}

void Pipeline::Handler::reset() {
    // PIPELINE
    pipelineBindDataMap_.clear();
    for (auto& pPipeline : pPipelines_) pPipeline->destroy();
    // CACHE
    if (cache_ != VK_NULL_HANDLE) vkDestroyPipelineCache(shell().context().dev, cache_, nullptr);

    maxPushConstantsSize_ = 0;
}

Uniform::offsetsMap Pipeline::Handler::makeUniformOffsetsMap() {
    Uniform::offsetsMap map;
    for (const auto& pPipeline : pPipelines_) {
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
    for (auto& pPipeline : pPipelines_) pPipeline->init();
}

std::vector<VkPushConstantRange> Pipeline::Handler::getPushConstantRanges(
    const PIPELINE& pipelineType, const std::vector<PUSH_CONSTANT>& pushConstantTypes) const {
    // Make the ranges...
    std::vector<VkPushConstantRange> ranges;

    uint32_t offset = 0;
    for (auto& type : pushConstantTypes) {
        VkPushConstantRange range = {};

        // clang-format off
        switch (type) {
            case PUSH_CONSTANT::DEFAULT:        range.size = sizeof(Pipeline::Default::PushConstant); break;
            case PUSH_CONSTANT::POST_PROCESS:   range.size = sizeof(::Compute::PostProcess::PushConstant); break;
            default: assert(false && "Unknown push constant"); exit(EXIT_FAILURE);
        }
        // clang-format on

        // shader stages
        range.stageFlags |= shaderHandler().getStageFlags(getPipeline(pipelineType)->SHADER_TYPES);

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

void Pipeline::Handler::createPipelineCache(VkPipelineCache& cache) {
    VkPipelineCacheCreateInfo pipeline_cache_info = {};
    pipeline_cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipeline_cache_info.initialDataSize = 0;
    pipeline_cache_info.pInitialData = nullptr;
    vk::assert_success(vkCreatePipelineCache(shell().context().dev, &pipeline_cache_info, nullptr, &cache));
}

void Pipeline::Handler::createPipeline(const std::string&& name, VkGraphicsPipelineCreateInfo& createInfo,
                                       VkPipeline& pipeline) {
    vk::assert_success(vkCreateGraphicsPipelines(shell().context().dev, cache_, 1, &createInfo, nullptr, &pipeline));

    if (settings().enable_debug_markers) {
        std::string markerName = name + " graphics pipline";
        ext::DebugMarkerSetObjectName(shell().context().dev, (uint64_t)pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                                      markerName.c_str());
    }
}

void Pipeline::Handler::createPipeline(const std::string&& name, VkComputePipelineCreateInfo& createInfo,
                                       VkPipeline& pipeline) {
    vk::assert_success(vkCreateComputePipelines(shell().context().dev, cache_, 1, &createInfo, nullptr, &pipeline));

    if (settings().enable_debug_markers) {
        std::string markerName = name + " compute pipline";
        ext::DebugMarkerSetObjectName(shell().context().dev, (uint64_t)pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                                      markerName.c_str());
    }
}

bool Pipeline::Handler::checkVertexPipelineMap(VERTEX key, PIPELINE value) const {
    for (const auto& type : Pipeline::VERTEX_MAP.at(key)) {
        if (type == value) return true;
    }
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
    VkGraphicsPipelineCreateInfo graphicsCreateInfo;
    VkComputePipelineCreateInfo computeCreateInfo;
    CreateInfoResources createInfoRes;

    auto it = set.begin();
    while (it != set.end()) {
        // Clear out memory
        graphicsCreateInfo = {};
        graphicsCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        graphicsCreateInfo.basePipelineIndex = 0;
        computeCreateInfo = {};
        computeCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        computeCreateInfo.basePipelineIndex = 0;
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
            createInfoRes.stagesInfo.clear();
            for (const auto& shaderType : pPipeline->SHADER_TYPES) {
                shaderHandler().getStagesInfo(shaderType, pPipeline->TYPE, it->second, createInfoRes.stagesInfo);
            }

            // TODO: This can be simplified.
            switch (pPipelineBindData->bindPoint) {
                // GRAPHICS
                case VK_PIPELINE_BIND_POINT_GRAPHICS: {
                    const auto& pPass = passHandler().getPass(it->second);

                    setBase(pPipelineBindData->pipeline, graphicsCreateInfo, hasBase);

                    graphicsCreateInfo.renderPass = pPass->pass;
                    graphicsCreateInfo.subpass = pPass->getSubpassId(pipelineType);
                    graphicsCreateInfo.layout = pPipelineBindData->layout;
                    pPipeline->setInfo(createInfoRes, &graphicsCreateInfo, nullptr);

                    // Give the render pass a chance to override default settings
                    pPass->overridePipelineCreateInfo(pipelineType, createInfoRes);

                    // Save the old pipeline for clean up if necessary
                    if (pPipelineBindData->pipeline != VK_NULL_HANDLE)
                        oldPipelines_.push_back({-1, pPipelineBindData->pipeline});

                    createPipeline(pPipeline->NAME + " " + std::to_string(static_cast<uint32_t>(it->second)),
                                   graphicsCreateInfo, pPipelineBindData->pipeline);

                    setBase(pPipelineBindData->pipeline, graphicsCreateInfo, hasBase);

                } break;
                // COMPUTE
                case VK_PIPELINE_BIND_POINT_COMPUTE: {
                    setBase(pPipelineBindData->pipeline, computeCreateInfo, hasBase);

                    computeCreateInfo.layout = pPipelineBindData->layout;
                    pPipeline->setInfo(createInfoRes, nullptr, &computeCreateInfo);

                    // Save the old pipeline for clean up if necessary
                    if (pPipelineBindData->pipeline != VK_NULL_HANDLE)
                        oldPipelines_.push_back({-1, pPipelineBindData->pipeline});

                    createPipeline(pPipeline->NAME + " " + std::to_string(static_cast<uint32_t>(it->second)),
                                   computeCreateInfo, pPipelineBindData->pipeline);

                    setBase(pPipelineBindData->pipeline, computeCreateInfo, hasBase);

                } break;
                default: {
                    assert(false);  // I think the only other types is ray tracing
                } break;
            }

            ++it;
        }
    }
}

void Pipeline::Handler::makeShaderInfoMap(Shader::infoMap& map) {
    for (const auto& pPipeline : pPipelines_) {
        const auto& shaderReplaceMap = pPipeline->getShaderTextReplaceInfoMap();
        for (const auto& shaderReplaceKeyValue : shaderReplaceMap) {
            for (const auto& shaderType : pPipeline->SHADER_TYPES) {
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

void Pipeline::Handler::getShaderStages(const std::set<PIPELINE>& pipelineTypes, VkShaderStageFlags& stages) {
    for (const auto& pPipeline : pPipelines_) {
        if (pipelineTypes.find(pPipeline->TYPE) == pipelineTypes.end()) continue;
        for (const auto& shaderType : pPipeline->SHADER_TYPES) {
            stages |= Shader::ALL.at(shaderType).stage;
        }
    }
}

void Pipeline::Handler::needsUpdate(const std::vector<SHADER> types) {
    for (const auto& pPipeline : pPipelines_) {
        for (const auto& shaderType : pPipeline->SHADER_TYPES) {
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
            vkDestroyPipeline(shell().context().dev, pair.second, nullptr);
            oldPipelines_.erase(oldPipelines_.begin() + i);
        } else if (pair.first == -1) {
            pair.first = static_cast<int>(frameIndex);
        }
    }
}

void Pipeline::Handler::update() {
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
