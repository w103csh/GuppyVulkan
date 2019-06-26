
#include "DescriptorHandler.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <variant>

#include "Mesh.h"
#include "Parallax.h"
#include "PBR.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "ScreenSpace.h"
// HANDLERS
#include "PipelineHandler.h"
#include "RenderPassHandler.h"
#include "ShaderHandler.h"
#include "TextureHandler.h"
#include "UniformHandler.h"

Descriptor::Handler::Handler(Game* pGame) : Game::Handler(pGame), pool_(VK_NULL_HANDLE) {
    for (const auto& type : Descriptor::Set::ALL) {
        switch (type) {
                // clang-format off
            case DESCRIPTOR_SET::UNIFORM_DEFAULT: pDescriptorSets_.push_back(std::make_unique<Set::Default::Uniform>()); break;
            case DESCRIPTOR_SET::SAMPLER_DEFAULT: pDescriptorSets_.push_back(std::make_unique<Set::Default::Sampler>()); break;
            case DESCRIPTOR_SET::SAMPLER_CUBE_DEFAULT: pDescriptorSets_.push_back(std::make_unique<Set::Default::CubeSampler>()); break;
            case DESCRIPTOR_SET::PROJECTOR_DEFAULT: pDescriptorSets_.push_back(std::make_unique<Set::Default::ProjectorSampler>()); break;
            case DESCRIPTOR_SET::UNIFORM_PBR: pDescriptorSets_.push_back(std::make_unique<Set::PBR::Uniform>()); break;
            case DESCRIPTOR_SET::UNIFORM_PARALLAX: pDescriptorSets_.push_back(std::make_unique<Set::Parallax::Uniform>()); break;
            case DESCRIPTOR_SET::SAMPLER_PARALLAX: pDescriptorSets_.push_back(std::make_unique<Set::Parallax::Sampler>()); break;
            case DESCRIPTOR_SET::UNIFORM_SCREEN_SPACE_DEFAULT: pDescriptorSets_.push_back(std::make_unique<Set::ScreenSpace::DefaultUniform>()); break;
            case DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_DEFAULT: pDescriptorSets_.push_back(std::make_unique<Set::ScreenSpace::DefaultSampler>()); break;
            default: assert(false);  // add new pipelines here
                // clang-format on
        }
        assert(pDescriptorSets_.back()->TYPE == type);
    }
}

void Descriptor::Handler::init() {
    reset();
    createPool();
    createLayouts();
}

void Descriptor::Handler::createPool() {
    // TODO: an actaul allocator that works, and doesn't waste a ton of memory
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
    };
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
        // Resolve the offsets map and init the layout resources.
        prepareDescriptorSet(pSet);

        if (!pSet->isInitialized()) continue;

        for (auto& res : pSet->resources_) {
            // Determine shader stages...
            pipelineHandler().getShaderStages(res.pipelineTypes, res.stages);

            // Gather bindings...
            std::vector<VkDescriptorSetLayoutBinding> bindings;
            for (auto& bindingKeyValue : pSet->BINDING_MAP) {
                auto it = res.offsets.map().find(std::get<0>(bindingKeyValue.second));
                if (it == res.offsets.map().end()) {
                    it = pSet->getDefaultResource().offsets.map().find(std::get<0>(bindingKeyValue.second));
                    assert(it != pSet->getDefaultResource().offsets.map().end());
                }
                auto binding = getDecriptorSetLayoutBinding(bindingKeyValue, res.stages, it->second);
                bindings.push_back(binding);
            }

            // Create layout...
            VkDescriptorSetLayoutCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            vk::assert_success(vkCreateDescriptorSetLayout(shell().context().dev, &createInfo, nullptr, &res.layout));

            if (settings().enable_debug_markers) {
                std::string markerName = " descriptor set layout";  // TODO: a meaningful name
                ext::DebugMarkerSetObjectName(shell().context().dev, (uint64_t)res.layout,
                                              VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, markerName.c_str());
            }
        }
    }
}

