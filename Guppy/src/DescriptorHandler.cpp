/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "DescriptorHandler.h"

#include <algorithm>
#include <set>
#include <sstream>
#include <utility>
#include <variant>

#include "Cloth.h"
#include "Deferred.h"
#include "FFT.h"
#include "Geometry.h"
#include "HeightFieldFluid.h"
#include "Mesh.h"
#include "Ocean.h"
#include "OceanComputeWork.h"
#include "Parallax.h"
#include "Particle.h"
#include "PBR.h"
#include "Pipeline.h"
#include "RenderPass.h"
#include "ScreenSpace.h"
#include "Shadow.h"
#include "Tessellation.h"
// HANDLERS
#include "ParticleHandler.h"
#include "PipelineHandler.h"
#include "PassHandler.h"
#include "ShaderHandler.h"
#include "TextureHandler.h"
#include "UniformHandler.h"

Descriptor::Handler::Handler(Game* pGame) : Game::Handler(pGame), pool_() {
    for (const auto& type : Descriptor::Set::ALL) {
        // clang-format off
        switch (type) {
            case DESCRIPTOR_SET::UNIFORM_DEFAULT:                           pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Default::UNIFORM_CREATE_INFO)); break;
            case DESCRIPTOR_SET::UNIFORM_CAMERA_ONLY:                       pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Default::UNIFORM_CAMERA_ONLY_CREATE_INFO)); break;
            case DESCRIPTOR_SET::UNIFORM_DEFCAM_DEFMAT_MX4:                 pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Default::UNIFORM_DEFCAM_DEFMAT_MX4)); break;
            case DESCRIPTOR_SET::UNIFORM_CAM_MATOBJ3D:                      pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Default::UNIFORM_CAM_MATOBJ3D_CREATE_INFO)); break;
            case DESCRIPTOR_SET::UNIFORM_OBJ3D:                             pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Default::UNIFORM_OBJ3D_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SAMPLER_DEFAULT:                           pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Default::SAMPLER_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SAMPLER_CUBE_DEFAULT:                      pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Default::CUBE_SAMPLER_CREATE_INFO)); break;
            case DESCRIPTOR_SET::PROJECTOR_DEFAULT:                         pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Default::PROJECTOR_SAMPLER_CREATE_INFO)); break;
            case DESCRIPTOR_SET::CAMERA_CUBE_MAP:                           pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Default::CAM_CUBE_CREATE_INFO)); break;
            case DESCRIPTOR_SET::UNIFORM_PBR:                               pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::PBR::UNIFORM_CREATE_INFO)); break;
            case DESCRIPTOR_SET::UNIFORM_PARALLAX:                          pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Parallax::UNIFORM_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SAMPLER_PARALLAX:                          pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Parallax::SAMPLER_CREATE_INFO)); break;
            case DESCRIPTOR_SET::UNIFORM_SCREEN_SPACE_DEFAULT:              pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::ScreenSpace::DEFAULT_UNIFORM_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_DEFAULT:              pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::ScreenSpace::DEFAULT_SAMPLER_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_HDR_LOG_BLIT:         pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::ScreenSpace::HDR_BLIT_SAMPLER_CREATE_INFO)); break;
            case DESCRIPTOR_SET::STORAGE_SCREEN_SPACE_COMPUTE_POST_PROCESS: pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::ScreenSpace::STORAGE_COMPUTE_POST_PROCESS_CREATE_INFO)); break;
            case DESCRIPTOR_SET::STORAGE_IMAGE_SCREEN_SPACE_COMPUTE_DEFAULT:pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::ScreenSpace::STORAGE_IMAGE_COMPUTE_DEFAULT_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_BLUR_A:               pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::ScreenSpace::BLUR_A_SAMPLER_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_BLUR_B:               pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::ScreenSpace::BLUR_B_SAMPLER_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SWAPCHAIN_IMAGE:                           pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::RenderPass::SWAPCHAIN_IMAGE_CREATE_INFO)); break;
            case DESCRIPTOR_SET::UNIFORM_DEFERRED_MRT:                      pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Deferred::MRT_UNIFORM_CREATE_INFO)); break;
            case DESCRIPTOR_SET::UNIFORM_DEFERRED_SSAO:                     pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Deferred::SSAO_UNIFORM_CREATE_INFO)); break;
            case DESCRIPTOR_SET::UNIFORM_DEFERRED_COMBINE:                  pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Deferred::COMBINE_UNIFORM_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SAMPLER_DEFERRED:                          pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Deferred::SAMPLER_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SAMPLER_DEFERRED_SSAO_RANDOM:              pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Deferred::SSAO_RANDOM_SAMPLER_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SHADOW_CUBE_UNIFORM_ONLY:                  pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Shadow::CUBE_UNIFORM_ONLY_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SHADOW_CUBE_ALL:                           pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Shadow::CUBE_ALL_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SAMPLER_SHADOW:                            pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Shadow::SAMPLER_CREATE_INFO)); break;
            case DESCRIPTOR_SET::SAMPLER_SHADOW_OFFSET:                     pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Shadow::SAMPLER_OFFSET_CREATE_INFO)); break;
            case DESCRIPTOR_SET::TESS_DEFAULT:                              pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Tessellation::DEFAULT_CREATE_INFO)); break;
            case DESCRIPTOR_SET::TESS_PHONG:                                pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Tessellation::PHONG_CREATE_INFO)); break;
            case DESCRIPTOR_SET::UNIFORM_GEOMETRY_DEFAULT:                  pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Geometry::DEFAULT_CREATE_INFO)); break;
            case DESCRIPTOR_SET::UNIFORM_PRTCL_WAVE:                        pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Particle::WAVE_CREATE_INFO)); break;
            case DESCRIPTOR_SET::UNIFORM_PRTCL_FOUNTAIN:                    pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Particle::FOUNTAIN_CREATE_INFO)); break;
            case DESCRIPTOR_SET::PRTCL_EULER:                               pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Particle::EULER_CREATE_INFO)); break;
            case DESCRIPTOR_SET::PRTCL_ATTRACTOR:                           pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Particle::ATTRACTOR_CREATE_INFO)); break;
            case DESCRIPTOR_SET::PRTCL_CLOTH:                               pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Particle::CLOTH_CREATE_INFO)); break;
            case DESCRIPTOR_SET::PRTCL_CLOTH_NORM:                          pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::Particle::CLOTH_NORM_CREATE_INFO)); break;
            case DESCRIPTOR_SET::HFF:                                       pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::HFF_CREATE_INFO)); break;
            case DESCRIPTOR_SET::HFF_DEF:                                   pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::HFF_DEF_CREATE_INFO)); break;
            case DESCRIPTOR_SET::FFT_DEFAULT:                               pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::FFT_DEFAULT_CREATE_INFO)); break;
            case DESCRIPTOR_SET::OCEAN_DISPATCH:                            pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::OCEAN_DISPATCH_CREATE_INFO)); break;
            case DESCRIPTOR_SET::OCEAN_DRAW:                                pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::OCEAN_DRAW_CREATE_INFO)); break;
            case DESCRIPTOR_SET::CDLOD_DEFAULT:                             pDescriptorSets_.emplace_back(new Set::Base(std::ref(*this), &Set::CDLOD_DEFAULT_CREATE_INFO)); break;
            default: assert(false);  // add new pipelines here
        }
        // clang-format on
        assert(pDescriptorSets_.back()->TYPE == type);
    }
}

