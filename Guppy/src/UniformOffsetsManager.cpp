
#include "UniformOffsetsManager.h"

#include <variant>

namespace Uniform {

const offsets DEFAULT_SINGLE_SET = {0};
const offsets DEFAULT_ALL_SET = {UINT32_MAX};
const std::set<PASS> PASS_ALL_SET = {PASS::ALL_ENUM};

// DEFAULT MAP
const std::map<DESCRIPTOR, offsets> DEFAULT_OFFSETS_MAP = {
    // UNIFORM
    // CAMERA
    {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, DEFAULT_SINGLE_SET},
    // LIGHT
    {UNIFORM::LIGHT_POSITIONAL_DEFAULT, DEFAULT_ALL_SET},
    {UNIFORM::LIGHT_SPOT_DEFAULT, DEFAULT_ALL_SET},
    {UNIFORM::LIGHT_POSITIONAL_PBR, DEFAULT_ALL_SET},
    // MISC
    {UNIFORM::FOG_DEFAULT, DEFAULT_SINGLE_SET},
    {UNIFORM::PROJECTOR_DEFAULT, DEFAULT_SINGLE_SET},
    {UNIFORM::SCREEN_SPACE_DEFAULT, DEFAULT_SINGLE_SET},
    // STORAGE
    // MISC
    {STORAGE_BUFFER::POST_PROCESS, DEFAULT_SINGLE_SET},
};

}  // namespace Uniform

Uniform::OffsetsManager::OffsetsManager() : offsetsMap_() { initializeMap(); }

void Uniform::OffsetsManager::initializeMap() {
    for (const auto& [descType, offsets] : DEFAULT_OFFSETS_MAP) {
        // validate
        assert(std::visit(Descriptor::HasOffsets{}, descType));
        assert(offsets.size());

        offsetsMapKey& key = std::make_pair(descType, PIPELINE::ALL_ENUM);
        assert(offsetsMap_.count(key) == 0);
        offsetsMap_.insert({std::move(key), {{offsets, PASS_ALL_SET}}});
    }
}

void Uniform::OffsetsManager::validateAddType(const offsetsMap& map, const ADD_TYPE& addType) {
    // Make sure the incoming map has valid elements.
    for (const auto& keyValue1 : map) {
        const auto& descPplnPair = keyValue1.first;
        const auto& offsetPassMap = keyValue1.second;
        switch (addType) {
            case ADD_TYPE::Pipeline:
                assert(descPplnPair.second != PIPELINE::ALL_ENUM);
                break;
            case ADD_TYPE::RenderPass:
                break;
        }
        for (const auto& keyValue2 : offsetPassMap) {
            const auto& offsets = keyValue2.first;
            const auto& passTypes = keyValue2.second;
            switch (addType) {
                case ADD_TYPE::Pipeline:
                    assert(passTypes == Uniform::PASS_ALL_SET);
                    break;
                case ADD_TYPE::RenderPass:
                    assert(passTypes != Uniform::PASS_ALL_SET);
                    break;
            }
        }
    }
}

void Uniform::OffsetsManager::addOffsets(const offsetsMap& map, const ADD_TYPE&& addType) {
    validateAddType(map, addType);

    // Merge the incoming map and validate it.
    for (const auto& descriptorPipelineKeyValue : map) {
        auto& key = descriptorPipelineKeyValue.first;
        auto& offsetsPassTypesMap = descriptorPipelineKeyValue.second;

        for (const auto& offsetsPassTypesKeyValue : offsetsPassTypesMap) {
            auto& offsets = offsetsPassTypesKeyValue.first;
            auto& passTypes = offsetsPassTypesKeyValue.second;

            auto searchMap = offsetsMap_.find(key);
            if (searchMap != offsetsMap_.end()) {
                // Key already exists so make sure offsets are unique before adding a
                // a new pass type.
                // NOT SURE ABOUT THESE CHECKS!!!!!!!!!!!!!!!!
                if (offsets == DEFAULT_SINGLE_SET) {
                    auto searchValue = searchMap->second.find(DEFAULT_SINGLE_SET);
                    if (searchValue != searchMap->second.end() && searchValue->second == PASS_ALL_SET)
                        assert(false && "Trying to add a new pass type when its the same as the default");
                    else
                        searchMap->second[offsets].insert(passTypes.begin(), passTypes.end());

                } else if (offsets == DEFAULT_ALL_SET) {
                    auto searchValue = searchMap->second.find(DEFAULT_ALL_SET);
                    if (searchValue != searchMap->second.end() && searchValue->second == PASS_ALL_SET)
                        assert(false && "Trying to add a new pass type when its the same as the default");
                    else
                        searchMap->second[offsets].insert(passTypes.begin(), passTypes.end());
                } else {
                    searchMap->second[offsets].insert(passTypes.begin(), passTypes.end());
                }
            } else {
                offsetsMap_[key][offsets].insert(passTypes.begin(), passTypes.end());
            }
        }
    }
}
