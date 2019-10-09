

#include "DescriptorSet.h"

#include <variant>

#include "Texture.h"
// HANDLERS
#include "DescriptorHandler.h"
#include "TextureHandler.h"

namespace {
Uniform::offsetsMapValue getOffsetsValueMapSansDefault(Uniform::offsetsMapValue map) {
    // Uniform::offsetsMapValue newMap = map;
    for (auto it = map.begin(); it != map.end();) {
        if (it->second == Uniform::PASS_ALL_SET)
            it = map.erase(it);
        else
            ++it;
    };
    return map;
}
}  // namespace

Descriptor::Set::Base::Base(Handler& handler, const CreateInfo* pCreateInfo)
    : Handlee(handler),
      TYPE(pCreateInfo->type),
      MACRO_NAME(pCreateInfo->macroName),
      bindingMap_(pCreateInfo->bindingMap),
      setCount_(0),
      defaultResourceOffset_(0),
      resources_(1) {
    // Create the default resource
    assert(resources_.size() == 1);
    for (const auto& [key, bindingInfo] : getBindingMap()) getDefaultResource().offsets.insert(bindingInfo.descType, {});

    getDefaultResource().passTypes = Uniform::PASS_ALL_SET;
}

const Descriptor::OffsetsMap Descriptor::Set::Base::getDescriptorOffsets(const uint32_t& offset) const {
    auto descriptorOffsets = resources_[defaultResourceOffset_].offsets;
    // If the resource in the helper tuple is not default replace with the overriden offsets.
    if (offset != defaultResourceOffset_) {
        for (const auto& [descType, offsets] : resources_[offset].offsets.map()) {
            descriptorOffsets.insert(descType, offsets);
        }
    }
    return descriptorOffsets;
}

void Descriptor::Set::Base::update(const uint32_t imageCount) {
    for (auto& [key, bindingInfo] : bindingMap_) {
        if (std::visit(Descriptor::IsImage{}, bindingInfo.descType)) {
            // This is really convoluted, but I don't care atm. Only bindings with texture ids
            // specified that have a per framebuffer suffix can have more than one.
            const auto& pTexture = handler().textureHandler().getTexture(bindingInfo.textureId, 0);
            if (pTexture != nullptr) {
                // This is so bad it hurts.
                assert(pTexture->PER_FRAMEBUFFER);
                bindingInfo.uniqueDataSets = imageCount;
            }
        } else if (std::visit(Descriptor::HassPerFramebufferData{}, bindingInfo.descType)) {
            bindingInfo.uniqueDataSets = imageCount;
        }
        // Update the number of sets needed based on the highest number of unique
        // descriptors required.
        setCount_ = (std::max)(setCount_, static_cast<uint8_t>(bindingInfo.uniqueDataSets));
    }
    assert(setCount_ > 0);
}

void Descriptor::Set::Base::updateOffsets(const Uniform::offsetsMap offsetsMap, const DESCRIPTOR& descType,
                                          const PIPELINE& pipelineType) {
    // Check for a pipeline specific offset
    auto searchOffsetsMap = offsetsMap.find({descType, pipelineType});
    if (searchOffsetsMap != offsetsMap.end()) {
        uniformOffsets_[searchOffsetsMap->first] = searchOffsetsMap->second;
    }

    bool foundPassAll = false;
    // Check for render pass specific offsets.
    searchOffsetsMap = offsetsMap.find({descType, PIPELINE::ALL_ENUM});
    assert(searchOffsetsMap != offsetsMap.end());
    for (const auto& [offsets, passTypes] : searchOffsetsMap->second) {
        if (passTypes.find(PASS::ALL_ENUM) != passTypes.end()) {
            // Make sure the default is the proper set. The default is added immediately, so
            // no need to re-add it to the layout resource.
            assert(passTypes == Uniform::PASS_ALL_SET);

            // A unique element for the UNIFORM descriptor type should already exist.
            assert(getDefaultResource().offsets.map().count(descType) == 1);

            auto it = getDefaultResource().offsets.map().find(descType);
            if (it->second.empty()) {
                // Add the default offsets from the offset manager.
                getDefaultResource().offsets.insert(descType, offsets);
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

bool Descriptor::Set::Base::hasTextureMaterial() const {
    for (const auto& [key, bindingInfo] : getBindingMap()) {
        if (std::visit(IsCombinedSamplerMaterial{}, bindingInfo.descType)) return true;
    }
    return false;
}

void Descriptor::Set::Base::findResourceForPipeline(std::vector<Resource>::iterator& it, const PIPELINE& type) {
    while (it != resources_.end()) {
        if (it->pipelineTypes.count(type) > 0) return;
        ++it;
    }
}

void Descriptor::Set::Base::findResourceForDefaultPipeline(std::vector<Resource>::iterator& it, const PIPELINE& type) {
    while (it != resources_.end()) {
        if (it->pipelineTypes.count(type) > 0 && it->passTypes == Uniform::PASS_ALL_SET) return;
        ++it;
    }
}
void Descriptor::Set::Base::findResourceSimilar(std::vector<Resource>::iterator& it, const PIPELINE& piplineType,
                                                const std::set<PASS>& passTypes, const DESCRIPTOR& descType,
                                                const Uniform::offsets& offsets) {
    std::set<PASS> tempSet;
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
