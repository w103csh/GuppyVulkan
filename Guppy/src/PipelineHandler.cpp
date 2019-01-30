
#include "PipelineHandler.h"

#include "Constants.h"
#include "Material.h"
#include "PBR.h"
#include "Pipeline.h"
#include "SceneHandler.h"
#include "ShaderHandler.h"
#include "Shell.h"
#include "TextureHandler.h"

Pipeline::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),
      // GENERAL
      cache_(VK_NULL_HANDLE),
      // DESCRIPTOR
      descPool_(VK_NULL_HANDLE),
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
    // DESCRIPTOR
    for (const auto& keyValue : descriptorMap_)
        if (keyValue.second.layout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(shell().context().dev, keyValue.second.layout, nullptr);
    // HANDLER OWNED
    if (cache_ != VK_NULL_HANDLE) vkDestroyPipelineCache(shell().context().dev, cache_, nullptr);
    if (descPool_ != VK_NULL_HANDLE) vkDestroyDescriptorPool(shell().context().dev, descPool_, nullptr);

    maxPushConstantsSize_ = 0;
}

void Pipeline::Handler::init() {
    // Clean up owned memory...
    reset();

    // PUSH CONSTANT
    maxPushConstantsSize_ =
        shell().context().physical_dev_props[shell().context().physical_dev_index].properties.limits.maxPushConstantsSize;

    // HANDLER OWNED
    createDescriptorPool();
    createPipelineCache(cache_);
    createDescriptorSetLayouts();

    // PIPELINES
    for (auto& pPipeline : pPipelines_) pPipeline->init();
}