uint32_t Descriptor::Handler::getDescriptorCount(const DESCRIPTOR& descType, const Uniform::offsets& offsets) const {
    if (offsets.empty()) {
        // If offsets is empty then it should not be a uniform descriptor, and the
        // count should always be 1. For now.
        assert(!std::visit(IsUniform{}, descType));
        return 1;
    }
    const auto& min = *offsets.begin();
    if (offsets.size() == 1) {
        if (min == Descriptor::Set::OFFSET_ALL) {
            if (std::visit(IsUniform{}, descType))
                return static_cast<uint32_t>(uniformHandler().getDescriptorCount(descType, offsets));
            else
                assert(false && "Unaccounted for scenario");
        }
    }
    return static_cast<uint32_t>(offsets.size());
}

void Descriptor::Handler::prepareDescriptorSet(std::unique_ptr<Descriptor::Set::Base>& pSet) {
    std::set<PIPELINE> pipelineTypes;
    // Gather all of the pipelines that need the set.
    for (const auto& pPipeline : pipelineHandler().getPipelines())
        for (const auto& setType : pPipeline->DESCRIPTOR_SET_TYPES)
            if (pSet->TYPE == setType) pipelineTypes.insert(pPipeline->TYPE);

    // Determine the number of layouts needed based on unique offsets for uniforms. gatherDescriptorSetOffsets
    // will fill out a Resource struct for the default, and all necessary pipeline defaults, and non-defaults.
    for (const auto& keyValue : pSet->BINDING_MAP) {
        for (const auto& pipelineType : pipelineTypes) {
            // Add the offsets to the offsets map (UNIFORMs are the only type that can have unique offsets so far).
            if (std::visit(Descriptor::IsUniform{}, std::get<0>(keyValue.second))) {
                pSet->updateOffsets(uniformHandler().getOffsetsMgr().getOffsetMap(), keyValue, pipelineType);
            }
            // Add the pipeline type to the resource pipeline types set.
            pSet->getDefaultResource().pipelineTypes.insert(pipelineType);
        }
    }

    // Resolve the data in resources added by the previous step with the exisiting resources. This should
    // split and merge the data so that there is less redunancy.
    for (const auto& descPipelineTypeKeyValue : pSet->uniformOffsets_) {
        const auto& descType = descPipelineTypeKeyValue.first.first;
        pipelineTypes.clear();

        for (const auto& [descriptorOffsets, passTypes] : descPipelineTypeKeyValue.second) {
            // Default offsets should never get here.
            assert(
                !(descPipelineTypeKeyValue.first.second == PIPELINE::ALL_ENUM && passTypes == Uniform::RENDER_PASS_ALL_SET));

            pipelineTypes = {descPipelineTypeKeyValue.first.second};

            bool isAllEnum = false;
            // If pipeline type is ALL_ENUM the offsets come from a render pass(es), so ask
            // the render pass(es) what pipeline types are necessary.
            if (*pipelineTypes.begin() == PIPELINE::ALL_ENUM) {
                assert(passTypes != Uniform::RENDER_PASS_ALL_SET);

                // Set a flag so that later you know it came from ALL_ENUM, and remove
                // ALL_ENUM from the list.
                isAllEnum = true;
                pipelineTypes.erase(pipelineTypes.begin());

                // Add the specific types that need to be iterated through for ALL_ENUM.
                for (const auto& passType : passTypes) {
                    assert(passType != RENDER_PASS::ALL_ENUM);
                    passHandler().getPass(passType)->addPipelineTypes(pipelineTypes);
                }
            }

            /* There are two types of offset overrides that need to be resolved at this point.
             *
             *  Pipeline Defaults: These are offsets that are should be used for a specific
             *      pipeline across all passes. They should be declared in the create info for
             *      the pipeline.
             *
             *      PIPELINE::ALL_ENUM should NOT be used.
             *      RENDER_PASS::ALL_ENUM is required.
             *
             *      (ex: PIPELINE::TRI_LIST_COLOR, RENDER_PASS::ALL_ENUM)
             *
             *      TODO: add the create info stuff mentioned above, and validate the types.
             *
             *  Non-defaults: These are offsets that are should be used only for a pipeline
             *      (or all pipelines) and a specfic pass (or multiple passes). They should
             *      be declared in the create info for the pass.
             *
             *      PIPELINE::ALL_ENUM can be used.
             *      RENDER_PASS::ALL_ENUM should NOT be used.
             *
             *      (ex: PIPELINE::ALL_ENUM, RENDER_PASS::SAMPLER)
             *
             *   Non-defaults should override any pipeline defaults, and pipeline defaults
             *   should override the main defualts declared in the OffsetsManager map.
             *
             */
            for (const auto& pipelineType : pipelineTypes) {
                assert(pSet->resources_.size());

                if (passTypes == Uniform::RENDER_PASS_ALL_SET) {
                    // PIPELINE DEFAULT

                    auto itDefault = ++pSet->resources_.begin();
                    pSet->findResourceForDefaultPipeline(itDefault, pipelineType);

                    if (itDefault != pSet->resources_.end()) {
                        // Add incoming to the default.
                        assert(itDefault->offsets.map().count(descType) == 0);
                        itDefault->offsets.insert(descType, descriptorOffsets);
                    } else {
                        // Create the default.
                        pSet->resources_.push_back({});
                        pSet->resources_.back().pipelineTypes.insert(pipelineType);
                        pSet->resources_.back().offsets.insert(descType, descriptorOffsets);
                        pSet->resources_.back().passTypes.insert(passTypes.begin(), passTypes.end());
                        // Update the iterator for use below.
                        itDefault = std::prev(pSet->resources_.end());
                        assert(itDefault->passTypes == Uniform::RENDER_PASS_ALL_SET);
                    }

                    // Update all non-defaults with the new info.
                    auto itNonDefault = ++pSet->resources_.begin();
                    pSet->findResourceForPipeline(itNonDefault, pipelineType);

                    while (itNonDefault != pSet->resources_.end()) {
                        if (itNonDefault->passTypes != Uniform::RENDER_PASS_ALL_SET) {
                            // Update the layout with the default offsets.
                            for (const auto& keyValue : itDefault->offsets.map()) {
                                auto search = itNonDefault->offsets.map().find(keyValue.first);
                                if (search == itNonDefault->offsets.map().end()) {
                                    itNonDefault->offsets.insert(keyValue.first, keyValue.second);
                                } else if (search->second != descriptorOffsets) {
                                    std::stringstream ss;
                                    ss << "Descriptor offsets : {";
                                    for (const auto& offset : descriptorOffsets) ss << offset << ",";
                                    ss << "} conflict with current offsets : {";
                                    for (const auto& offset : search->second) ss << offset << ",";
                                    ss << "}.";
                                    shell().log(Shell::LOG_WARN, ss.str().c_str());
                                }
                            }
                            assert(itNonDefault->offsets.map().size() >= itDefault->offsets.map().size());
                        }
                        pSet->findResourceForPipeline(++itNonDefault, pipelineType);
                    }

                } else {
                    // NON-DEFAULT

                    auto itNonDefault = ++pSet->resources_.begin();
                    pSet->findResourceForPipeline(itNonDefault, pipelineType);

                    // Look for layouts with pass types that contain my pass types. If found then either add
                    // offsets, or split the layout apart.

                    bool workFinished = false;
                    while (itNonDefault != pSet->resources_.end()) {
                        if (itNonDefault->passTypes != Uniform::RENDER_PASS_ALL_SET) {
                            // TODO: Doing it slowly here. Come back and speed it up if you want.

                            if (itNonDefault->passTypes == passTypes) {
                                // The pass types are identical so just add incoming offsets

                                // Make sure the descriptor is not in the map. If it is then there
                                // are conflicting pipeline defaults.
                                assert(itNonDefault->offsets.map().count(descType) == 0);

                                itNonDefault->offsets.insert(descType, descriptorOffsets);

                                workFinished = true;
                                break;

                            } else if (std::find_first_of(itNonDefault->passTypes.begin(), itNonDefault->passTypes.end(),
                                                          passTypes.begin(),
                                                          passTypes.end()) != itNonDefault->passTypes.end()) {
                                // A layout exists for the pipeline that has at least one of the incoming pass types.

                                // Copy the resource
                                std::vector<Descriptor::Set::Resource> newLayoutResources;
                                std::set<RENDER_PASS> tempSet;

                                // Get the set of passes in itNonDefault that are not in the incoming passes.
                                std::set_difference(itNonDefault->passTypes.begin(), itNonDefault->passTypes.end(),
                                                    passTypes.begin(), passTypes.end(),
                                                    std::inserter(tempSet, tempSet.begin()));

                                // Create a new layout that is the same as the old one without the incoming offsets.
                                if (tempSet.size()) {
                                    assert(false);  // Should work but was not tested.
                                    newLayoutResources.push_back(*itNonDefault);
                                    newLayoutResources.back().passTypes.clear();
                                    newLayoutResources.back().passTypes.merge(tempSet);

                                    // Get the set of passes in itNonDefault that not the ones we just removed.
                                    std::set_difference(itNonDefault->passTypes.begin(), itNonDefault->passTypes.end(),
                                                        newLayoutResources.back().passTypes.begin(),
                                                        newLayoutResources.back().passTypes.end(),
                                                        std::inserter(tempSet, tempSet.begin()));

                                    // I think the new split-off layout should not need to be merged forward
                                    // at this point but its hard to know for certain.
                                }

                                // If tempSet has something in it then the existing layout was split, so
                                // update the passes in the original, so it does not contain the split off
                                // passes.
                                if (tempSet.size()) {
                                    itNonDefault->passTypes.clear();
                                    itNonDefault->passTypes.merge(tempSet);
                                }

                                // If this asserts then why did you initialize it like that. It could be done
                                // in fewer declarations.
                                assert(itNonDefault->offsets.map().count(descType) == 0);
                                itNonDefault->offsets.insert(descType, descriptorOffsets);

                                std::set_difference(passTypes.begin(), passTypes.end(), itNonDefault->passTypes.begin(),
                                                    itNonDefault->passTypes.end(), std::inserter(tempSet, tempSet.begin()));

                                // Create a new layout that only has the pass types and offsets that were not merged in
                                // already.
                                if (tempSet.size()) {
                                    newLayoutResources.push_back({});
                                    newLayoutResources.back().pipelineTypes.insert(pipelineType);
                                    if (isAllEnum) newLayoutResources.back().pipelineTypes.insert(PIPELINE::ALL_ENUM);
                                    newLayoutResources.back().offsets.insert(descType, descriptorOffsets);
                                    newLayoutResources.back().passTypes.merge(tempSet);
                                }

                                assert(newLayoutResources.size());  // Something new should have been created.
                                pSet->resources_.insert(pSet->resources_.end(), newLayoutResources.begin(),
                                                        newLayoutResources.end());

                                workFinished = true;
                                break;
                            }
                        }
                        pSet->findResourceForPipeline(++itNonDefault, pipelineType);
                    }

                    if (!workFinished) {
                        itNonDefault = ++pSet->resources_.begin();
                        pSet->findResourceSimilar(itNonDefault, pipelineType, passTypes, descType, descriptorOffsets);

                        if (itNonDefault != pSet->resources_.end()) {
                            itNonDefault->pipelineTypes.insert(pipelineType);
                            workFinished = true;
                        }
                    }

                    // Offsets were not dealt with using the routines above for mergeing or spliting, so just
                    // create a new one here.
                    if (!workFinished) {
                        auto itDefault = ++pSet->resources_.begin();
                        pSet->findResourceForDefaultPipeline(itDefault, pipelineType);

                        if (itDefault != pSet->resources_.end()) {
                            // Copy from default and clear the pass types.
                            pSet->resources_.push_back(*itDefault);
                            pSet->resources_.back().passTypes.clear();
                        } else {
                            // Create a new one if no default is found.
                            pSet->resources_.push_back({});
                        }

                        // Pipeline type. Add ALL_ENUM if that was the origin, so that better
                        // merging tests can be done.
                        pSet->resources_.back().pipelineTypes.insert(pipelineType);
                        if (isAllEnum) pSet->resources_.back().pipelineTypes.insert(PIPELINE::ALL_ENUM);
                        // Add/overwrite the offsets
                        pSet->resources_.back().offsets.insert(descType, descriptorOffsets);
                        // Pass types
                        pSet->resources_.back().passTypes.insert(passTypes.begin(), passTypes.end());
                    }
                }
            }
        }
    }
}