void Descriptor::Handler::init() {
    reset();
    createPool();
    createLayouts();

    assert(shell().context().imageCount == 3);  // Potential imageCount problem
    // Determine the number of sets required
    for (auto& pSet : pDescriptorSets_) pSet->update(shell().context().imageCount);
}

void Descriptor::Handler::createPool() {
    // TODO: an actaul allocator that works, and doesn't waste a ton of memory
    std::vector<vk::DescriptorPoolSize> poolSizes = {
        {vk::DescriptorType::eSampler, 1000},
        {vk::DescriptorType::eCombinedImageSampler, 1000},
        {vk::DescriptorType::eSampledImage, 1000},
        {vk::DescriptorType::eStorageImage, 1000},
        {vk::DescriptorType::eUniformTexelBuffer, 1000},
        {vk::DescriptorType::eStorageTexelBuffer, 1000},
        {vk::DescriptorType::eUniformBuffer, 1000},
        {vk::DescriptorType::eStorageBuffer, 1000},
        {vk::DescriptorType::eUniformBufferDynamic, 1000},
        {vk::DescriptorType::eStorageBufferDynamic, 1000},
        {vk::DescriptorType::eInputAttachment, 1000},
    };
    vk::DescriptorPoolCreateInfo poolInfo = {};
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolInfo.maxSets = 1000 * poolSizes.size();
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    pool_ = shell().context().dev.createDescriptorPool(poolInfo, shell().context().pAllocator);
}

/* OLD POOL LOGIC THAT HAD A BAD ATTEMPT AT AN ALLOCATION STRATEGY
 */
