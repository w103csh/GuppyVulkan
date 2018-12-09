
#include "Constants.h"
#include "Material.h"
#include "PipelineHandler.h"

PipelineHandler PipelineHandler::inst_;

PipelineHandler::PipelineHandler()
    : setLayouts_{VK_NULL_HANDLE, VK_NULL_HANDLE},
      cache_(VK_NULL_HANDLE),
      pipelineLayouts_{VK_NULL_HANDLE, VK_NULL_HANDLE},
      defAttachRefs_(),
      createResources_() {
    for (auto& layout : pipelineLayouts_) layout = VK_NULL_HANDLE;
    inst_.cache_ = VK_NULL_HANDLE;
    for (auto& layout : setLayouts_) layout = VK_NULL_HANDLE;
}

void PipelineHandler::reset() {
    // pipelines
    // layouts
    for (auto& layout : pipelineLayouts_)
        if (layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(ctx_.dev, layout, nullptr);
    // cache
    if (cache_ != VK_NULL_HANDLE) vkDestroyPipelineCache(ctx_.dev, cache_, nullptr);

    // desciptor
    for (auto& layout : setLayouts_)
        if (layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(ctx_.dev, layout, nullptr);
}

void PipelineHandler::init(Shell* sh, const Game::Settings& settings) {
    // Clean up owned memory...
    inst_.reset();

    inst_.sh_ = sh;
    inst_.ctx_ = sh->context();
    inst_.settings_ = settings;

    inst_.createPushConstantRange();

    inst_.createDescriptorSetLayout(Vertex::TYPE::COLOR, inst_.setLayouts_[static_cast<int>(Vertex::TYPE::COLOR)]);
    inst_.createDescriptorSetLayout(Vertex::TYPE::TEXTURE, inst_.setLayouts_[static_cast<int>(Vertex::TYPE::TEXTURE)]);

    PipelineHandler::createPipelineCache(inst_.cache_);
    // PipelineHandler::createPipelineLayout(inst_.setLayouts_, inst_.layout_, "DEFAULT");

    inst_.createDefaultAttachments();
}

void PipelineHandler::createPushConstantRange() {
    VkPushConstantRange range;

    // Material
    if (sizeof(PushConstants) > inst_.ctx_.physical_dev_props[inst_.ctx_.physical_dev_index].properties.limits.maxPushConstantsSize)
        assert(0 && "Too many push constants");
    range = {};
    range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    range.offset = 0;
    range.size = sizeof(PushConstants);
    pushConstantRanges_.push_back(range);
}

void PipelineHandler::createDescriptorSetLayout(Vertex::TYPE type, VkDescriptorSetLayout& setLayout) {
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    // UNIFORM BUFFER
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = static_cast<uint32_t>(bindings.size());
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;  // TODO: make a frag specific ubo...
    binding.pImmutableSamplers = nullptr;                                            // Optional
    bindings.push_back(binding);

    // TEXTURE
    if (type == Vertex::TYPE::TEXTURE) {
        // Sampler
        binding = {};
        binding.binding = static_cast<uint32_t>(bindings.size());
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding.pImmutableSamplers = nullptr;  // Optional
        bindings.push_back(binding);

        // Dynamic (texture flags)
        binding = {};
        binding.binding = static_cast<uint32_t>(bindings.size());
        binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        binding.pImmutableSamplers = nullptr;  // Optional
        bindings.push_back(binding);
    }

    // LAYOUT
    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uint32_t bindingCount = static_cast<uint32_t>(bindings.size());
    layout_info.bindingCount = bindingCount;
    layout_info.pBindings = bindingCount > 0 ? bindings.data() : nullptr;

    vk::assert_success(vkCreateDescriptorSetLayout(inst_.ctx_.dev, &layout_info, nullptr, &setLayout));

    ext::DebugMarkerSetObjectName(inst_.ctx_.dev, (uint64_t)setLayout, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT,
                                  (Vertex::getTypeName(type) + " descriptor set layout").c_str());

    createPipelineLayout(type, setLayout, Vertex::getTypeName(type));
}

void PipelineHandler::getDescriptorLayouts(uint32_t image_count, Vertex::TYPE type, std::vector<VkDescriptorSetLayout>& layouts) {
    for (uint32_t i = 0; i < image_count; i++) {
        layouts.push_back(inst_.setLayouts_[static_cast<int>(type)]);
    }
}

void PipelineHandler::createDescriptorPool(std::unique_ptr<DescriptorResources>& pRes) {
    const auto& imageCount = inst_.ctx_.image_count;
    uint32_t maxSets = (pRes->colorCount * imageCount) + (pRes->texCount * imageCount);

    std::vector<VkDescriptorPoolSize> poolSizes;
    VkDescriptorPoolSize poolSize;

    poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = (pRes->colorCount + pRes->texCount) * imageCount;
    poolSizes.push_back(poolSize);

    poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = pRes->texCount * imageCount;
    poolSizes.push_back(poolSize);

    poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSize.descriptorCount = pRes->texCount * imageCount;
    poolSizes.push_back(poolSize);

    VkDescriptorPoolCreateInfo desc_pool_info = {};
    desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc_pool_info.maxSets = maxSets;
    desc_pool_info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    desc_pool_info.pPoolSizes = poolSizes.data();

    vk::assert_success(vkCreateDescriptorPool(inst_.ctx_.dev, &desc_pool_info, nullptr, &pRes->pool));
}

std::unique_ptr<DescriptorResources> PipelineHandler::createDescriptorResources(std::vector<VkDescriptorBufferInfo> uboInfos,
                                                                                std::vector<VkDescriptorBufferInfo> dynUboInfos,
                                                                                size_t colorCount, size_t texCount) {
    auto pRes = std::make_unique<DescriptorResources>(uboInfos, dynUboInfos, colorCount, texCount);

    std::vector<VkDescriptorPoolSize> pool_sizes = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
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
    pool_info.maxSets = 1000 * pool_sizes.size();
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    vk::assert_success(vkCreateDescriptorPool(inst_.ctx_.dev, &pool_info, nullptr, &pRes->pool));

    // As of now these don't change dynamically so just create them after the pool is created.
    // COLOR
    auto setCount = static_cast<uint32_t>(colorCount * inst_.ctx_.image_count);
    if (setCount) {
        std::vector<VkDescriptorSetLayout> layouts;
        PipelineHandler::getDescriptorLayouts(setCount, Vertex::TYPE::COLOR, layouts);

        VkDescriptorSetAllocateInfo alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = pRes->pool;
        alloc_info.descriptorSetCount = setCount;
        alloc_info.pSetLayouts = layouts.data();

        pRes->colorSets.resize(setCount);
        vk::assert_success(vkAllocateDescriptorSets(inst_.ctx_.dev, &alloc_info, pRes->colorSets.data()));

        std::vector<VkWriteDescriptorSet> writes;
        VkWriteDescriptorSet write;
        for (size_t i = 0; i < setCount; i++) {
            write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = pRes->colorSets[i];
            write.dstBinding = 0;  // !!!!
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &pRes->uboInfos[0];  // !!! hardcode
            writes.push_back(write);
        }
        vkUpdateDescriptorSets(inst_.ctx_.dev, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }

    // TEXTURE
    pRes->texSets.resize(texCount);

    return std::move(pRes);
}

void PipelineHandler::createTextureDescriptorSets(const VkDescriptorImageInfo& info, int offset,
                                                  std::unique_ptr<DescriptorResources>& pRes) {
    auto& sets = pRes->texSets[offset];
    // If sets are not empty then they already exist so don't re-allocate them.
    if (sets.empty()) {
        sets.resize(inst_.ctx_.image_count);

        std::vector<VkDescriptorSetLayout> layouts;
        PipelineHandler::getDescriptorLayouts(inst_.ctx_.image_count, Vertex::TYPE::TEXTURE, layouts);

        VkDescriptorSetAllocateInfo alloc_info;
        alloc_info = {};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = pRes->pool;
        alloc_info.descriptorSetCount = inst_.ctx_.image_count;
        alloc_info.pSetLayouts = layouts.data();

        vk::assert_success(vkAllocateDescriptorSets(inst_.ctx_.dev, &alloc_info, sets.data()));

        std::vector<VkWriteDescriptorSet> writes;
        VkWriteDescriptorSet write;
        for (size_t i = 0; i < sets.size(); i++) {
            // Camera
            write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = sets[i];
            write.dstBinding = 0;  // !!!!
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            write.descriptorCount = 1;
            write.pBufferInfo = &pRes->uboInfos[0];  // !!! hardcode
            writes.push_back(write);
            // Sampler
            write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = sets[i];
            write.dstBinding = 1;  // !!!!
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            write.descriptorCount = 1;
            write.pImageInfo = &info;
            writes.push_back(write);
            // Dynamic (texture flags)
            write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = sets[i];
            write.dstBinding = 2;  // !!!!
            write.dstArrayElement = 0;
            write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            write.descriptorCount = 1;
            write.pBufferInfo = &pRes->dynUboInfos[0];  // !!! hardcode
            writes.push_back(write);
        }
        vkUpdateDescriptorSets(inst_.ctx_.dev, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void PipelineHandler::createPipelineCache(VkPipelineCache& cache) {
    VkPipelineCacheCreateInfo pipeline_cache_info = {};
    pipeline_cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipeline_cache_info.initialDataSize = 0;
    pipeline_cache_info.pInitialData = nullptr;
    vk::assert_success(vkCreatePipelineCache(inst_.ctx_.dev, &pipeline_cache_info, nullptr, &cache));
}

void PipelineHandler::createPipelineLayout(Vertex::TYPE type, const VkDescriptorSetLayout& setLayout, std::string markerName) {
    VkPipelineLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // push constants
    layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstantRanges_.size());
    layoutInfo.pPushConstantRanges = pushConstantRanges_.data();
    // descriptor layouts
    uint32_t setLayoutCount = 1;  // static_cast<uint32_t>(setLayouts.size());
    layoutInfo.setLayoutCount = setLayoutCount;
    layoutInfo.pSetLayouts = &setLayout;

    auto& layout = pipelineLayouts_[static_cast<int>(type)];
    vk::assert_success(vkCreatePipelineLayout(inst_.ctx_.dev, &layoutInfo, nullptr, &layout));

    if (!markerName.empty()) {
        ext::DebugMarkerSetObjectName(inst_.ctx_.dev, (uint64_t)layout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT,
                                      (markerName + " pipeline layout").c_str());
    }
}

void PipelineHandler::createInputStateCreateInfo(Vertex::TYPE type, PipelineCreateInfoResources& resources) {
    resources.vertexInputStateInfo = {};
    resources.vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    switch (type) {
        case Vertex::TYPE::COLOR: {
            resources.bindingDesc = Vertex::getColorBindDesc();
            resources.attrDesc = Vertex::getColorAttrDesc();
        } break;
        case Vertex::TYPE::TEXTURE: {
            resources.bindingDesc = Vertex::getTexBindDesc();
            resources.attrDesc = Vertex::getTexAttrDesc();
        } break;
        default: { throw std::runtime_error("shader type not handled!"); } break;
    }

    // bindings
    resources.vertexInputStateInfo.vertexBindingDescriptionCount = 1;
    resources.vertexInputStateInfo.pVertexBindingDescriptions = &resources.bindingDesc;
    // attributes
    resources.vertexInputStateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(resources.attrDesc.size());
    resources.vertexInputStateInfo.pVertexAttributeDescriptions = resources.attrDesc.data();
}

// These are settings based ... TODO: I think there might be a more space efficient order.
void PipelineHandler::createDefaultAttachments(bool clear, VkImageLayout finalLayout) {
    VkAttachmentDescription attachemnt = {};

    // COLOR ATTACHMENT (SWAP-CHAIN)
    defAttachRefs_.color = {};
    attachemnt = {};
    attachemnt.format = inst_.ctx_.surface_format.format;
    attachemnt.samples = VK_SAMPLE_COUNT_1_BIT;
    attachemnt.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachemnt.finalLayout = finalLayout;
    attachemnt.flags = 0;
    attachments_.push_back(attachemnt);
    // REFERENCE
    defAttachRefs_.color.attachment = static_cast<uint32_t>(attachments_.size() - 1);
    defAttachRefs_.color.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // COLOR ATTACHMENT RESOLVE (MULTI-SAMPLE)
    defAttachRefs_.colorResolve = {};
    if (inst_.settings_.include_color) {
        attachemnt = {};
        attachemnt.format = inst_.ctx_.surface_format.format;
        attachemnt.samples = inst_.ctx_.num_samples;
        attachemnt.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachemnt.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachemnt.flags = 0;
        // REFERENCE
        defAttachRefs_.colorResolve.attachment = static_cast<uint32_t>(attachments_.size() - 1);  // point to swapchain attachment
        defAttachRefs_.colorResolve.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachments_.push_back(attachemnt);
        defAttachRefs_.color.attachment = static_cast<uint32_t>(attachments_.size() - 1);  // point to multi-sample attachment
    }

    // DEPTH ATTACHMENT
    defAttachRefs_.depth = {};
    if (inst_.settings_.include_depth) {
        attachemnt = {};
        attachemnt.format = inst_.ctx_.depth_format;
        attachemnt.samples = inst_.ctx_.num_samples;
        attachemnt.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        // This was "don't care" in the sample and that makes more sense to
        // me. This obvious is for some kind of stencil operation.
        attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachemnt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachemnt.flags = 0;
        attachments_.push_back(attachemnt);
        // REFERENCE
        defAttachRefs_.depth.attachment = static_cast<uint32_t>(attachments_.size() - 1);
        defAttachRefs_.depth.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
}

// VkSubpassDescription PipelineHandler::create_default_subpass() {
//    VkSubpassDescription subpass = {};
//    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//    subpass.inputAttachmentCount = 0;
//    subpass.pInputAttachments = nullptr;
//    subpass.colorAttachmentCount = 1;
//    subpass.pColorAttachments = &inst_.defAttachRefs_.color;
//    subpass.pResolveAttachments = inst_.settings_.include_color ? &inst_.defAttachRefs_.colorResolve : nullptr;
//    subpass.pDepthStencilAttachment = inst_.settings_.include_depth ? &inst_.defAttachRefs_.depth : nullptr;
//    subpass.preserveAttachmentCount = 0;
//    subpass.pPreserveAttachments = nullptr;
//
//    return subpass;
//}

void PipelineHandler::createPipelineResources(PipelineResources& resources) {
    // SUBPASSES
    inst_.createSubpasses(resources);
    // DEPENDENCIES
    inst_.createDependencies(resources);
    // RENDER PASS
    inst_.createRenderPass(resources);

    // BASE PIPELINE (TRI_LIST_COLOR)
    inst_.createBasePipeline(Vertex::TYPE::COLOR, PIPELINE_TYPE::TRI_LIST_COLOR, ShaderHandler::getShader(SHADER_TYPE::COLOR_VERT),
                             ShaderHandler::getShader(SHADER_TYPE::COLOR_FRAG), inst_.createResources_, resources);

    // SETUP FOR DERIVATIVES
    resources.pipelineInfo.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    resources.pipelineInfo.basePipelineHandle = resources.pipelines[static_cast<int>(PIPELINE_TYPE::TRI_LIST_COLOR)];
    resources.pipelineInfo.basePipelineIndex = -1;  // TODO: why is this -1?

    // DERIVATIVES
    inst_.createPipeline(PIPELINE_TYPE::LINE, resources);
    inst_.createPipeline(PIPELINE_TYPE::TRI_LIST_TEX, resources);
}

void PipelineHandler::createPipeline(PIPELINE_TYPE type, PipelineResources& resources) {
    // Destory old pipeline if necessary
    VkPipeline& oldPipeline = resources.pipelines[static_cast<int>(type)];
    if (oldPipeline != VK_NULL_HANDLE) {
        inst_.oldPipelines_.push_back(oldPipeline);
    }

    // Create the pipeline (Requires the base pipeline to have been created!!!)
    VkPipelineInputAssemblyStateCreateInfo input_info = {};
    input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_info.primitiveRestartEnable = VK_FALSE;

    switch (type) {
        case PIPELINE_TYPE::LINE: {
            resources.pipelineInfo.subpass = static_cast<uint32_t>(type);
            // layout
            resources.pipelineInfo.layout = inst_.pipelineLayouts_[static_cast<int>(Vertex::TYPE::COLOR)];
            // vertex input
            inst_.createInputStateCreateInfo(Vertex::TYPE::COLOR, inst_.createResources_);
            resources.pipelineInfo.pVertexInputState = &inst_.createResources_.vertexInputStateInfo;
            // shader stages
            inst_.createResources_.stagesInfo[0] = ShaderHandler::getShader(SHADER_TYPE::COLOR_VERT)->info;
            inst_.createResources_.stagesInfo[1] = ShaderHandler::getShader(SHADER_TYPE::LINE_FRAG)->info;
            resources.pipelineInfo.stageCount = static_cast<uint32_t>(inst_.createResources_.stagesInfo.size());
            resources.pipelineInfo.pStages = inst_.createResources_.stagesInfo.data();
            // topology
            input_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            resources.pipelineInfo.pInputAssemblyState = &input_info;

            vk::assert_success(vkCreateGraphicsPipelines(inst_.ctx_.dev, inst_.cache_, 1, &resources.pipelineInfo, nullptr,
                                                         &resources.pipelines[static_cast<int>(PIPELINE_TYPE::LINE)]));
        } break;

        case PIPELINE_TYPE::TRI_LIST_COLOR: {
            resources.pipelineInfo.subpass = static_cast<uint32_t>(type);
            // layout
            resources.pipelineInfo.layout = inst_.pipelineLayouts_[static_cast<int>(Vertex::TYPE::COLOR)];
            // vertex input
            inst_.createInputStateCreateInfo(Vertex::TYPE::COLOR, inst_.createResources_);
            resources.pipelineInfo.pVertexInputState = &inst_.createResources_.vertexInputStateInfo;
            // shader stages
            inst_.createResources_.stagesInfo[0] = ShaderHandler::getShader(SHADER_TYPE::COLOR_VERT)->info;
            inst_.createResources_.stagesInfo[1] = ShaderHandler::getShader(SHADER_TYPE::COLOR_FRAG)->info;
            resources.pipelineInfo.stageCount = static_cast<uint32_t>(inst_.createResources_.stagesInfo.size());
            resources.pipelineInfo.pStages = inst_.createResources_.stagesInfo.data();
            // topology
            input_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            resources.pipelineInfo.pInputAssemblyState = &input_info;

            vk::assert_success(vkCreateGraphicsPipelines(inst_.ctx_.dev, inst_.cache_, 1, &resources.pipelineInfo, nullptr,
                                                         &resources.pipelines[static_cast<int>(PIPELINE_TYPE::TRI_LIST_COLOR)]));
        } break;

        case PIPELINE_TYPE::TRI_LIST_TEX: {
            resources.pipelineInfo.subpass = static_cast<uint32_t>(type);
            // layout
            resources.pipelineInfo.layout = inst_.pipelineLayouts_[static_cast<int>(Vertex::TYPE::TEXTURE)];
            // vertex input
            inst_.createInputStateCreateInfo(Vertex::TYPE::TEXTURE, inst_.createResources_);
            resources.pipelineInfo.pVertexInputState = &inst_.createResources_.vertexInputStateInfo;
            // shader stages
            inst_.createResources_.stagesInfo[0] = ShaderHandler::getShader(SHADER_TYPE::TEX_VERT)->info;
            inst_.createResources_.stagesInfo[1] = ShaderHandler::getShader(SHADER_TYPE::TEX_FRAG)->info;
            resources.pipelineInfo.stageCount = static_cast<uint32_t>(inst_.createResources_.stagesInfo.size());
            resources.pipelineInfo.pStages = inst_.createResources_.stagesInfo.data();
            // topology
            input_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            resources.pipelineInfo.pInputAssemblyState = &input_info;

            vk::assert_success(vkCreateGraphicsPipelines(inst_.ctx_.dev, inst_.cache_, 1, &resources.pipelineInfo, nullptr,
                                                         &resources.pipelines[static_cast<int>(PIPELINE_TYPE::TRI_LIST_TEX)]));
        } break;

        default: { throw std::runtime_error("Unhandled pipeline type"); } break;
    }
}

void PipelineHandler::createSubpasses(PipelineResources& resources) {
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &inst_.defAttachRefs_.color;
    subpass.pResolveAttachments = inst_.settings_.include_color ? &inst_.defAttachRefs_.colorResolve : nullptr;
    subpass.pDepthStencilAttachment = inst_.settings_.include_depth ? &inst_.defAttachRefs_.depth : nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;
    resources.subpasses.push_back(subpass);  // 0 - TRI_LIST_COLOR
    resources.subpasses.push_back(subpass);  // 1 - LINE
    resources.subpasses.push_back(subpass);  // 2 - TRI_LIST_TEX
    resources.subpasses.push_back(subpass);  // 3 - UI
}

void PipelineHandler::createDependencies(PipelineResources& resources) {
    // TODO: used for waiting in draw (figure this out... what is the VK_SUBPASS_EXTERNAL one?)
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = static_cast<uint32_t>(PIPELINE_TYPE::TRI_LIST_COLOR);
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    resources.dependencies.push_back(dependency);

    // Line subpass
    // TODO: fix this...
    dependency = {};
    dependency.srcSubpass = static_cast<uint32_t>(PIPELINE_TYPE::TRI_LIST_COLOR);
    dependency.dstSubpass = static_cast<uint32_t>(PIPELINE_TYPE::LINE);
    dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependency.dstAccessMask = 0;
    resources.dependencies.push_back(dependency);

    // Tex subpass
    // TODO: fix this...
    dependency = {};
    dependency.srcSubpass = static_cast<uint32_t>(PIPELINE_TYPE::LINE);
    dependency.dstSubpass = static_cast<uint32_t>(PIPELINE_TYPE::TRI_LIST_TEX);
    dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependency.dstAccessMask = 0;
    resources.dependencies.push_back(dependency);

    // UI subpass
    // TODO: fix this...
    dependency = {};
    dependency.srcSubpass = static_cast<uint32_t>(PIPELINE_TYPE::TRI_LIST_TEX);
    dependency.dstSubpass = static_cast<uint32_t>(PIPELINE_TYPE::UI);
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    resources.dependencies.push_back(dependency);
}

void PipelineHandler::createRenderPass(PipelineResources& resources) {
    VkRenderPassCreateInfo rp_info = {};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.pNext = nullptr;
    // Attachmments
    uint32_t attachmentCount = static_cast<uint32_t>(inst_.attachments_.size());
    rp_info.attachmentCount = attachmentCount;
    rp_info.pAttachments = attachmentCount > 0 ? inst_.attachments_.data() : nullptr;
    // Subpasses
    uint32_t subpassCount = static_cast<uint32_t>(resources.subpasses.size());
    rp_info.subpassCount = subpassCount;
    rp_info.pSubpasses = subpassCount > 0 ? resources.subpasses.data() : nullptr;
    // Dependencies
    uint32_t dependencyCount = static_cast<uint32_t>(resources.dependencies.size());
    rp_info.dependencyCount = dependencyCount;
    rp_info.pDependencies = dependencyCount > 0 ? resources.dependencies.data() : nullptr;

    vk::assert_success(vkCreateRenderPass(inst_.ctx_.dev, &rp_info, nullptr, &resources.renderPass));

    if (!resources.markerName.empty()) {
        ext::DebugMarkerSetObjectName(inst_.ctx_.dev, (uint64_t)resources.renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                                      (resources.markerName + " render pass").c_str());
    }
}

void PipelineHandler::createBasePipeline(Vertex::TYPE vertexType, PIPELINE_TYPE pipelineType, const std::unique_ptr<Shader>& vs,
                                         const std::unique_ptr<Shader>& fs, PipelineCreateInfoResources& createRes,
                                         PipelineResources& pipelineRes) {
    // DYNAMIC STATE
    // TODO: this is weird
    createRes.dynamicStateInfo = {};
    memset(createRes.dynamicStates, 0, sizeof(createRes.dynamicStates));
    createRes.dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    createRes.dynamicStateInfo.pNext = nullptr;
    createRes.dynamicStateInfo.pDynamicStates = createRes.dynamicStates;
    createRes.dynamicStateInfo.dynamicStateCount = 0;

    // INPUT ASSEMBLY
    createInputStateCreateInfo(vertexType, createRes);

    createRes.inputAssemblyStateInfo = {};
    createRes.inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    createRes.inputAssemblyStateInfo.pNext = nullptr;
    createRes.inputAssemblyStateInfo.flags = 0;
    createRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // RASTERIZER

    createRes.rasterizationStateInfo = {};
    createRes.rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    createRes.rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    createRes.rasterizationStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    createRes.rasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    /*  If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far
        planes are clamped to them as opposed to discarding them. This is useful in some special
        cases like shadow maps. Using this requires enabling a GPU feature.
    */
    createRes.rasterizationStateInfo.depthClampEnable = VK_FALSE;
    createRes.rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
    createRes.rasterizationStateInfo.depthBiasEnable = VK_FALSE;
    createRes.rasterizationStateInfo.depthBiasConstantFactor = 0;
    createRes.rasterizationStateInfo.depthBiasClamp = 0;
    createRes.rasterizationStateInfo.depthBiasSlopeFactor = 0;
    /*  The lineWidth member is straightforward, it describes the thickness of lines in terms of
        number of fragments. The maximum line width that is supported depends on the hardware and
        any line thicker than 1.0f requires you to enable the wideLines GPU feature.
    */
    createRes.rasterizationStateInfo.lineWidth = 1.0f;

    createRes.multisampleStateInfo = {};
    createRes.multisampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    createRes.multisampleStateInfo.rasterizationSamples = ctx_.num_samples;
    createRes.multisampleStateInfo.sampleShadingEnable =
        settings_.enable_sample_shading;  // enable sample shading in the pipeline (sampling for fragment interiors)
    createRes.multisampleStateInfo.minSampleShading =
        settings_.enable_sample_shading ? MIN_SAMPLE_SHADING : 0.0f;  // min fraction for sample shading; closer to one is smooth
    createRes.multisampleStateInfo.pSampleMask = nullptr;             // Optional
    createRes.multisampleStateInfo.alphaToCoverageEnable = VK_FALSE;  // Optional
    createRes.multisampleStateInfo.alphaToOneEnable = VK_FALSE;       // Optional

    // BLENDING

    createRes.blendAttach = {};
    createRes.blendAttach.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // blend_attachment.blendEnable = VK_FALSE;
    // blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional
    // blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
    // blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    // blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    // blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    // blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    // common setup
    createRes.blendAttach.blendEnable = VK_TRUE;
    createRes.blendAttach.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    createRes.blendAttach.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    createRes.blendAttach.colorBlendOp = VK_BLEND_OP_ADD;
    createRes.blendAttach.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    createRes.blendAttach.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    createRes.blendAttach.alphaBlendOp = VK_BLEND_OP_ADD;

    createRes.colorBlendStateInfo = {};
    createRes.colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    createRes.colorBlendStateInfo.attachmentCount = 1;
    createRes.colorBlendStateInfo.pAttachments = &createRes.blendAttach;
    createRes.colorBlendStateInfo.logicOpEnable = VK_FALSE;
    createRes.colorBlendStateInfo.logicOp = VK_LOGIC_OP_COPY;  // What does this do?
    createRes.colorBlendStateInfo.blendConstants[0] = 0.0f;
    createRes.colorBlendStateInfo.blendConstants[1] = 0.0f;
    createRes.colorBlendStateInfo.blendConstants[2] = 0.0f;
    createRes.colorBlendStateInfo.blendConstants[3] = 0.0f;
    // blend_info.blendConstants[0] = 0.2f;
    // blend_info.blendConstants[1] = 0.2f;
    // blend_info.blendConstants[2] = 0.2f;
    // blend_info.blendConstants[3] = 0.2f;

    // VIEWPORT & SCISSOR

    createRes.viewportStateInfo = {};
    createRes.viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
#ifndef __ANDROID__
    createRes.viewportStateInfo.viewportCount = NUM_VIEWPORTS;
    createRes.dynamicStates[createRes.dynamicStateInfo.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    createRes.viewportStateInfo.scissorCount = NUM_SCISSORS;
    createRes.dynamicStates[createRes.dynamicStateInfo.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
    createRes.viewportStateInfo.pScissors = nullptr;
    createRes.viewportStateInfo.pViewports = nullptr;
#else
    // TODO: this is outdated now...
    // Temporary disabling dynamic viewport on Android because some of drivers doesn't
    // support the feature.
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

    // DEPTH

    createRes.depthStencilStateInfo = {};
    createRes.depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    createRes.depthStencilStateInfo.pNext = nullptr;
    createRes.depthStencilStateInfo.flags = 0;
    createRes.depthStencilStateInfo.depthTestEnable = settings_.include_depth;
    createRes.depthStencilStateInfo.depthWriteEnable = settings_.include_depth;
    createRes.depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    createRes.depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
    createRes.depthStencilStateInfo.minDepthBounds = 0;
    createRes.depthStencilStateInfo.maxDepthBounds = 1.0f;
    createRes.depthStencilStateInfo.stencilTestEnable = VK_FALSE;
    createRes.depthStencilStateInfo.front = {};
    createRes.depthStencilStateInfo.back = {};
    // dss.back.failOp = VK_STENCIL_OP_KEEP; // ARE THESE IMPORTANT !!!
    // dss.back.passOp = VK_STENCIL_OP_KEEP;
    // dss.back.compareOp = VK_COMPARE_OP_ALWAYS;
    // dss.back.compareMask = 0;
    // dss.back.reference = 0;
    // dss.back.depthFailOp = VK_STENCIL_OP_KEEP;
    // dss.back.writeMask = 0;
    // dss.front = ds.back;

    // TESSELATION
    createRes.tessellationStateInfo = {};  // TODO: this is a potential problem

    // SHADER
    createRes.stagesInfo[0] = vs->info;
    createRes.stagesInfo[1] = fs->info;

    // PIPELINE

    pipelineRes.pipelineInfo = {};
    pipelineRes.pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineRes.pipelineInfo.flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    // shader stages
    uint32_t stageCount = static_cast<uint32_t>(createRes.stagesInfo.size());
    pipelineRes.pipelineInfo.stageCount = stageCount;
    pipelineRes.pipelineInfo.pStages = stageCount > 0 ? createRes.stagesInfo.data() : nullptr;
    // fixed function stages
    pipelineRes.pipelineInfo.pColorBlendState = &createRes.colorBlendStateInfo;
    pipelineRes.pipelineInfo.pDepthStencilState = &createRes.depthStencilStateInfo;
    pipelineRes.pipelineInfo.pDynamicState = &createRes.dynamicStateInfo;
    pipelineRes.pipelineInfo.pInputAssemblyState = &createRes.inputAssemblyStateInfo;
    pipelineRes.pipelineInfo.pMultisampleState = &createRes.multisampleStateInfo;
    pipelineRes.pipelineInfo.pRasterizationState = &createRes.rasterizationStateInfo;
    pipelineRes.pipelineInfo.pTessellationState = nullptr;
    pipelineRes.pipelineInfo.pVertexInputState = &createRes.vertexInputStateInfo;
    pipelineRes.pipelineInfo.pViewportState = &createRes.viewportStateInfo;
    // layout
    pipelineRes.pipelineInfo.layout = pipelineLayouts_[static_cast<int>(vertexType)];
    pipelineRes.pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineRes.pipelineInfo.basePipelineIndex = 0;
    // render pass
    pipelineRes.pipelineInfo.renderPass = pipelineRes.renderPass;
    pipelineRes.pipelineInfo.subpass = static_cast<uint32_t>(pipelineType);

    vk::assert_success(vkCreateGraphicsPipelines(ctx_.dev, cache_, 1, &pipelineRes.pipelineInfo, nullptr,
                                                 &pipelineRes.pipelines[static_cast<int>(pipelineType)]));

    if (!pipelineRes.markerName.empty()) {
        std::string s = pipelineRes.markerName +
                        (vertexType == Vertex::TYPE::COLOR ? " COLOR " : vertexType == Vertex::TYPE::TEXTURE ? " TEXTURE " : " ") +
                        "pipline";
        ext::DebugMarkerSetObjectName(ctx_.dev, (uint64_t)pipelineRes.pipelines[static_cast<int>(pipelineType)],
                                      VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, s.c_str());
    }
}

void PipelineHandler::destroyPipelineResources(PipelineResources& resources) {
    vkDestroyRenderPass(inst_.ctx_.dev, resources.renderPass, nullptr);
    for (auto& pipeline : resources.pipelines) vkDestroyPipeline(inst_.ctx_.dev, pipeline, nullptr);
}

void PipelineHandler::cleanupOldResources() {
    for (auto& pipeline : inst_.oldPipelines_) vkDestroyPipeline(inst_.ctx_.dev, pipeline, nullptr);
    inst_.oldPipelines_.clear();
}