Descriptor::Set::resourceHelpers Descriptor::Handler::getResourceHelpers(const std::set<RENDER_PASS> passTypes,
                                                                         const PIPELINE& pipelineType,
                                                                         const std::vector<DESCRIPTOR_SET>& descSetTypes) {
    // I don't care that this is redundant and slow atm.

    Set::resourceHelpers helpers;
    for (const auto& setType : descSetTypes) {
        const auto& pSet = getSet(setType);

        // Organize the descriptor set layout resources in a temporary structure that
        // will be used to attempt to cull redunancies.
        helpers.push_back({});
        for (const auto& passType : passTypes) {
            // Look for non-default offsets first
            uint32_t i = 1;
            for (; i < pSet->resources_.size(); i++) {
                const auto& res = pSet->resources_[i];
                if (res.pipelineTypes.count(pipelineType) && res.passTypes.count(passType)) {
                    helpers.back().push_back({{passType}, &res, i, pSet->TYPE, res.passTypes});
                    break;
                }
            }
            if (i == pSet->resources_.size()) {
                // Look for default offsets second
                for (i = 1; i < pSet->resources_.size(); i++) {
                    const auto& res = pSet->resources_[i];
                    if (res.pipelineTypes.count(pipelineType) && res.passTypes == Uniform::RENDER_PASS_ALL_SET) {
                        helpers.back().push_back({{passType}, &res, i, pSet->TYPE, res.passTypes});
                        break;
                    }
                }
                // Add the default if no override exist
                if (i == pSet->resources_.size()) {
                    helpers.back().push_back(
                        {{passType}, &pSet->resources_.front(), 0, pSet->TYPE, Uniform::RENDER_PASS_ALL_SET});
                }
            }
        }

        assert(helpers.back().size() == passTypes.size());
    }

    // Cull redundant sets for layout.

    // Resource index0 for comparison with index1.
    for (auto resourceIndex0 = 0; resourceIndex0 < helpers.front().size(); resourceIndex0++) {
        // Resource index1 for comparison with index0.
        for (auto resourceIndex1 = resourceIndex0 + 1; resourceIndex1 < helpers.front().size(); resourceIndex1++) {
            auto setIndex = 0;
            // Compare each set at index0 with set at index1.
            for (; setIndex < helpers.size(); setIndex++) {
                if (std::get<4>(helpers[setIndex][resourceIndex0]) != std::get<4>(helpers[setIndex][resourceIndex1])) break;
            }
            if (setIndex == helpers.size()) {
                for (setIndex = 0; setIndex < helpers.size(); setIndex++) {
                    auto it = helpers[setIndex].begin() + resourceIndex1;
                    // Copy the pass types from the helper to be erased, to retain the info about the pass type the
                    // helper was created for.
                    std::get<0>(helpers[setIndex][resourceIndex0]).insert(std::get<0>(*it).begin(), std::get<0>(*it).end());
                    helpers[setIndex].erase(it);
                }
                --resourceIndex1;
            }
        }
    }

    assert(helpers.size());
    return helpers;
}

