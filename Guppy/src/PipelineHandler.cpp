
#include "PipelineHandler.h"

#include "Constants.h"
#include "Material.h"
#include "Pipeline.h"
#include "ShaderHandler.h"
#include "TextureHandler.h"

Pipeline::Handler Pipeline::Handler::inst_;

Pipeline::Handler::Handler()
    :  // GENERAL
      cache_(VK_NULL_HANDLE),
      // DESCRIPTOR
      descPool_(VK_NULL_HANDLE),
      // PIPELINES
      defaultCreateInfoResources_(),
      pDefaultTriListColor_(std::make_unique<DefaultTriListColor>()),
      pDefaultLine_(std::make_unique<DefaultLine>()),
      pDefaultTriListTex_(std::make_unique<DefaultTriListTexture>()) {}

void Pipeline::Handler::reset() {
    // PIPELINE
    if (pDefaultTriListColor_ != nullptr) inst_.pDefaultTriListColor_->destroy(inst_.ctx_.dev);
    if (pDefaultLine_ != nullptr) inst_.pDefaultLine_->destroy(inst_.ctx_.dev);
    if (pDefaultTriListTex_ != nullptr) inst_.pDefaultTriListTex_->destroy(inst_.ctx_.dev);
    // DESCRIPTOR
    for (const auto& keyValue : inst_.descriptorMap_)
        if (keyValue.second.layout != VK_NULL_HANDLE)
            vkDestroyDescriptorSetLayout(inst_.ctx_.dev, keyValue.second.layout, nullptr);
    // HANDLER OWNED
    if (cache_ != VK_NULL_HANDLE) vkDestroyPipelineCache(ctx_.dev, cache_, nullptr);
    if (descPool_ != VK_NULL_HANDLE) vkDestroyDescriptorPool(inst_.ctx_.dev, descPool_, nullptr);
}

void Pipeline::Handler::init(Shell* sh, const Game::Settings& settings) {
    // Clean up owned memory...
    inst_.reset();

    inst_.sh_ = sh;
    inst_.ctx_ = sh->context();
    inst_.settings_ = settings;

    // HANDLER OWNED
    inst_.createDescriptorPool();
    inst_.createPipelineCache(inst_.cache_);
    inst_.createDefaultPushConstantRange(inst_.ctx_);
    inst_.createDescriptorSetLayouts();

    // PIPELINES
    inst_.pDefaultTriListColor_->init(inst_.ctx_, inst_.settings_);
    inst_.pDefaultLine_->init(inst_.ctx_, inst_.settings_);
    inst_.pDefaultTriListTex_->init(inst_.ctx_, inst_.settings_);
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
    vk::assert_success(vkCreateDescriptorPool(inst_.ctx_.dev, &pool_info, nullptr, &descPool_));
}

/* OLD POOL LOGIC THAT HAD A BAD ATTEMPT AT AN ALLOCATION STRATEGY
 */
// void Pipeline::Handler::createDescriptorPool(std::unique_ptr<DescriptorResources> & pRes) {
//    const auto& imageCount = inst_.ctx_.image_count;
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
//    vk::assert_success(vkCreateDescriptorPool(inst_.ctx_.dev, &desc_pool_info, nullptr, &pRes->pool));
//}

void Pipeline::Handler::createDefaultPushConstantRange(const Shell::Context& ctx) {
    VkPushConstantRange range;

    if (sizeof(DefaultPushConstants) > ctx.physical_dev_props[ctx.physical_dev_index].properties.limits.maxPushConstantsSize)
        assert(0 && "Too many push constants");
    range = {};
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    range.offset = 0;
    range.size = sizeof(DefaultPushConstants);
    pushConstantRanges_.push_back(range);
}

