
#include "UniformOffsetsManager.h"

namespace Uniform {

const offsets OFFSET_SINGLE_SET = {0};
const offsets OFFSET_ALL_SET = {UINT32_MAX};
const offsets OFFSET_DONT_CARE = {};

const std::set<RENDER_PASS> RENDER_PASS_ALL_SET = {RENDER_PASS::ALL_ENUM};
const offsetsMapValue OFFSET_SINGLE_DEFAULT_MAP = {{OFFSET_SINGLE_SET, RENDER_PASS_ALL_SET}};
const offsetsMapValue OFFSET_ALL_DEFAULT_MAP = {{OFFSET_ALL_SET, RENDER_PASS_ALL_SET}};

// DEFAULT MAP
const offsetsMap DEFAULT_OFFSETS_MAP = {
    // CAMERA
    {
        {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, PIPELINE::ALL_ENUM},
        OFFSET_SINGLE_DEFAULT_MAP,
    },
    // THIS WILL OVERRIDE THE PREVIOUS. IT NEEDS TO BE MERGED IN BY USING THE RENDER PASS ADD METHOD. !!!!!!!
    //{
    //    {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, PIPELINE::ALL_ENUM},
    //    {{{3},
    //      {RENDER_PASS::IMGUI,
    //       RENDER_PASS::SAMPLER}}},  // GOOD TEST FOR COMPATIBILITY WOULD BE TO SET THIS TO 2 AND LOOK AT COMMENT BELOW
    //},
    // LIGHT
    {
        {UNIFORM::LIGHT_POSITIONAL_DEFAULT, PIPELINE::ALL_ENUM},
        OFFSET_ALL_DEFAULT_MAP,
    },
    {
        {UNIFORM::LIGHT_SPOT_DEFAULT, PIPELINE::ALL_ENUM},
        OFFSET_ALL_DEFAULT_MAP,
    },
    {
        {UNIFORM::LIGHT_POSITIONAL_PBR, PIPELINE::ALL_ENUM},
        OFFSET_ALL_DEFAULT_MAP,
    },
    // GENERAL
    // MOVE THIS TYPE TO A PIPELINE ADD METHOD (3 of them) !!!!
    //{
    //    {UNIFORM::FOG_DEFAULT, PIPELINE::TRI_LIST_COLOR},
    //    {{{1}, RENDER_PASS_ALL_SET}},
    //},
    //{
    //    {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, PIPELINE::TRI_LIST_COLOR},
    //    {{{2}, RENDER_PASS_ALL_SET}},  // GOOD TEST FOR COMPATIBILITY WITH ABOVE. IF BOTH OFFSETS ARE 2,
    //    // AND CAMERA_DEFAULT_PERSPECTIVE(2) IS ONLY USED IN THE SAME DESCRIPTOR SET BY TRI_LIST_COLOR PIPELINE
    //    // AND SAMPLER PASS THEN THERE IS NO NEED TO CREATE ANOTHER SET LAYOUT. ITS ONLY IS NECESSARY IF SAMPLER
    //    // PASS USES ANY OTHER PIPELINE WITH SAME DESCRIPTOR SET THAT CONTAINS CAMERA_DEFAULT_PERSPECTIVE AS
    //    // TRI_LIST_COLOR WITH A DIFFERENT OFFSET. GOOD LORD THAT DIFFICULT TO CONVEY IN WRITING.
    //},
    //{
    //    {UNIFORM::FOG_DEFAULT, PIPELINE::LINE},
    //    {{{1}, RENDER_PASS_ALL_SET}},  // MAYBE ADD SOME KIND OF VALIDATION TO SHOW THAT THIS IS STUPID AND IS
    //    // COVERED BY THE OTHER TRI_LIST_COLOR ONE
    //},
    {
        {UNIFORM::FOG_DEFAULT, PIPELINE::ALL_ENUM},
        OFFSET_SINGLE_DEFAULT_MAP,
    },
    {
        {UNIFORM::PROJECTOR_DEFAULT, PIPELINE::ALL_ENUM},
        OFFSET_SINGLE_DEFAULT_MAP,
    },
    {
        {UNIFORM::SCREEN_SPACE_DEFAULT, PIPELINE::ALL_ENUM},
        OFFSET_SINGLE_DEFAULT_MAP,
    },
};

}  // namespace Uniform

Uniform::OffsetsManager::OffsetsManager()  //
    : offsetsMap_(DEFAULT_OFFSETS_MAP) {
    validateInitialMap();
}

void Uniform::OffsetsManager::validateInitialMap() {
    /* Add any general validation for the map here. At this point I am just going to
     *  check for offsets set not being empty and ALL_ENUMs being used. You could
     *  potentially check for other things like is the descriptor type unique or is
     *  it in a list of all the active descriptors, so that you could be warned about
     *  ithat it will not work right.
     */
    for (const auto& keyValue1 : offsetsMap_) {
        const auto& descPplnPair = keyValue1.first;
        const auto& offsetPassMap = keyValue1.second;
        assert(descPplnPair.second == PIPELINE::ALL_ENUM);
        for (const auto& keyValue2 : offsetPassMap) {
            const auto& offsets = keyValue2.first;
            const auto& passTypes = keyValue2.second;
            assert(offsets.size());
            assert(passTypes == Uniform::RENDER_PASS_ALL_SET);
        }
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
                    assert(passTypes == Uniform::RENDER_PASS_ALL_SET);
                    break;
                case ADD_TYPE::RenderPass:
                    assert(passTypes != Uniform::RENDER_PASS_ALL_SET);
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
                if (offsets == OFFSET_SINGLE_SET) {
                    auto searchValue = searchMap->second.find(OFFSET_SINGLE_SET);
                    if (searchValue != searchMap->second.end() && searchValue->second == RENDER_PASS_ALL_SET)
                        assert(false && "Trying to add a new pass type when its the same as the default");
                    else
                        searchMap->second[offsets].insert(passTypes.begin(), passTypes.end());

                } else if (offsets == OFFSET_ALL_SET) {
                    auto searchValue = searchMap->second.find(OFFSET_ALL_SET);
                    if (searchValue != searchMap->second.end() && searchValue->second == RENDER_PASS_ALL_SET)
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