void Pipeline::Handler::createDescriptorPool() {
    // TODO: an actaul allocator that works, and doesn't waste a ton of memory
    std::vector<VkDescriptorPoolSize> poolSizes = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                                   {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                                   {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                                   {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                                   {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                                   {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                                   {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                                   {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                                   {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                                   {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                                   {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000 * poolSizes.size();
    pool_info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    pool_info.pPoolSizes = poolSizes.data();
    vk::assert_success(vkCreateDescriptorPool(shell().context().dev, &pool_info, nullptr, &descPool_));
}

/* OLD POOL LOGIC THAT HAD A BAD ATTEMPT AT AN ALLOCATION STRATEGY
 */
// void Pipeline::Handler::createDescriptorPool(std::unique_ptr<DescriptorResources> & pRes) {
//    const auto& imageCount = shell().context().image_count;
//    uint32_t maxSets = (pRes->colorCount * imageCount) + (pRes->texCount * imageCount);
//
//    std::vector<VkDescriptorPoolSize> poolSizes;
//    VkDescriptorPoolSize poolSize;
//
//    poolSize = {};
//    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    poolSize.descriptorCount = (pRes->colorCount + pRes->texCount) * imageCount;
//    poolSizes.push_back(poolSize);
//
//    poolSize = {};
//    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    poolSize.descriptorCount = pRes->texCount * imageCount;
//    poolSizes.push_back(poolSize);
//
//    poolSize = {};
//    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
//    poolSize.descriptorCount = pRes->texCount * imageCount;
//    poolSizes.push_back(poolSize);
//
//    VkDescriptorPoolCreateInfo desc_pool_info = {};
//    desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//    desc_pool_info.maxSets = maxSets;
//    desc_pool_info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
//    desc_pool_info.pPoolSizes = poolSizes.data();
//
//    vk::assert_success(vkCreateDescriptorPool(shell().context().dev, &desc_pool_info, nullptr, &pRes->pool));
//}

void Pipeline::Handler::createDescriptorSetLayouts() {
    for (const auto& pPipeline : pPipelines_) {
        descriptorMap_[pPipeline->getDescriptorTypeSet()] = {};
    }

    for (auto const& keyValue : descriptorMap_) {
        // Gather bindings...
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for (const auto& descType : keyValue.first) {
            switch (descType) {
                case DESCRIPTOR::DEFAULT_UNIFORM:
                case DESCRIPTOR::DEFAULT_DYNAMIC_UNIFORM:
                    // TODO: Should these functions be virtual ???
                    // bindings.push_back(Shader::Handler::getDescriptorLayoutBinding(descType));
                    break;
                case DESCRIPTOR::DEFAULT_SAMPLER:
                    // TODO: Should these functions be virtual ???
                    // bindings.push_back(TextureHandler::getDescriptorLayoutBinding(descType));
                    break;
                default:
                    throw std::runtime_error("descriptor type not handled!");
            }
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        DescriptorMapItem resources = {};
        vk::assert_success(vkCreateDescriptorSetLayout(shell().context().dev, &layoutInfo, nullptr, &resources.layout));

        if (settings().enable_debug_markers) {
            std::string markerName = " descriptor set layout";  // TODO: a meaningful name
            ext::DebugMarkerSetObjectName(shell().context().dev, (uint64_t)resources.layout,
                                          VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, markerName.c_str());
        }

        descriptorMap_[keyValue.first] = resources;
    }
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

// TODO: add params that can indicate to free/reallocate
void Pipeline::Handler::makeDescriptorSets(const PIPELINE& type, DescriptorSetsReference& descSetsRef,
                                           const std::shared_ptr<Texture::DATA>& pTexture) {
    const auto& pPipeline = getPipeline(type);
    const auto& descTypeKey = pPipeline->getDescriptorTypeSet();
    validateDescriptorTypeKey(descTypeKey, pTexture);

    auto& item = descriptorMap_[descTypeKey];
    auto offset = pPipeline->getDescriptorSetOffset(pTexture);

    if (offset >= item.resources.size()) item.resources.resize(offset + 1);
    auto& resource = item.resources[offset];

    // If sets are not already created  then make them...
    if (resource.status != STATUS::READY) {
        allocateDescriptorSets(descTypeKey, item.layout, resource, pTexture);
    } else if (resource.sets.size() != static_cast<size_t>(1 * shell().context().imageCount)) {
        assert(false);  // I haven't dealt with this yet...
    }

    // Copy desc sets from the resouces for the meshse. (TODO: rethink this)

    // PIPELINE
    descSetsRef.pipeline = pPipeline->pipeline_;
    descSetsRef.bindPoint = pPipeline->BIND_POINT;
    descSetsRef.layout = pPipeline->layout_;

    // DESCRIPTOR
    descSetsRef.firstSet = 0;
    descSetsRef.descriptorSetCount = 1;
    descSetsRef.pDescriptorSets = resource.sets.data();
    // TODO: this is not right...
    descSetsRef.dynamicOffsets = shaderHandler().getDynamicOffsets(descTypeKey);

    // PUSH CONSTANT
    descSetsRef.pushConstantTypes = pPipeline->PUSH_CONSTANT_TYPES;
    descSetsRef.pushConstantStages = shaderHandler().getStageFlags(pPipeline->SHADER_TYPES);
}

void Pipeline::Handler::allocateDescriptorSets(const std::set<DESCRIPTOR> types, const VkDescriptorSetLayout& layout,
                                               DescriptorMapItem::Resource& resource,
                                               const std::shared_ptr<Texture::DATA>& pTexture, uint32_t count) {
    auto setCount = static_cast<uint32_t>(count * shell().context().imageCount);
    assert(setCount > 0);

    std::vector<VkDescriptorSetLayout> layouts(setCount, layout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool_;
    allocInfo.descriptorSetCount = setCount;
    allocInfo.pSetLayouts = layouts.data();

    resource.sets.resize(setCount);
    vk::assert_success(vkAllocateDescriptorSets(shell().context().dev, &allocInfo, resource.sets.data()));

    shaderHandler().updateDescriptorSets(types, resource.sets, setCount, pTexture);

    resource.status = STATUS::READY;
}

void Pipeline::Handler::validateDescriptorTypeKey(const std::set<DESCRIPTOR> types,
                                                  const std::shared_ptr<Texture::DATA>& pTexture = nullptr) {
    bool isValid = false;
    for (const auto& type : types) {
        switch (type) {
            case DESCRIPTOR::DEFAULT_SAMPLER:
                // Ensure that the texture is ready before potentially creating a descriptor set from it.
                assert(pTexture != nullptr && pTexture->status == STATUS::READY);
                break;
        }
        if (isValid) return;
    }
}

void Pipeline::Handler::createPipelines(const std::unique_ptr<RenderPass::Base>& pPass, bool remake) {
    for (const auto& type : pPass->PIPELINE_TYPES) {
        auto& pPipeline = getPipeline(type);
        if (pPipeline->pipeline_ != VK_NULL_HANDLE && !remake) return;
        pPipeline->SUBPASS_ID = pPass->getSubpassId(type);
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
    createInfo.subpass = (pPass != nullptr) ? pPass->getSubpassId(type) : pPipeline->SUBPASS_ID;

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
