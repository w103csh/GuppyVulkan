

#include "DescriptorSet.h"

#include "Texture.h"

namespace {
Uniform::offsetsMapValue getOffsetsValueMapSansDefault(Uniform::offsetsMapValue map) {
    // Uniform::offsetsMapValue newMap = map;
    for (auto it = map.begin(); it != map.end();) {
        if (it->second == Uniform::RENDER_PASS_ALL_SET)
            it = map.erase(it);
        else
            ++it;
    };
    return map;
}
}  // namespace

Descriptor::Set::Base::Base(const DESCRIPTOR_SET&& type, const std::string&& macroName,
                            const Descriptor::bindingMap&& bindingMap)
    : TYPE(type),             //
      MACRO_NAME(macroName),  //
      BINDING_MAP(bindingMap),
      resources_(1),
      defaultResourceOffset_(0) {
    // Create the default resource
    assert(resources_.size() == 1);
    for (const auto& keyValue : BINDING_MAP) getDefaultResource().offsets.insert(std::get<0>(keyValue.second), {});
    getDefaultResource().passTypes = Uniform::RENDER_PASS_ALL_SET;
}

const Descriptor::OffsetsMap Descriptor::Set::Base::getDescriptorOffsets(const resourceTuple& tuple) const {
    auto descriptorOffsets = resources_[defaultResourceOffset_].offsets;
    // If the resource in the helper tuple is not default replace with the overriden offsets.
    if (std::get<2>(tuple) != defaultResourceOffset_) {
        for (const auto& [descType, offsets] : std::get<1>(tuple)->offsets.map()) {
            descriptorOffsets.insert(descType, offsets);
        }
    }
    return descriptorOffsets;
}

void Descriptor::Set::Base::updateOffsets(const Uniform::offsetsMap offsetsMap,
                                          const Descriptor::bindingMapKeyValue& bindingMapKeyValue,
                                          const PIPELINE& pipelineType) {
    // Check for a pipeline specific offset
    auto searchOffsetsMap = offsetsMap.find({std::get<0>(bindingMapKeyValue.second), pipelineType});
    if (searchOffsetsMap != offsetsMap.end()) {
        uniformOffsets_[searchOffsetsMap->first] = searchOffsetsMap->second;
    }

    bool foundPassAll = false;
    // Check for render pass specific offsets.
    searchOffsetsMap = offsetsMap.find({std::get<0>(bindingMapKeyValue.second), PIPELINE::ALL_ENUM});
    assert(searchOffsetsMap != offsetsMap.end());
    for (const auto& [offsets, passTypes] : searchOffsetsMap->second) {
        if (passTypes.find(RENDER_PASS::ALL_ENUM) != passTypes.end()) {
            // Make sure the default is the proper set. The default is added immediately, so
            // no need to re-add it to the layout resource.
            assert(passTypes == Uniform::RENDER_PASS_ALL_SET);

            // A unique element for the UNIFORM descriptor type should already exist.
            assert(getDefaultResource().offsets.map().count(std::get<0>(bindingMapKeyValue.second)) == 1);

            auto it = getDefaultResource().offsets.map().find(std::get<0>(bindingMapKeyValue.second));
            if (it->second.empty()) {
                // Add the default offsets from the offset manager.
                getDefaultResource().offsets.insert(std::get<0>(bindingMapKeyValue.second), offsets);
            } else {
                // Make sure there are not bad duplicates in the offsets manager.
                assert(it->second == offsets);
            }

            foundPassAll = true;
        } else {
            auto searchSetMap = uniformOffsets_.find(searchOffsetsMap->first);
            if (searchSetMap == uniformOffsets_.end()) {
                uniformOffsets_[searchOffsetsMap->first] = getOffsetsValueMapSansDefault(searchOffsetsMap->second);
            } else {
                // Triple check that everything is the same
                assert(searchSetMap->second == getOffsetsValueMapSansDefault(searchOffsetsMap->second));
            }
        }
    }

    // Make sure default offsets are found.
    assert(foundPassAll);
}