VkDescriptorSetLayoutBinding Descriptor::Handler::getDecriptorSetLayoutBinding(
    const Descriptor::bindingMapKeyValue& keyValue, const VkShaderStageFlags& stageFlags,
    const Uniform::offsets& offsets) const {
    VkDescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = keyValue.first.first;
    layoutBinding.descriptorType = std::visit(GetVkType{}, std::get<0>(keyValue.second));
    layoutBinding.descriptorCount = getDescriptorCount(std::get<0>(keyValue.second), offsets);
    layoutBinding.stageFlags = stageFlags;
    layoutBinding.pImmutableSamplers = nullptr;  // TODO: use this
    return layoutBinding;
}

// TODO: add params that can indicate to free/reallocate
void Descriptor::Handler::getBindData(Mesh::Base& mesh) {
    mesh.descriptorBindDataMap_.clear();

    // Get the same descriptor sets info as the pipeline layouts.
    auto helpers = getResourceHelpers(passHandler().getActivePassTypes(mesh.PIPELINE_TYPE), mesh.PIPELINE_TYPE,
                                      pipelineHandler().getPipeline(mesh.PIPELINE_TYPE)->DESCRIPTOR_SET_TYPES);

    for (const auto& resourceTuples : helpers) {
        assert(resourceTuples.size());
        auto& pSet = getSet(std::get<3>(resourceTuples.front()));

        // Check for a texture offset.
        uint32_t textureOffset = 0;
        if (pSet->hasTextureMaterial()) {
            assert(mesh.getMaterial()->hasTexture());
            textureOffset = mesh.getMaterial()->getTexture()->OFFSET;
        }

        // Get/create the resource offset map for the material (or 0 if no material).
        if (pSet->descriptorSetsMap_.count(textureOffset) == 0) {
            pSet->descriptorSetsMap_.insert({textureOffset, {}});
        }
        auto& resourceOffsetsMap = pSet->descriptorSetsMap_.at(textureOffset);

        for (const auto& resourceTuple : resourceTuples) {
            // Get/create the descriptor sets map element for the resource.
            if (resourceOffsetsMap.count(std::get<2>(resourceTuple)) == 0) {
                resourceOffsetsMap.insert({std::get<2>(resourceTuple), {}});

                allocateDescriptorSets(*std::get<1>(resourceTuple), resourceOffsetsMap.at(std::get<2>(resourceTuple)));
                updateDescriptorSets(pSet->BINDING_MAP, pSet->getDescriptorOffsets(std::get<2>(resourceTuple)),
                                     resourceOffsetsMap.at(std::get<2>(resourceTuple)), &mesh);
            }

            // Add bind data for mesh

            if (mesh.descriptorBindDataMap_.count(std::get<0>(resourceTuple)) == 0) {
                mesh.descriptorBindDataMap_.insert(
                    {std::get<0>(resourceTuple),
                     {0, std::vector<std::vector<VkDescriptorSet>>(shell().context().imageCount)}});
            }
            auto& bindData = mesh.descriptorBindDataMap_.at(std::get<0>(resourceTuple));

            assert(bindData.descriptorSets.size() == resourceOffsetsMap.at(std::get<2>(resourceTuple)).size());
            for (auto i = 0; i < bindData.descriptorSets.size(); i++) {
                bindData.descriptorSets[i].push_back(resourceOffsetsMap.at(std::get<2>(resourceTuple))[i]);
            }
            getDynamicOffsets(pSet, bindData.dynamicOffsets, mesh);
        }
    }
}

