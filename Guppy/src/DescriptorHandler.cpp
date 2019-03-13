
#include <algorithm>

#include "DescriptorHandler.h"

#include "Mesh.h"
#include "PBR.h"
#include "Pipeline.h"
#include "PipelineHandler.h"
#include "ShaderHandler.h"
#include "UniformHandler.h"

Descriptor::Handler::Handler(Game* pGame) : Game::Handler(pGame), pool_(VK_NULL_HANDLE) {
    pDescriptorSets_.push_back(std::make_unique<Descriptor::Set::Default::Uniform>());
    pDescriptorSets_.push_back(std::make_unique<Descriptor::Set::Default::Sampler>());
    pDescriptorSets_.push_back(std::make_unique<Descriptor::Set::PBR::Uniform>());
}

void Descriptor::Handler::init() {
    reset();
    createPool();
    createLayouts();
}

void Descriptor::Handler::createPool() {
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
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000 * poolSizes.size();
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    vk::assert_success(vkCreateDescriptorPool(shell().context().dev, &poolInfo, nullptr, &pool_));
}

/* OLD POOL LOGIC THAT HAD A BAD ATTEMPT AT AN ALLOCATION STRATEGY
 */
// void Descriptor::Handler::createDescriptorPool(std::unique_ptr<DescriptorResources> & pRes) {
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

void Descriptor::Handler::createLayouts() {
    for (auto& pSet : pDescriptorSets_) {
        // Gather bindings...
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        for (auto& bindingKeyValue : pSet->BINDING_MAP) {
            auto binding = getDecriptorSetLayoutBinding(bindingKeyValue, pSet->stages);
            bindings.push_back(binding);
        }

        // Create layout...
        VkDescriptorSetLayoutCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        createInfo.pBindings = bindings.data();

        vk::assert_success(vkCreateDescriptorSetLayout(shell().context().dev, &createInfo, nullptr, &pSet->layout));

        if (settings().enable_debug_markers) {
            std::string markerName = " descriptor set layout";  // TODO: a meaningful name
            ext::DebugMarkerSetObjectName(shell().context().dev, (uint64_t)pSet->layout,
                                          VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, markerName.c_str());
        }
    }
}

VkDescriptorSetLayoutBinding Descriptor::Handler::getDecriptorSetLayoutBinding(
    const Descriptor::bindingMapKeyValue& keyValue, const VkShaderStageFlags& stageFlags) const {
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = std::get<1>(keyValue.first);
    layoutBinding.descriptorType = DESCRIPTOR_TYPE_MAP.at(keyValue.second.first);
    layoutBinding.descriptorCount = getDescriptorCount(keyValue.second);
    layoutBinding.stageFlags = stageFlags;
    layoutBinding.pImmutableSamplers = nullptr;  // TODO: use this
    return layoutBinding;
}

uint32_t Descriptor::Handler::getDescriptorCount(const Descriptor::bindingMapValue& value) const {
    const auto& min = *value.second.begin();
    if (value.second.size() == 1) {
        if (min == Descriptor::Set::OFFSET_SINGLE) return 1;
        if (min == Descriptor::Set::OFFSET_ALL) {
            if (DESCRIPTOR_UNIFORM_ALL.count(value.first))
                return static_cast<uint32_t>(uniformHandler().getDescriptorCount(value));
            else
                assert(false && "Unaccounted for scenario");
        }
    }
    return static_cast<uint32_t>(value.second.size());
}

std::vector<VkDescriptorSetLayout> Descriptor::Handler::getDescriptorSetLayouts(const std::list<DESCRIPTOR_SET>& setTypes) {
    std::vector<VkDescriptorSetLayout> layouts;
    for (const auto& setType : setTypes) {
        auto& pSet = getSet(setType);
        if (pSet->TYPE == setType) layouts.push_back(pSet->layout);
    }
    assert(!layouts.empty() && "Didn't find a descriptor layout");
    return layouts;
}

// TODO: add params that can indicate to free/reallocate
void Descriptor::Handler::getReference(Mesh::Base& mesh) {
    auto& reference = mesh.descriptorReference_;

    // REFERENCE
    reference.firstSet = 0;
    reference.descriptorSets.clear();
    reference.descriptorSets.resize(shell().context().imageCount);
    reference.dynamicOffsets.clear();

    for (const auto& setType : pipelineHandler().getPipeline(mesh.PIPELINE_TYPE)->DESCRIPTOR_SET_TYPES) {
        auto& pSet = getSet(setType);

        uint32_t offset = 0;
        switch (pSet->TYPE) {
            case DESCRIPTOR_SET::SAMPLER_DEFAULT: {
                offset = mesh.getMaterial()->getTexture()->offset;
            } break;
        }

        auto& resource = pSet->getResource(offset);

        // If sets are not already created then make them...
        if (resource.status != STATUS::READY) {
            allocateDescriptorSets(pSet, resource);
            updateDescriptorSets(pSet, resource, mesh);
            resource.status = STATUS::READY;
        } else if (resource.descriptorSets.size() != static_cast<size_t>(1 * shell().context().imageCount)) {
            assert(false);  // I haven't dealt with this yet...
        }

        // REFERENCE
        assert(reference.descriptorSets.size() == resource.descriptorSets.size());
        for (auto i = 0; i < reference.descriptorSets.size(); i++) {
            reference.descriptorSets[i].push_back(resource.descriptorSets[i]);
        }
        getDynamicOffsets(pSet, reference.dynamicOffsets, mesh);
    }
}

