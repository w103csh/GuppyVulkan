
#include "PipelineHandler.h"

#include "Constants.h"
#include "Material.h"
#include "Mesh.h"
#include "Parallax.h"
#include "PBR.h"
#include "Pipeline.h"
#include "Shell.h"
// HANDLERS
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
}  // namespace

Pipeline::Handler::Handler(Game* pGame) : Game::Handler(pGame), cache_(VK_NULL_HANDLE), maxPushConstantsSize_(UINT32_MAX) {
    for (const auto& type : PIPELINE_ALL) {
        switch (type) {
            case PIPELINE::TRI_LIST_COLOR:
                pPipelines_.emplace_back(std::make_unique<Default::TriListColor>(std::ref(*this)));
                break;
            case PIPELINE::LINE:
                pPipelines_.emplace_back(std::make_unique<Default::Line>(std::ref(*this)));
                break;
            case PIPELINE::TRI_LIST_TEX:
                pPipelines_.emplace_back(std::make_unique<Default::TriListTexture>(std::ref(*this)));
                break;
            case PIPELINE::CUBE:
                pPipelines_.emplace_back(std::make_unique<Default::Cube>(std::ref(*this)));
                break;
            case PIPELINE::PBR_COLOR:
                pPipelines_.emplace_back(std::make_unique<PBR::Color>(std::ref(*this)));
                break;
            case PIPELINE::PBR_TEX:
                pPipelines_.emplace_back(std::make_unique<PBR::Texture>(std::ref(*this)));
                break;
            case PIPELINE::BP_TEX_CULL_NONE:
                pPipelines_.emplace_back(std::make_unique<BP::TextureCullNone>(std::ref(*this)));
                break;
            case PIPELINE::PARALLAX_SIMPLE:
                pPipelines_.emplace_back(std::make_unique<Parallax::Simple>(std::ref(*this)));
                break;
            case PIPELINE::PARALLAX_STEEP:
                pPipelines_.emplace_back(std::make_unique<Parallax::Steep>(std::ref(*this)));
                break;
            default:
                assert(false);  // add new pipelines here
        }
        assert(pPipelines_.back()->TYPE == type);
    }
    // Validate the list of instantiated pipelines.
    assert(pPipelines_.size() == PIPELINE_ALL.size());
    for (const auto& pPipeline : pPipelines_) assert(pPipeline != nullptr);
}

void Pipeline::Handler::reset() {
    // PIPELINE
    for (auto& pPipeline : pPipelines_) pPipeline->destroy();
    for (auto& keyValue : pipelineMap_) vkDestroyPipeline(shell().context().dev, keyValue.second, nullptr);
    // CACHE
    if (cache_ != VK_NULL_HANDLE) vkDestroyPipelineCache(shell().context().dev, cache_, nullptr);

    maxPushConstantsSize_ = 0;
}

void Pipeline::Handler::init() {
    reset();

    // PUSH CONSTANT
    maxPushConstantsSize_ =
        shell().context().physical_dev_props[shell().context().physical_dev_index].properties.limits.maxPushConstantsSize;
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

        switch (type) {
            case PUSH_CONSTANT::DEFAULT:
                range.size = sizeof(Pipeline::Default::PushConstant);
                break;
            default:
                assert(false && "Unknown push constant");
        }

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
        std::string markerName = name + " pipline";
        ext::DebugMarkerSetObjectName(shell().context().dev, (uint64_t)pipeline, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                                      markerName.c_str());
    }
}

const std::vector<Pipeline::Reference> Pipeline::Handler::createPipelines(
    const std::multiset<std::pair<PIPELINE, RenderPass::offset>>& set) {
    //
    std::vector<Pipeline::Reference> refs;
    VkGraphicsPipelineCreateInfo createInfo;
    CreateInfoResources createInfoRes;

    auto it = set.begin();
    while (it != set.end()) {
        createInfo = {};
        createInfo.basePipelineHandle = VK_NULL_HANDLE;
        createInfo.basePipelineIndex = 0;
        createInfoRes = {};

        auto pipelineType = it->first;
        auto& pPipeline = getPipeline(it->first);

        while (it != set.end() && it->first == pipelineType) {
            bool hasBase = false;
            // Get pipeline from map or create a map element
            pipelineMapKey key = {pipelineType, it->second};
            if (pipelineMap_.count(key) == 0) {
                pipelineMap_.insert({key, VK_NULL_HANDLE});
            }

            VkPipeline& pipeline = pipelineMap_.at(key);
            // Setup for derivatives...
            setBase(pipeline, createInfo, hasBase);

            // Save the old pipeline for clean up if necessary
            if (pipeline != VK_NULL_HANDLE) oldPipelines_.push_back({-1, pipeline});

            const auto& pPass = passHandler().getPass(it->second);
            createInfo.renderPass = pPass->pass;
            createInfo.subpass = pPass->getSubpassId(pipelineType);

            pPipeline->setInfo(createInfoRes, createInfo);

            // Give the render pass a chance to override default settings
            pPass->overridePipelineCreateInfo(pipelineType, createInfoRes);

            // Create the pipeline
            createPipeline(pPipeline->NAME + " " + std::to_string(it->second), createInfo, pipeline);

            // REFERENCE
            refs.emplace_back(pPipeline->makeReference());
            refs.back().pipeline = pipeline;

            // Setup for derivatives...
            setBase(pipeline, createInfo, hasBase);

            std::advance(it, 1);
        }
    }

    return refs;
}

VkShaderStageFlags Pipeline::Handler::getDescriptorSetStages(const DESCRIPTOR_SET& setType) {
    VkShaderStageFlags stages = 0;
    for (const auto& pPipeline : pPipelines_) {
        for (const auto& setType : pPipeline->DESCRIPTOR_SET_TYPES) {
            if (setType == setType) {
                for (const auto& shaderType : pPipeline->SHADER_TYPES) {
                    stages |= SHADER_ALL.at(shaderType).stage;
                }
            }
        }
    }
    return stages;
}

void Pipeline::Handler::initShaderInfoMap(Shader::shaderInfoMap& map) {
    for (const auto& pPipeline : pPipelines_) {
        const auto& slotMap = pPipeline->getDescSetMacroSlotMap();
        for (const auto& shaderType : pPipeline->SHADER_TYPES) {
            map.insert({{shaderType, slotMap}, {}});
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

    std::multiset<std::pair<PIPELINE, RenderPass::offset>> updateSet;

    auto itUpdate = needsUpdateSet_.begin();
    auto itMap = pipelineMap_.begin();

    while (itUpdate != needsUpdateSet_.end() && itMap != pipelineMap_.end()) {
        while (*itUpdate != itMap->first.first && itMap != pipelineMap_.end()) ++itMap;
        while (*itUpdate == itMap->first.first && itMap != pipelineMap_.end()) {
            updateSet.insert({itMap->first.first, itMap->first.second});
            ++itMap;
        }
        ++itUpdate;
    }

    if (updateSet.empty()) return;

    auto refs = createPipelines(updateSet);
    passHandler().updatePipelineReferences(updateSet, refs);

    needsUpdateSet_.clear();
}