void Descriptor::Handler::updateBindData(const std::shared_ptr<Texture::Base>& pTexture) {
    // Find the descriptor set(s)
    for (const auto& pSet : pDescriptorSets_) {
        for (const auto& [key, descTypeTextureIdPair] : pSet->BINDING_MAP) {
            if (std::get<1>(descTypeTextureIdPair) == pTexture->NAME) {
                // Not trying to make this work for anything else atm.
                assert(std::visit(IsCombinedSamplerPipeline{}, std::get<0>(descTypeTextureIdPair)));
                assert(pSet->descriptorSetsMap_.size() == 1);
                for (auto& [resourceOffset, descriptorSets] : pSet->descriptorSetsMap_.at(0)) {
                    updateDescriptorSets(pSet->BINDING_MAP, pSet->getDescriptorOffsets(resourceOffset), descriptorSets);
                }
            }
        }
    }
}

void Descriptor::Handler::allocateDescriptorSets(const Descriptor::Set::Resource& resource,
                                                 std::vector<VkDescriptorSet>& descriptorSets) {
    auto setCount = static_cast<uint32_t>(shell().context().imageCount);
    assert(setCount > 0);

    descriptorSets.resize(setCount);
    std::vector<VkDescriptorSetLayout> layouts(setCount, resource.layout);

    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pool_;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    vk::assert_success(vkAllocateDescriptorSets(shell().context().dev, &allocInfo, descriptorSets.data()));
}

