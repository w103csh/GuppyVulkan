
#include "PipelineHandler.h"

#include "Constants.h"
#include "DescriptorHandler.h"
#include "Material.h"
#include "Mesh.h"
#include "PBR.h"
#include "Pipeline.h"
#include "SceneHandler.h"
#include "ShaderHandler.h"
#include "Shell.h"
#include "TextureHandler.h"

Pipeline::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),
      cache_(VK_NULL_HANDLE),
      // DEFAULT (TODO: remove default things...)
      defaultPipelineInfo_{}  //
{
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
            case PIPELINE::PBR_COLOR:
                pPipelines_.emplace_back(std::make_unique<PBR::Color>(std::ref(*this)));
                break;
            default:
                assert(false);  // add new pipelines here
        }
    }
    // Validate the list of instantiated pipelines.
    assert(pPipelines_.size() == PIPELINE_ALL.size());
    for (const auto& pPipeline : pPipelines_) assert(pPipeline != nullptr);
}

void Pipeline::Handler::reset() {
    // PIPELINE
    for (auto& pPipeline : pPipelines_) pPipeline->destroy();
    // CACHE
    if (cache_ != VK_NULL_HANDLE) vkDestroyPipelineCache(shell().context().dev, cache_, nullptr);

    maxPushConstantsSize_ = 0;
}

void Pipeline::Handler::init() {
    reset();

    // PUSH CONSTANT
    maxPushConstantsSize_ =
        shell().context().physical_dev_props[shell().context().physical_dev_index].properties.limits.maxPushConstantsSize;

    // CACHE
    createPipelineCache(cache_);

    // PIPELINES
    for (auto& pPipeline : pPipelines_) pPipeline->init();
}

void Pipeline::Handler::getReference(Mesh::Base& mesh) {
    const auto& pPipeline = getPipeline(mesh.PIPELINE_TYPE);
    auto reference = &mesh.pipelineReference_;
    // PIPELINE
    reference->pipeline = pPipeline->pipeline_;
    reference->bindPoint = pPipeline->BIND_POINT;
    reference->layout = pPipeline->layout_;
    // PUSH CONSTANT
    reference->pushConstantTypes = pPipeline->PUSH_CONSTANT_TYPES;
    reference->pushConstantStages = shaderHandler().getStageFlags(pPipeline->SHADER_TYPES);
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
            case PUSH_CONSTANT::PBR:
                range.size = sizeof(Pipeline::PBR::PushConstant);
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

void Pipeline::Handler::createPipelines(const std::unique_ptr<RenderPass::Base>& pPass, bool remake) {
    for (const auto& type : pPass->PIPELINE_TYPES) {
        auto& pPipeline = getPipeline(type);
        pPipeline->subpassId_ = pPass->getSubpassId(type);
        if (pPipeline->pipeline_ != VK_NULL_HANDLE && !remake) return;
        createPipeline(type, pPass, false);
    }
}

// TODO: this needs a refactor... only does default pipeline stuff!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const VkPipeline& Pipeline::Handler::createPipeline(const PIPELINE& type, const std::unique_ptr<RenderPass::Base>& pPass,
                                                    bool setBase) {
    auto& createInfo = getPipelineCreateInfo(type);
    auto& pPipeline = getPipeline(type);

    // Store the render pass handle on first creation
    if (pPass != nullptr) createInfo.renderPass = pPass->pass;
    createInfo.subpass = pPipeline->getSubpassId();

    // TODO: WHAT HAPPENS IF YOU DELETE THE BASE PIPELINE?????
    if (setBase) {
        createInfo.basePipelineHandle = VK_NULL_HANDLE;
        createInfo.basePipelineIndex = 0;
    }
    // Save old pipeline for clean up if necessary...
    if (pPipeline->pipeline_ != VK_NULL_HANDLE) oldPipelines_.push_back({-1, pPipeline->pipeline_});

    auto& pipeline = pPipeline->create(cache_, defaultCreateInfoResources_, createInfo);

    // Setup for derivatives...
    if (setBase) {
        createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
        createInfo.basePipelineHandle = pipeline;
        createInfo.basePipelineIndex = -1;
    }

    return pipeline;
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
    for (const auto& type : needsUpdateSet_) {
        createPipeline(type);
        sceneHandler().updatePipelineReferences(type, getPipeline(type)->pipeline_);
    }
    needsUpdateSet_.clear();
}