// void Descriptor::Handler::createDescriptorPool(std::unique_ptr<DescriptorResources> & pRes) {
//    const auto& imageCount = shell().context().image_count;
//    uint32_t maxSets = (pRes->colorCount * imageCount) + (pRes->texCount * imageCount);
//
//    std::vector<vk::DescriptorPoolSize> poolSizes;
//    vk::DescriptorPoolSize poolSize;
//
//    poolSize = {};
//    poolSize.type = vk::DescriptorType::eUniformBuffer;
//    poolSize.descriptorCount = (pRes->colorCount + pRes->texCount) * imageCount;
//    poolSizes.push_back(poolSize);
//
//    poolSize = {};
//    poolSize.type = vk::DescriptorType::eCombinedImageSampler;
//    poolSize.descriptorCount = pRes->texCount * imageCount;
//    poolSizes.push_back(poolSize);
//
//    poolSize = {};
//    poolSize.type = vk::DescriptorType::eUniformBufferDynamic;
//    poolSize.descriptorCount = pRes->texCount * imageCount;
//    poolSizes.push_back(poolSize);
//
//    vk::DescriptorPoolCreateInfo desc_pool_info = {};
//    desc_pool_info.maxSets = maxSets;
//    desc_pool_info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
//    desc_pool_info.pPoolSizes = poolSizes.data();
//
//    pRes->pool = shell().context().dev.createDescriptorPool(desc_pool_info, shell().context().pAllocator);
//}

void Descriptor::Handler::createLayouts() {
    for (auto& pSet : pDescriptorSets_) {
        // Resolve the offsets map and init the layout resources.
        prepareDescriptorSet(pSet);

        if (!pSet->isInitialized()) continue;

        for (auto& res : pSet->resources_) {
            // Determine shader stages...
            pipelineHandler().getShaderStages(res.pipelineTypes, pSet->TYPE, res.stages);

            // Gather bindings...
            std::vector<vk::DescriptorSetLayoutBinding> bindings;
            for (auto& bindingKeyValue : pSet->getBindingMap()) {
                auto it = res.offsets.map().find(bindingKeyValue.second.descType);
                if (it == res.offsets.map().end()) {
                    it = pSet->getDefaultResource().offsets.map().find(bindingKeyValue.second.descType);
                    assert(it != pSet->getDefaultResource().offsets.map().end());
                }
                auto binding = getDecriptorSetLayoutBinding(bindingKeyValue, res.stages, it->second);
                bindings.push_back(binding);
            }

            // Create layout...
            vk::DescriptorSetLayoutCreateInfo createInfo = {};
            createInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            createInfo.pBindings = bindings.data();

            res.layout = shell().context().dev.createDescriptorSetLayout(createInfo, shell().context().pAllocator);
            std::string markerName = "";  // TODO: a meaningful name
            // shell().context().dbg.setMarkerName(res.layout, markerName.c_str());
        }
    }
}