void Pipeline::Handler::createDescriptorSetLayouts() {
    // TODO: this is another reason why the pipelines should be
    // in a container.
    descriptorMap_[pDefaultTriListColor_->getDescriptorTypeSet()] = {};
    descriptorMap_[pDefaultLine_->getDescriptorTypeSet()] = {};
    descriptorMap_[pDefaultTriListTex_->getDescriptorTypeSet()] = {};

    for (auto const& keyValue : descriptorMap_) {
        // Gather bindings...
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        for (const auto& descType : keyValue.first) {
            switch (descType) {
                case DESCRIPTOR_TYPE::DEFAULT_UNIFORM:
                case DESCRIPTOR_TYPE::DEFAULT_DYNAMIC_UNIFORM:
                    // TODO: Should these functions be virtual ???
                    bindings.push_back(Shader::Handler::getDescriptorLayoutBinding(descType));
                    break;
                case DESCRIPTOR_TYPE::DEFAULT_SAMPLER:
                    // TODO: Should these functions be virtual ???
                    bindings.push_back(TextureHandler::getDescriptorLayoutBinding(descType));
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
        vk::assert_success(vkCreateDescriptorSetLayout(ctx_.dev, &layoutInfo, nullptr, &resources.layout));

        if (settings_.enable_debug_markers) {
            std::string markerName = " descriptor set layout";  // TODO: a meaningful name
            ext::DebugMarkerSetObjectName(ctx_.dev, (uint64_t)resources.layout,
                                          VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, markerName.c_str());
        }

        descriptorMap_[keyValue.first] = resources;
    }
}

void Pipeline::Handler::createPipelineCache(VkPipelineCache& cache) {
    VkPipelineCacheCreateInfo pipeline_cache_info = {};
    pipeline_cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipeline_cache_info.initialDataSize = 0;
    pipeline_cache_info.pInitialData = nullptr;
    vk::assert_success(vkCreatePipelineCache(inst_.ctx_.dev, &pipeline_cache_info, nullptr, &cache));
}

// TODO: add params that can indicate to free/reallocate
void Pipeline::Handler::makeDescriptorSets(const PIPELINE_TYPE& type, DescriptorSetsReference& descSetsRef,
                                           const std::shared_ptr<Texture::Data>& pTexture) {
    const auto& pPipeline = inst_.getPipelineInternal(type);
    const auto& descTypeKey = pPipeline->getDescriptorTypeSet();
    inst_.validateDescriptorTypeKey(descTypeKey, pTexture);

    auto& item = inst_.descriptorMap_[descTypeKey];
    auto offset = pPipeline->getDescriptorSetOffset(pTexture);

    if (offset >= item.resources.size()) item.resources.resize(offset + 1);
    auto& resource = item.resources[offset];

    // If sets are not already created  then make them...
    if (resource.status != STATUS::READY) {
        inst_.allocateDescriptorSets(descTypeKey, item.layout, resource, pTexture);
    } else if (resource.sets.size() != static_cast<size_t>(1 * inst_.ctx_.imageCount)) {
        assert(false);  // I haven't dealt with this yet...
    }

    // Copy desc sets from the resouces for the meshse. (TODO: rethink this)
    descSetsRef.pipeline = pPipeline->pipeline_;
    descSetsRef.bindPoint = pPipeline->bindPoint_;
    descSetsRef.layout = pPipeline->layout_;
    descSetsRef.firstSet = 0;
    descSetsRef.descriptorSetCount = 1;
    descSetsRef.pDescriptorSets = resource.sets.data();
    // TODO: this is not right...
    descSetsRef.dynamicOffsets = Shader::Handler::getDynamicOffsets(descTypeKey);
}

void Pipeline::Handler::allocateDescriptorSets(const std::set<DESCRIPTOR_TYPE> types, const VkDescriptorSetLayout& layout,
                                               DescriptorMapItem::Resource& resource,
                                               const std::shared_ptr<Texture::Data>& pTexture, uint32_t count) {
    auto setCount = static_cast<uint32_t>(count * ctx_.imageCount);
    assert(setCount > 0);

    std::vector<VkDescriptorSetLayout> layouts(setCount, layout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descPool_;
    allocInfo.descriptorSetCount = setCount;
    allocInfo.pSetLayouts = layouts.data();

    resource.sets.resize(setCount);
    vk::assert_success(vkAllocateDescriptorSets(ctx_.dev, &allocInfo, resource.sets.data()));

    Shader::Handler::updateDescriptorSets(types, resource.sets, setCount, pTexture);

    resource.status = STATUS::READY;
}

void Pipeline::Handler::validateDescriptorTypeKey(const std::set<DESCRIPTOR_TYPE> types,
                                                  const std::shared_ptr<Texture::Data>& pTexture = nullptr) {
    bool isValid = false;
    for (const auto& type : types) {
        switch (type) {
            case DESCRIPTOR_TYPE::DEFAULT_SAMPLER:
                // Ensure that the texture is ready before potentially creating a descriptor set from it.
                assert(pTexture != nullptr && pTexture->status == STATUS::READY);
                break;
        }
        if (isValid) return;
    }
}

void Pipeline::Handler::createPipelines(const std::unique_ptr<RenderPass::Base>& pPass, bool remake) {
    for (const auto& type : pPass->PIPELINE_TYPES) {
        auto& pPipeline = inst_.getPipelineInternal(type);
        if (pPipeline->pipeline_ != VK_NULL_HANDLE && !remake) return;
        pPipeline->SUBPASS_ID = pPass->getSubpassId(type);
        inst_.createPipeline(type, pPass, false);
    }
}

// TODO: this needs a refactor... only does default pipeline stuff!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const VkPipeline& Pipeline::Handler::createPipeline(const PIPELINE_TYPE& type,
                                                    const std::unique_ptr<RenderPass::Base>& pPass, bool setBase) {
    auto& createInfo = inst_.getPipelineCreateInfo(type);
    auto& pPipeline = inst_.getPipelineInternal(type);

    // Store the render pass handle on first creation
    if (pPass != nullptr) createInfo.renderPass = pPass->pass;
    createInfo.subpass = (pPass != nullptr) ? pPass->getSubpassId(type) : pPipeline->SUBPASS_ID;

    // TODO: WHAT HAPPENS IF YOU DELETE THE BASE PIPELINE?????
    if (setBase) {
        createInfo.basePipelineHandle = VK_NULL_HANDLE;
        createInfo.basePipelineIndex = 0;
    }
    // Save old pipeline for clean up if necessary...
    if (pPipeline->pipeline_ != VK_NULL_HANDLE) inst_.oldPipelines_.push_back({-1, pPipeline->pipeline_});

    auto& pipeline =
        pPipeline->create(inst_.ctx_, inst_.settings_, inst_.cache_, inst_.defaultCreateInfoResources_, createInfo);

    // Setup for derivatives...
    if (setBase) {
        createInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
        createInfo.basePipelineHandle = pipeline;
        createInfo.basePipelineIndex = -1;
    }

    return pipeline;
}

// "frameIndex" of -1 means clean up regardless
void Pipeline::Handler::cleanup(int frameIndex) {
    if (inst_.oldPipelines_.empty()) return;

    for (uint8_t i = 0; i < inst_.oldPipelines_.size(); i++) {
        auto& pair = inst_.oldPipelines_[i];
        if (pair.first == frameIndex || frameIndex == -1) {
            vkDestroyPipeline(inst_.ctx_.dev, pair.second, nullptr);
            inst_.oldPipelines_.erase(inst_.oldPipelines_.begin() + i);
        } else if (pair.first == -1) {
            pair.first = static_cast<int>(frameIndex);
        }
    }
}