void Descriptor::Handler::updateDescriptorSets(const Descriptor::bindingMap& bindingMap,
                                               const Descriptor::OffsetsMap& offsets,
                                               std::vector<VkDescriptorSet>& descriptorSets, const Mesh::Base* pMesh) const {
    // std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<std::vector<VkDescriptorBufferInfo>> bufferInfos;
    // std::vector<VkBufferView> texelBufferViews;
    std::vector<VkWriteDescriptorSet> writes;

    uint32_t samplerIndex = 0;
    for (const auto& keyValue : bindingMap) {
        const auto& descType = std::get<0>(keyValue.second);

        // Set binding info
        writes.push_back(getWrite(keyValue));

        // This stuff is not great but oh well.
        const Uniform::offsets* pOffsets = nullptr;
        if (std::visit(IsCombinedSampler{}, descType)) {
            // There are no offsets for samplers. Yikes. Maybe one day...
            assert(samplerIndex < offsets.map().count(descType));
            auto it = offsets.map().equal_range(descType).first;
            std::advance(it, samplerIndex);
            pOffsets = &it->second;
        } else {
            assert(offsets.map().count(descType) == 1);
            pOffsets = &offsets.map().find(descType)->second;
        }
        assert(pOffsets != nullptr);

        writes.back().descriptorCount = getDescriptorCount(descType, *pOffsets);

        // WRITE INFO
        if (std::visit(IsUniformDynamic{}, descType)) {
            // MATERIAL
            assert(pMesh != nullptr);
            pMesh->pMaterial_->setWriteInfo(writes.back());
        } else if (std::visit(IsUniform{}, descType)) {
            // UNIFORM
            bufferInfos.push_back(uniformHandler().getWriteInfos(descType, *pOffsets));
            writes.back().descriptorCount = static_cast<uint32_t>(bufferInfos.back().size());
            writes.back().pBufferInfo = bufferInfos.back().data();
        } else if (std::visit(IsCombinedSamplerMaterial{}, std::get<0>(keyValue.second))) {
            // MATERIAL SAMPLER
            assert(pMesh != nullptr && pMesh->getMaterial()->hasTexture());
            pMesh->getMaterial()->getTexture()->setWriteInfo(writes.back(), samplerIndex++);
        } else if (std::visit(IsCombinedSamplerPipeline{}, descType)) {
            // PIPELINE SAMPLER
            textureHandler().getTextureByName(std::get<1>(keyValue.second))->setWriteInfo(writes.back(), samplerIndex++);
        } else {
            assert(false);
            throw std::runtime_error("Unhandled descriptor type");
        }
    }

    for (uint32_t i = 0; i < descriptorSets.size(); i++) {
        for (auto& write : writes) write.dstSet = descriptorSets[i];
        vkUpdateDescriptorSets(shell().context().dev, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

VkWriteDescriptorSet Descriptor::Handler::getWrite(const Descriptor::bindingMapKeyValue& keyValue) const {
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = keyValue.first.first;
    write.dstArrayElement = keyValue.first.second;
    write.descriptorType = std::visit(GetVkType{}, std::get<0>(keyValue.second));
    return write;
}

void Descriptor::Handler::getDynamicOffsets(const std::unique_ptr<Descriptor::Set::Base>& pSet,
                                            std::vector<uint32_t>& dynamicOffsets, Mesh::Base& mesh) {
    for (auto& keyValue : pSet->BINDING_MAP) {
        if (std::visit(IsUniformDynamic{}, std::get<0>(keyValue.second))) {
            dynamicOffsets.push_back(static_cast<uint32_t>(mesh.getMaterial()->BUFFER_INFO.memoryOffset));
        }
    }
}

void Descriptor::Handler::reset() {
    // POOL
    if (pool_ != VK_NULL_HANDLE) vkDestroyDescriptorPool(shell().context().dev, pool_, nullptr);
    // LAYOUT
    for (const auto& pSet : pDescriptorSets_) {
        for (auto& res : pSet->resources_) {
            if (res.layout != VK_NULL_HANDLE)  //
                vkDestroyDescriptorSetLayout(shell().context().dev, res.layout, nullptr);
        }
    }
}