void Descriptor::Set::Base::findResourceForPipeline(std::vector<Resource>::iterator& it, const PIPELINE& type) {
    while (it != resources_.end()) {
        if (it->pipelineTypes.count(type) > 0) return;
        ++it;
    }
}

void Descriptor::Set::Base::findResourceForDefaultPipeline(std::vector<Resource>::iterator& it, const PIPELINE& type) {
    while (it != resources_.end()) {
        if (it->pipelineTypes.count(type) > 0 && it->passTypes == Uniform::RENDER_PASS_ALL_SET) return;
        ++it;
    }
}
void Descriptor::Set::Base::findResourceSimilar(std::vector<Resource>::iterator& it, const PIPELINE& piplineType,
                                                const std::set<RENDER_PASS>& passTypes, const DESCRIPTOR& descType,
                                                const Uniform::offsets& offsets) {
    std::set<RENDER_PASS> tempSet;
    for (; it != resources_.end(); ++it) {
        tempSet.clear();
        if (it->offsets.map().size() != 1) continue;
        if (it->pipelineTypes.count(piplineType)) continue;
        if (it->pipelineTypes.count(PIPELINE::ALL_ENUM) == 0) continue;
        std::set_difference(passTypes.begin(), passTypes.end(), it->passTypes.begin(), it->passTypes.end(),
                            std::inserter(tempSet, tempSet.begin()));
        if (tempSet.size() >= passTypes.size()) continue;
        auto search = it->offsets.map().find(descType);
        if (search == it->offsets.map().end()) continue;
        if (search->second != offsets) continue;
    }
}

Descriptor::Set::Default::Uniform::Uniform()
    : Set::Base{
          DESCRIPTOR_SET::UNIFORM_DEFAULT,
          "_DS_UNI_DEF",
          {
              {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, ""}},
              {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_DEFAULT, ""}},
              {{2, 0}, {UNIFORM::FOG_DEFAULT, ""}},
              {{3, 0}, {UNIFORM::LIGHT_POSITIONAL_DEFAULT, ""}},
              {{4, 0}, {UNIFORM::LIGHT_SPOT_DEFAULT, ""}},
          },
      } {}

Descriptor::Set::Default::Sampler::Sampler()
    : Set::Base{
          DESCRIPTOR_SET::SAMPLER_DEFAULT,
          "_DS_SMP_DEF",
          {
              {{0, 0}, {COMBINED_SAMPLER::MATERIAL, ""}},
              //{{1, 0}, {DESCRIPTOR::SAMPLER_MATERIAL_COMBINED, ""}},
              //{{2, 0}, {DESCRIPTOR::SAMPLER_MATERIAL_COMBINED, ""}},
              //{{3, 0}, {DESCRIPTOR::SAMPLER_MATERIAL_COMBINED, ""}},
          },
      } {}

Descriptor::Set::Default::CubeSampler::CubeSampler()
    : Set::Base{
          DESCRIPTOR_SET::SAMPLER_CUBE_DEFAULT,
          "_DS_CBE_DEF",
          {
              {{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::SKYBOX_CREATE_INFO.name}},
          },
      } {}

Descriptor::Set::Default::ProjectorSampler::ProjectorSampler()
    : Set::Base{
          DESCRIPTOR_SET::PROJECTOR_DEFAULT,
          "_DS_PRJ_DEF",
          {
              //{{0, 0}, {DESCRIPTOR::SAMPLER_PIPELINE_COMBINED, Texture::STATUE_CREATE_INFO.name}},
              {{0, 0}, {COMBINED_SAMPLER::PIPELINE, RenderPass::TEXTURE_2D_CREATE_INFO.name}},
              {{1, 0}, {UNIFORM::PROJECTOR_DEFAULT, ""}},
          },
      } {}