uint32_t Descriptor::Handler::getDescriptorCount(const DESCRIPTOR& descType, const Uniform::offsets& offsets) const {
    if (offsets.empty()) {
        // If offsets is empty then it should not have offsets, and the
        // count should always be 1. For now.
        assert(!std::visit(HasOffsets{}, descType));
        return 1;
    }
    const auto& min = *offsets.begin();
    if (offsets.size() == 1) {
        if (min == Descriptor::Set::OFFSET_ALL) {
            if (std::visit(HasOffsets{}, descType))
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
    for (const auto& [pipelineType, pPipeline] : pipelineHandler().getPipelines())
        for (const auto& [setType, stageFlags] : pPipeline->DESC_SET_STAGE_PAIRS)
            if (pSet->TYPE == setType) pipelineTypes.insert(pPipeline->TYPE);

    // Determine the number of layouts needed based on unique offsets for uniforms. gatherDescriptorSetOffsets
    // will fill out a Resource struct for the default, and all necessary pipeline defaults, and non-defaults.
    for (const auto& [key, bindingInfo] : pSet->getBindingMap()) {
        for (const auto& pipelineType : pipelineTypes) {
            // Add the offsets to the offsets map (UNIFORMs are the only type that can have unique offsets so far).
            if (std::visit(Descriptor::HasOffsets{}, bindingInfo.descType)) {
                pSet->updateOffsets(uniformHandler().getOffsetsMgr().getOffsetMap(), bindingInfo.descType, pipelineType);
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
            assert(!(descPipelineTypeKeyValue.first.second == PIPELINE{GRAPHICS::ALL_ENUM} &&
                     passTypes == Uniform::PASS_ALL_SET));

            pipelineTypes = {descPipelineTypeKeyValue.first.second};

            bool isAllEnum = false;
            // If pipeline type is ALL_ENUM the offsets come from a render pass(es), so ask
            // the render pass(es) what pipeline types are necessary.
            if (*pipelineTypes.begin() == PIPELINE{GRAPHICS::ALL_ENUM}) {
                assert(passTypes != Uniform::PASS_ALL_SET);

                // Set a flag so that later you know it came from ALL_ENUM, and remove
                // ALL_ENUM from the list.
                isAllEnum = true;
                pipelineTypes.erase(pipelineTypes.begin());

                // Add the specific types that need to be iterated through for ALL_ENUM.
                for (const auto& passType : passTypes) {
                    assert(passType != PASS{RENDER_PASS::ALL_ENUM});
                    passHandler().addPipelineTypes(passType, pipelineTypes);
                }
            }

            /* There are two types of offset overrides that need to be resolved at this point.
             *
             *  Pipeline Defaults: These are offsets that should be used for a specific
             *      pipeline across all passes. They should be declared in the create info for
             *      the pipeline.
             *
             *      GRAPHICS::ALL_ENUM should NOT be used.
             *      RENDER_PASS::ALL_ENUM is required.
             *
             *      (ex: GRAPHICS::TRI_LIST_COLOR, RENDER_PASS::ALL_ENUM)
             *
             *      TODO: add the create info stuff mentioned above, and validate the types.
             *
             *  Non-defaults: These are offsets that should be used only for a pipeline
             *      (or all pipelines) and a specfic pass (or multiple passes). They should
             *      be declared in the create info for the pass.
             *
             *      GRAPHICS::ALL_ENUM can be used.
             *      RENDER_PASS::ALL_ENUM should NOT be used.
             *
             *      (ex: GRAPHICS::ALL_ENUM, RENDER_PASS::SAMPLER)
             *
             *   Non-defaults should override any pipeline defaults, and pipeline defaults
             *   should override the main defaults declared in the OffsetsManager map.
             *
             */
            for (const auto& pipelineType : pipelineTypes) {
                assert(pSet->resources_.size());

                if (passTypes == Uniform::PASS_ALL_SET) {
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
                        assert(itDefault->passTypes == Uniform::PASS_ALL_SET);
                    }

                    // Update all non-defaults with the new info.
                    auto itNonDefault = ++pSet->resources_.begin();
                    pSet->findResourceForPipeline(itNonDefault, pipelineType);

                    while (itNonDefault != pSet->resources_.end()) {
                        if (itNonDefault->passTypes != Uniform::PASS_ALL_SET) {
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
                                    shell().log(Shell::LogPriority::LOG_WARN, ss.str().c_str());
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
                        if (itNonDefault->passTypes != Uniform::PASS_ALL_SET) {
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
                                std::set<PASS> tempSet;

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
                                    if (isAllEnum) newLayoutResources.back().pipelineTypes.insert(GRAPHICS::ALL_ENUM);
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
                        if (isAllEnum) pSet->resources_.back().pipelineTypes.insert(GRAPHICS::ALL_ENUM);
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

Descriptor::Set::resourceHelpers Descriptor::Handler::getResourceHelpers(
    const std::set<PASS> passTypes, const PIPELINE& pipelineType,
    const Descriptor::Set::typeShaderStagePairs& descSetStagePairs) const {
    // I don't care that this is redundant and slow atm.

    Set::resourceHelpers helpers;
    for (const auto& [setType, stageFlags] : descSetStagePairs) {
        const auto& set = getDescriptorSet(setType);

        // Organize the descriptor set layout resources in a temporary structure that
        // will be used to attempt to cull redunancies.
        helpers.push_back({});
        for (const auto& passType : passTypes) {
            // Look for non-default offsets first
            uint32_t i = 1;
            for (; i < set.resources_.size(); i++) {
                const auto& res = set.resources_[i];
                if (res.pipelineTypes.count(pipelineType) && res.passTypes.count(passType)) {
                    helpers.back().push_back({{passType}, &res, i, set.TYPE, res.passTypes});
                    break;
                }
            }
            if (i == set.resources_.size()) {
                // Look for default offsets second
                for (i = 1; i < set.resources_.size(); i++) {
                    const auto& res = set.resources_[i];
                    if (res.pipelineTypes.count(pipelineType) && res.passTypes == Uniform::PASS_ALL_SET) {
                        helpers.back().push_back({{passType}, &res, i, set.TYPE, res.passTypes});
                        break;
                    }
                }
                // Add the default if no override exist
                if (i == set.resources_.size()) {
                    helpers.back().push_back({{passType}, &set.resources_.front(), 0, set.TYPE, Uniform::PASS_ALL_SET});
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
                if (helpers[setIndex][resourceIndex0].passTypes2 != helpers[setIndex][resourceIndex1].passTypes2) break;
            }
            if (setIndex == helpers.size()) {
                for (setIndex = 0; setIndex < helpers.size(); setIndex++) {
                    auto it = helpers[setIndex].begin() + resourceIndex1;
                    // Copy the pass types from the helper to be erased, to retain the info about the pass type the
                    // helper was created for.
                    helpers[setIndex][resourceIndex0].passTypes1.insert(it->passTypes1.begin(), it->passTypes1.end());
                    helpers[setIndex].erase(it);
                }
                --resourceIndex1;
            }
        }
    }

    assert(helpers.size());
    return helpers;
}

vk::DescriptorSetLayoutBinding Descriptor::Handler::getDecriptorSetLayoutBinding(
    const Descriptor::bindingMapKeyValue& keyValue, const vk::ShaderStageFlags& stageFlags,
    const Uniform::offsets& offsets) const {
    vk::DescriptorSetLayoutBinding layoutBinding = {};
    layoutBinding.binding = keyValue.first.first;
    layoutBinding.descriptorType = std::visit(GetVulkanDescriptorType{}, keyValue.second.descType);
    layoutBinding.descriptorCount = getDescriptorCount(keyValue.second.descType, offsets);
    layoutBinding.stageFlags = stageFlags;
    layoutBinding.pImmutableSamplers = nullptr;  // TODO: use this
    return layoutBinding;
}

// TODO: add params that can indicate to free/reallocate
void Descriptor::Handler::getBindData(const PIPELINE& pipelineType, Descriptor::Set::bindDataMap& bindDataMap,
                                      const std::vector<Descriptor::Base*> pDynamicItems) {
    bindDataMap.clear();

    // Get a list of active passes for the pipeline type.
    std::set<PASS> passTypes;
    passHandler().getActivePassTypes(passTypes, pipelineType);

    assert(passTypes.size() && "No active pass types contain the pipeline type");

    // Get the same descriptor sets info as the pipeline layouts.
    auto resHelpers =
        getResourceHelpers(passTypes, pipelineType, pipelineHandler().getPipeline(pipelineType)->DESC_SET_STAGE_PAIRS);

    for (const auto& helpers : resHelpers) {
        assert(helpers.size());
        auto& pSet = getSet(helpers.front().setType);

        /**
         * Determine a unique key for the descriptor set bind data based on the dynamic descriptors that are required for the
         * by the user.
         */
        Set::mapKey descriptorSetsMapKey = {};
        auto itDescSetsMapKey = descriptorSetsMapKey.begin();
        auto itCmbSampMat = pDynamicItems.begin();
        auto itStrBuffDyn = pDynamicItems.begin();
        for (const auto& [key, bindingInfo] : pSet->getBindingMap()) {
            if (std::visit(IsCombinedSamplerMaterial{}, bindingInfo.descType)) {
                // MATERIAL SAMPLER
                itCmbSampMat = std::find_if(itCmbSampMat, pDynamicItems.end(), [](const auto& pItem) {
                    return std::visit(IsMaterial{}, pItem->getDescriptorType());
                });
                if (itDescSetsMapKey != descriptorSetsMapKey.end() && itCmbSampMat != pDynamicItems.end() &&
                    static_cast<Material::Base*>(*itCmbSampMat)->getTexture() != nullptr) {
                    *itDescSetsMapKey = static_cast<Material::Base*>(*itCmbSampMat)->getTexture()->OFFSET;
                    itDescSetsMapKey++;
                    itCmbSampMat++;
                } else {
                    assert(false && "No space left in key, or no material for the sampler.");
                    exit(EXIT_FAILURE);
                }
            } else if (std::visit(IsStorageBufferDynamic{}, bindingInfo.descType)) {
                // DYNAMIC STORAGE BUFFER
                itStrBuffDyn = std::find_if(itStrBuffDyn, pDynamicItems.end(), [](const auto& pItem) {
                    return std::visit(IsStorageBufferDynamic{}, pItem->getDescriptorType());
                });
                if (itDescSetsMapKey != descriptorSetsMapKey.end() && itStrBuffDyn != pDynamicItems.end()) {
                    *itDescSetsMapKey = static_cast<uint32_t>((*itStrBuffDyn)->BUFFER_INFO.itemOffset);
                    itDescSetsMapKey++;
                    itStrBuffDyn++;
                } else {
                    assert(false && "No space left in key, or no storage buffer.");
                    exit(EXIT_FAILURE);
                }
            }
        }

        // Get/create the resource offset map for the material (or 0 if no material).
        auto itDescSetsMap = pSet->descriptorSetsMap_.find(descriptorSetsMapKey);
        if (itDescSetsMap == pSet->descriptorSetsMap_.end()) {
            // Add a element with the proper number of sets
            auto it = pSet->descriptorSetsMap_.insert({descriptorSetsMapKey, {}});
            assert(it.second);
            itDescSetsMap = it.first;
        }

        for (const auto& helper : helpers) {
            // Get/create the descriptor sets map element for the resource.
            auto itResOffsetsMap = itDescSetsMap->second.find(helper.resourceOffset);
            if (itResOffsetsMap == itDescSetsMap->second.end()) {
                // Add a element with the proper number of sets
                auto it = itDescSetsMap->second.insert({
                    helper.resourceOffset,
                    {
                        std::make_shared<Set::resourceInfoMap>(),
                        std::vector<vk::DescriptorSet>(pSet->getSetCount()),
                    },
                });
                assert(it.second);
                itResOffsetsMap = it.first;

                // Allocate
                allocateDescriptorSets(std::ref(*helper.pResource), itResOffsetsMap->second.second);

                // Update
                updateDescriptorSets(pSet->getBindingMap(), pSet->getDescriptorOffsets(helper.resourceOffset),
                                     itResOffsetsMap->second, pDynamicItems);
            }
            const auto& [pInfoMap, sets] = itResOffsetsMap->second;

            // Not sure what to do atm if this is not true. Sets probably need to be allocated, and
            // updated. But I am not sure if this will ever happen.
            assert(sets.size() == pSet->getSetCount());

            auto itBindDataMap = bindDataMap.find(helper.passTypes1);
            if (itBindDataMap == bindDataMap.end()) {
                auto insertPair = bindDataMap.insert({
                    helper.passTypes1,
                    {0, std::vector<std::vector<vk::DescriptorSet>>(sets.size())},
                });
                assert(insertPair.second);
                itBindDataMap = insertPair.first;
            }

            if (sets.size() > itBindDataMap->second.descriptorSets.size()) {
                while (sets.size() != itBindDataMap->second.descriptorSets.size())  //
                    itBindDataMap->second.descriptorSets.push_back(itBindDataMap->second.descriptorSets.front());
            } else if (sets.size() != itBindDataMap->second.descriptorSets.size()) {
                // Set needs to be duplicated for existing image count depenedent sets
                assert(itBindDataMap->second.descriptorSets.size() == shell().context().imageCount);
            }

            for (auto i = 0; i < itBindDataMap->second.descriptorSets.size(); i++) {
                itBindDataMap->second.descriptorSets[i].push_back(sets[i < sets.size() ? i : 0]);
            }

            // DYNAMIC OFFSETS
            getDynamicOffsets(pSet, itBindDataMap->second.dynamicOffsets, pDynamicItems);

            // This is for convenience. Might be more of a pain than its worth. I already
            // had to change it to a shared_ptr.
            assert(itBindDataMap->second.setResourceInfoMap.count(pSet->TYPE) == 0);
            itBindDataMap->second.setResourceInfoMap[pSet->TYPE] = pInfoMap;
        }
    }
}

void Descriptor::Handler::allocateDescriptorSets(const Descriptor::Set::Resource& resource,
                                                 std::vector<vk::DescriptorSet>& descriptorSets) {
    std::vector<vk::DescriptorSetLayout> layouts(descriptorSets.size(), resource.layout);

    vk::DescriptorSetAllocateInfo allocInfo = {};
    allocInfo.descriptorPool = pool_;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    helpers::checkVkResult(shell().context().dev.allocateDescriptorSets(&allocInfo, descriptorSets.data()));
}

void Descriptor::Handler::updateDescriptorSets(const Descriptor::bindingMap& bindingMap,
                                               const Descriptor::OffsetsMap& offsets, Set::resourceInfoMapSetsPair& pair,
                                               const std::vector<Descriptor::Base*> pDynamicItems) const {
    std::vector<std::vector<vk::WriteDescriptorSet>> writesList(pair.second.size());
    assert(writesList.size() == pair.second.size());

    auto itDynItm = pDynamicItems.begin();
    auto itCmbSampMat = pDynamicItems.begin();

    for (const auto& [key, bindingInfo] : bindingMap) {
        // Perpare the set resource info map
        auto infoMapKey = std::make_pair(key.first, bindingInfo.descType);
        auto itInfoMap = pair.first->find(infoMapKey);
        if (itInfoMap == pair.first->end()) {
            auto it = pair.first->insert({infoMapKey, {}});
            assert(it.second);
            itInfoMap = it.first;
        } else {
            itInfoMap->second.reset();
        }

        /* Let the set info resource helper struct know how many different data sets are
         *  required, so that everything needed to interpret its contents is known (I don't
         *  like copying this count here because who knows where it can fall out of sync, but
         *  I can't think of anything else atm...).
         */
        itInfoMap->second.uniqueDataSets = bindingInfo.uniqueDataSets;
        assert(itInfoMap->second.uniqueDataSets > 0);

        // Set the basic binding info
        for (auto i = 0; i < writesList.size(); i++) {
            writesList[i].push_back(getWrite({key, bindingInfo}, pair.second[i]));
        }

        if (std::visit(HasOffsets{}, bindingInfo.descType)) {
            // HAS OFFSETS

            assert(offsets.map().count(bindingInfo.descType) == 1);
            auto itOffsetsMap = offsets.map().find(bindingInfo.descType);
            assert(itOffsetsMap->second.size());

            uniformHandler().getWriteInfos(bindingInfo.descType, itOffsetsMap->second, itInfoMap->second);

            assert(static_cast<uint32_t>(itInfoMap->second.bufferInfos.size()) ==
                   (itInfoMap->second.uniqueDataSets * itInfoMap->second.descCount));

        } else if (std::visit(IsDynamic{}, bindingInfo.descType)) {
            // DYNAMIC

            if (itDynItm == pDynamicItems.end() || (*itDynItm)->getDescriptorType() != bindingInfo.descType) {
                if (itDynItm == pDynamicItems.begin()) {
                    /**
                     * This might lead to confusion in the future but I am going to do it now because its just taking up too
                     * much of my time at the moment. Here, I am only going to advance the itDynItm pointer to the first hit
                     * if it is at the beginning of the list. The reason this is necessary is because some draw buffers have
                     * dynamic uniforms in different descriptor sets now. This is essentially trying to find the start of the
                     * list for a specific descriptor set while still trying to enforce the ordering of the descriptors in
                     * the pDynamicItems array. This should force the user to be aware of what they are doing when they
                     * call getBindData, which is the whole point of forcing the ordering, but also allowing this to work
                     * until a better solution is developed.
                     */
                    while (itDynItm != pDynamicItems.end() && (*itDynItm)->getDescriptorType() != bindingInfo.descType)
                        itDynItm++;
                }
                if (itDynItm == pDynamicItems.end()) {
                    assert(false && "Not enough dynamic items, or the items aren't the right type in the right order.");
                    exit(EXIT_FAILURE);
                }
            }

            itInfoMap->second.descCount = 1;
            itInfoMap->second.bufferInfos.resize(itInfoMap->second.uniqueDataSets);
            (*itDynItm)->setDescriptorInfo(itInfoMap->second, 0);

            if (std::visit(IsUniformDynamic{}, (*itDynItm)->getDescriptorType())) {
                auto sMsg =
                    Descriptor::GetPerframeBufferWarning(bindingInfo.descType, (*itDynItm)->BUFFER_INFO, itInfoMap->second);
                if (sMsg.size()) shell().log(Shell::LogPriority::LOG_WARN, sMsg.c_str());
            } else if (std::visit(IsStorageBufferDynamic{}, bindingInfo.descType)) {
                assert(itInfoMap->second.bufferInfos.size() == 1);
            } else {
                assert(false && "Unhandled dynamic uniform type");
                exit(EXIT_FAILURE);
            }
            itDynItm++;

        } else if (std::visit(IsCombinedSamplerMaterial{}, bindingInfo.descType)) {
            // MATERIAL SAMPLER

            itCmbSampMat = std::find_if(itCmbSampMat, pDynamicItems.end(), [](const auto& pItem) {
                return std::visit(IsMaterial{}, pItem->getDescriptorType());
            });
            if (itCmbSampMat == pDynamicItems.end() ||
                static_cast<Material::Base*>(*itCmbSampMat)->getTexture() == nullptr ||
                static_cast<Material::Base*>(*itCmbSampMat)->getTexture()->samplers.size() != 1) {
                assert(false && "No material for the sampler");
                exit(EXIT_FAILURE);
            }
            itInfoMap->second.imageInfos.push_back(
                static_cast<Material::Base*>(*itCmbSampMat)->getTexture()->samplers.at(0).imgInfo);
            itInfoMap->second.descCount = 1;
            itCmbSampMat++;

        } else if (std::visit(IsUniformTexelBuffer{}, bindingInfo.descType)) {
            // UNIFORM TEXEL BUFFER

            auto pPipelineTexelBuffer = textureHandler().getBufferView(bindingInfo.textureId);
            if (pPipelineTexelBuffer == nullptr) {
                assert(false && "Could not find uniform texel buffer");
                exit(EXIT_FAILURE);
            }
            itInfoMap->second.texelBufferView = pPipelineTexelBuffer->view;
            itInfoMap->second.descCount = 1;

        } else if (std::visit(IsPipelineImage{}, bindingInfo.descType) ||
                   std::visit(IsSwapchainStorageImage{}, bindingInfo.descType)) {
            // PIPELINE IMAGE/SAMPLER, SWAPCHAIN

            auto pPipelineTexture = textureHandler().getTexture(bindingInfo.textureId);
            if (pPipelineTexture != nullptr) {
                // Single texture required
                assert(itInfoMap->second.uniqueDataSets == 1);
                // TODO: Maybe treat this scenario like above.
                assert(pPipelineTexture->samplers.size() == 1);
                itInfoMap->second.imageInfos.push_back(pPipelineTexture->samplers[0].imgInfo);

            } else {
                assert(itInfoMap->second.uniqueDataSets > 1);
                for (uint32_t i = 0; i < itInfoMap->second.uniqueDataSets; i++) {
                    // Check for per frame texture id suffix. This is for per swap
                    pPipelineTexture = textureHandler().getTexture(bindingInfo.textureId, i);
                    assert(pPipelineTexture != nullptr);
                    // Never tested/thought about otherwise
                    assert(pPipelineTexture->samplers.size() == 1);
                    itInfoMap->second.imageInfos.push_back(pPipelineTexture->samplers[0].imgInfo);
                }
                assert(itInfoMap->second.imageInfos.size() <= writesList.size());
            }
            itInfoMap->second.descCount = 1;

            assert(itInfoMap->second.uniqueDataSets == itInfoMap->second.imageInfos.size());

        } else {
            assert(false && "Unhandled descriptor type");
            exit(EXIT_FAILURE);
        }

        // Make sure descCount was set.
        assert(itInfoMap->second.descCount);

        for (uint32_t i = 0; i < writesList.size(); i++) {
            auto& writes = writesList[i];
            itInfoMap->second.setWriteInfo(i, writes.back());
        }
    }

    for (auto& writes : writesList) {
        shell().context().dev.updateDescriptorSets(writes, {});
    }
}

void Descriptor::Handler::updateBindData(const std::vector<std::string> textureIds) {
    // Find the descriptor set(s)
    for (const auto& pSet : pDescriptorSets_) {
        // At the moment I think that this is fine. I am not sure though. The reason this can be empty is because
        // the descriptor set is created and then not used in an active pass type that is swapchain dependent.
        if (pSet->descriptorSetsMap_.empty()) continue;

        for (const auto& [key, bindingInfo] : pSet->getBindingMap()) {
            if (bindingInfo.textureId.size()) {
                for (const auto& textureId : textureIds) {
                    assert(textureId.size());
                    // Because of the regex check for per framebuffer textures in the the texture
                    // handler there can be a straight equivalence check here.
                    if (textureId == bindingInfo.textureId) {
                        // Not trying to make this work for anything else atm.
                        bool isValidType = std::visit(IsPipelineImage{}, bindingInfo.descType);
                        isValidType |= std::visit(IsSwapchainStorageImage{}, bindingInfo.descType);
                        assert(isValidType);
                        assert(pSet->descriptorSetsMap_.size() == 1);
                        for (auto& [resourceOffset, descriptorSets] : pSet->descriptorSetsMap_.at({0, 0, 0, 0})) {
                            updateDescriptorSets(pSet->getBindingMap(), pSet->getDescriptorOffsets(resourceOffset),
                                                 descriptorSets, {});
                        }
                    }
                }
            }
        }
    }
}

vk::WriteDescriptorSet Descriptor::Handler::getWrite(const Descriptor::bindingMapKeyValue& keyValue,
                                                     const vk::DescriptorSet& set) const {
    vk::WriteDescriptorSet write = {};
    write.dstBinding = keyValue.first.first;
    write.dstArrayElement = keyValue.first.second;
    write.descriptorType = std::visit(GetVulkanDescriptorType{}, keyValue.second.descType);
    write.dstSet = set;
    return write;
}

// TODO: This work can probably be done in the updateDescriptorSets loop.
void Descriptor::Handler::getDynamicOffsets(const std::unique_ptr<Descriptor::Set::Base>& pSet,
                                            std::vector<uint32_t>& dynamicOffsets,
                                            const std::vector<Descriptor::Base*> pDynamicItems) {
    auto itDynItm = pDynamicItems.begin();
    for (auto& [key, bindingInfo] : pSet->getBindingMap()) {
        if (std::visit(IsDynamic{}, bindingInfo.descType)) {
            if (itDynItm == pDynamicItems.end() || bindingInfo.descType != (*itDynItm)->getDescriptorType()) {
                if (itDynItm == pDynamicItems.begin()) {
                    /**
                     *  IMPORTANT: See the comment in updateDescriptorSets!
                     */
                    while (itDynItm != pDynamicItems.end() && (*itDynItm)->getDescriptorType() != bindingInfo.descType)
                        itDynItm++;
                }
                if (itDynItm == pDynamicItems.end()) {
                    assert(false && "Not enough dynamic items, or the items aren't the right type in the right order.");
                    exit(EXIT_FAILURE);
                }
            }
            dynamicOffsets.push_back(static_cast<uint32_t>((*itDynItm)->BUFFER_INFO.memoryOffset));
            itDynItm++;
        }
    }
}

void Descriptor::Handler::reset() {
    const auto& ctx = shell().context();
    // POOL
    if (pool_) ctx.dev.destroyDescriptorPool(pool_, ctx.pAllocator);
    // LAYOUT
    for (const auto& pSet : pDescriptorSets_) {
        for (auto& res : pSet->resources_) {
            if (res.layout) ctx.dev.destroyDescriptorSetLayout(res.layout, ctx.pAllocator);
        }
    }
}