void Descriptor::Handler::allocateDescriptorSets(const std::unique_ptr<Descriptor::Set::Base>& pSet,
                                                 Descriptor::Set::Resource& resource) {
    auto setCount = static_cast<uint32_t>(shell().context().imageCount);
    assert(setCount > 0);

    resource.descriptorSets.resize(setCount);
    std::vector<VkDescriptorSetLayout> layouts(setCount, pSet->layout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool_;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    vk::assert_success(vkAllocateDescriptorSets(shell().context().dev, &allocInfo, resource.descriptorSets.data()));
}

void Descriptor::Handler::updateDescriptorSets(const std::unique_ptr<Descriptor::Set::Base>& pSet,
                                               Descriptor::Set::Resource& resource, Mesh::Base& mesh) const {
    // std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos;
    // std::vector<VkBufferView> texelBufferViews;
    std::vector<VkWriteDescriptorSet> writes(pSet->BINDING_MAP.size());

    uint32_t count = 0;
    for (const auto& keyValue : pSet->BINDING_MAP) {
        // WRITE
        writes[count] = getWrite(keyValue);
        // INFO
        if (DESCRIPTOR_MATERIAL_ALL.count(keyValue.second.first)) {
            // MATERIAL
            mesh.pMaterial_->setWriteInfo(writes[count]);
        } else if (DESCRIPTOR_UNIFORM_ALL.count(keyValue.second.first)) {
            // UNIFORM
            bufferInfos.push_back(uniformHandler().getWriteInfos(keyValue.second));
            writes[count].descriptorCount = static_cast<uint32_t>(bufferInfos.back().size());
            writes[count].pBufferInfo = bufferInfos.back().data();
        } else if (DESCRIPTOR_SAMPLER_ALL.count(keyValue.second.first)) {
            // SAMPLER
            // TODO: add "Descriptor::Interface" to texture if it changes to a class...
            assert(mesh.getMaterial()->getTexture() != nullptr);
            writes[count].pImageInfo = &mesh.getMaterial()->getTexture()->imgDescInfo;
        } else {
            assert(false);
            throw std::runtime_error("Unhandled descriptor type");
        }
        ++count;
    }

    for (uint32_t i = 0; i < resource.descriptorSets.size(); i++) {
        for (auto& write : writes) write.dstSet = resource.descriptorSets[i];
        vkUpdateDescriptorSets(shell().context().dev, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void Descriptor::Handler::getDynamicOffsets(const std::unique_ptr<Descriptor::Set::Base>& pSet,
                                            std::vector<uint32_t>& dynamicOffsets, Mesh::Base& mesh) {
    for (auto& keyValue : pSet->BINDING_MAP) {
        if (helpers::isDescriptorTypeDynamic(keyValue.second.first)) {
            switch (keyValue.second.first) {
                case DESCRIPTOR::MATERIAL_DEFAULT:
                case DESCRIPTOR::MATERIAL_PBR: {
                    dynamicOffsets.push_back(mesh.getMaterial()->getDynamicOffset());
                } break;
                default:
                    throw std::runtime_error("Unhandled case");
            }
        }
    }
}

VkWriteDescriptorSet Descriptor::Handler::getWrite(const Descriptor::bindingMapKeyValue& keyValue) const {
    assert(!keyValue.second.second.empty());
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = std::get<1>(keyValue.first);
    write.dstArrayElement = std::get<2>(keyValue.first);
    write.descriptorType = DESCRIPTOR_TYPE_MAP.at(keyValue.second.first);
    write.descriptorCount = getDescriptorCount(keyValue.second);  // TODO: don't do this a second time
    return write;
}

void Descriptor::Handler::reset() {
    // POOL
    if (pool_ != VK_NULL_HANDLE) vkDestroyDescriptorPool(shell().context().dev, pool_, nullptr);
    // LAYOUT
    for (const auto& pSet : pDescriptorSets_)
        if (pSet->layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(shell().context().dev, pSet->layout, nullptr);
}
