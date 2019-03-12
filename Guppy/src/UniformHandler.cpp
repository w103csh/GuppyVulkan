
#include <regex>
#include <sstream>

#include "UniformHandler.h"

#include "InputHandler.h"
#include "MeshHandler.h"
#include "Shell.h"

namespace {

const std::string MACRO_KEY = "MACRO_NAME";
const std::string MACRO_REGEX_TEMPLATE = "(#define)\\s+(" + MACRO_KEY + ")\\s+(\\d+)";
const uint8_t MACRO_REGEX_GROUP = 3;

//// TODO: what should this be???
// struct _UniformTag {
//    const char name[17] = "ubo tag";
//} tag;

}  // namespace

Uniform::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),
      // CAMERA
      camDefPersMgr_{"Default Perspective Camera", DESCRIPTOR::CAMERA_PERSPECTIVE_DEFAULT, 5, "CAM_DEF_PERS"},
      // LIGHT
      lgtDefPosMgr_{"Default Positional Light", DESCRIPTOR::LIGHT_POSITIONAL_DEFAULT, 20, "LGT_DEF_POS"},
      lgtPbrPosMgr_{"PBR Positional Light", DESCRIPTOR::LIGHT_POSITIONAL_PBR, 20, "LGT_PBR_POS"},
      lgtDefSptMgr_{"Default Spot Light", DESCRIPTOR::LIGHT_SPOT_DEFAULT, 20, "LGT_DEF_SPT"},
      // MISCELLANEOUS
      uniDefFogMgr_{"Default Fog", DESCRIPTOR::FOG_DEFAULT, 5, "UNI_DEF_FOG"},
      // GENERAL
      hasVisualHelpers(false) {}

void Uniform::Handler::init() {
    reset();

    camDefPersMgr_.init(shell().context(), settings());
    lgtDefPosMgr_.init(shell().context(), settings());
    lgtPbrPosMgr_.init(shell().context(), settings());
    lgtDefSptMgr_.init(shell().context(), settings());
    uniDefFogMgr_.init(shell().context(), settings());

    createCameras();
    createLights();
    createMiscellaneous();
}

void Uniform::Handler::reset() {
    const auto& dev = shell().context().dev;
    // CAMERAS
    camDefPersMgr_.destroy(dev);
    // LIGHTS
    lgtDefPosMgr_.destroy(dev);
    lgtPbrPosMgr_.destroy(dev);
    lgtDefSptMgr_.destroy(dev);
    // MISCELLANEOUS
    uniDefFogMgr_.destroy(dev);
}

void Uniform::Handler::createCameras() {
    const auto& dev = shell().context().dev;
    Camera::Default::Perspective::CreateInfo createInfo = {};

    createInfo.aspect = static_cast<float>(settings().initial_width) / static_cast<float>(settings().initial_height);
    camDefPersMgr_.insert(dev, &createInfo);
    camDefPersMgr_.insert(dev, &createInfo);

    assert(MAIN_CAMERA_OFFSET < camDefPersMgr_.pItems.size());
}

void Uniform::Handler::createLights() {
    const auto& dev = shell().context().dev;
    Light::CreateInfo createInfo = {};

    // POSITIONAL DEFAULT
    createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(20.5f, 10.5f, -23.5f));
    lgtDefPosMgr_.insert(dev, &createInfo);
    createInfo.model = helpers::affine(glm::vec3(1.0f), {-2.5f, 4.5f, -1.5f});
    lgtDefPosMgr_.insert(dev, &createInfo);
    createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(-20.0f, 5.0f, -6.0f));
    lgtDefPosMgr_.insert(dev, &createInfo);

    // POSITIONAL PBR (TODO: these being seperately created is really dumb!!! If this is
    // necessary there should be one set of positional lights, and multiple data buffers
    // for the one set...)
    createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(10.5f, 17.5f, -23.5f));
    // lgtPbrPosMgr_.insert(dev, &createInfo);
    createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(22.5f, 5.5f, -7.5f));
    // lgtPbrPosMgr_.insert(dev, &createInfo);

    // SPOT DEFAULT
    Light::Default::Spot::CreateInfo spotCreateInfo = {};
    spotCreateInfo.exponent = glm::radians(25.0f);
    spotCreateInfo.exponent = 25.0f;
    spotCreateInfo.model = helpers::viewToWorld({0.0f, 4.5f, 1.0f}, {0.0f, 0.0f, -1.5f}, UP_VECTOR);
    lgtDefSptMgr_.insert(dev, &spotCreateInfo);
}

void Uniform::Handler::createMiscellaneous() {
    const auto& dev = shell().context().dev;
    // FOG
    uniDefFogMgr_.insert(dev);
}

void Uniform::Handler::createVisualHelpers() {
    // DEFAULT POSITIONAL
    for (auto& pItem : lgtDefPosMgr_.pItems) {
        auto& lgt = lgtDefPosMgr_.getTypedItem<Light::Default::Positional::Base>(pItem);
        meshHandler().makeModelSpaceVisualHelper(lgt);
        hasVisualHelpers = true;
    }
    // PBR POSITIONAL
    for (auto& pItem : lgtPbrPosMgr_.pItems) {
        auto& lgt = lgtPbrPosMgr_.getTypedItem<Light::PBR::Positional::Base>(pItem);
        meshHandler().makeModelSpaceVisualHelper(lgt);
        hasVisualHelpers = true;
    }
    // DEFAULT SPOT
    for (auto& pItem : lgtDefSptMgr_.pItems) {
        auto& lgt = lgtDefSptMgr_.getTypedItem<Light::Default::Spot::Base>(pItem);
        meshHandler().makeModelSpaceVisualHelper(lgt);
        hasVisualHelpers = true;
    }
    // for (auto& light : posLights) {
    //    meshCreateInfo = {};
    //    meshCreateInfo.isIndexed = false;
    //    meshCreateInfo.model = light.getModel();
    //    std::unique_ptr<LineMesh> pHelper = std::make_unique<VisualHelper>(&meshCreateInfo, 0.5f);
    //    SceneHandler::getActiveScene()->moveMesh(settings(), shell().context(), std::move(pHelper));
    //}
    // for (auto& light : spotLights) {
    //    meshCreateInfo = {};
    //    meshCreateInfo.isIndexed = false;
    //    meshCreateInfo.model = light.getModel();
    //    std::unique_ptr<LineMesh> pHelper = std::make_unique<VisualHelper>(&meshCreateInfo, 0.5f);
    //    SceneHandler::getActiveScene()->moveMesh(settings(), shell().context(), std::move(pHelper));
    //}
}

void Uniform::Handler::update() {
    // MAIN CAMERA
    auto& camera = getMainCamera();
    camera.update(InputHandler::getPosDir(), InputHandler::getLookDir());
    update(camera);

    // DEFAULT POSITIONAL
    for (auto& pItem : lgtDefPosMgr_.pItems) {
        auto& lgt = lgtDefPosMgr_.getTypedItem<Light::Default::Positional::Base>(pItem);
        lgt.update(camera.getCameraSpacePosition(lgt.getPosition()));
        update(lgt);
    }
    // lgtDefPosMgr_.update(shell().context().dev);
    // PBR POSITIONAL
    for (auto& pItem : lgtPbrPosMgr_.pItems) {
        auto& lgt = lgtPbrPosMgr_.getTypedItem<Light::PBR::Positional::Base>(pItem);
        lgt.update(camera.getCameraSpacePosition(lgt.getPosition()));
        update(lgt);
    }
    // lgtDefPosMgr_.update(shell().context().dev);
    // DEFAULT SPOT
    for (auto& pItem : lgtDefSptMgr_.pItems) {
        auto& lgt = lgtDefSptMgr_.getTypedItem<Light::Default::Spot::Base>(pItem);
        lgt.update(camera.getCameraSpaceDirection(lgt.getDirection()), camera.getCameraSpacePosition(lgt.getPosition()));
        update(lgt);
    }
    // lgtDefPosMgr_.update(shell().context().dev);
}

uint32_t Uniform::Handler::getDescriptorCount(const Descriptor::bindingMapValue& value) {
    const auto& min = *value.second.begin();
    const auto& max = *value.second.rbegin();
    if (value.second.size() == 1 && min == Descriptor::Set::OFFSET_ALL) {
        return static_cast<uint32_t>(getUniforms(value.first).size());
    }
    assert(min >= 0 && max < getUniforms(value.first).size());
    return static_cast<uint32_t>(value.second.size());
}

std::set<uint32_t> Uniform::Handler::getBindingOffsets(const Descriptor::bindingMapValue& value) {
    if (value.second.size()) {
        if (*value.second.begin() == Descriptor::Set::OFFSET_SINGLE) return {0};
        if (*value.second.begin() == Descriptor::Set::OFFSET_ALL) {
            std::set<uint32_t> offsets;
            for (int i = 0; i < getUniforms(value.first).size(); i++) offsets.insert(i);
            return offsets;
        }
    }
    return value.second;
}

bool Uniform::Handler::validatePipelineLayout(const Descriptor::bindingMap& map) {
    std::map<DESCRIPTOR, UNIFORM_INDEX> validationMap;
    // Find the highest offset value for each descriptor type.
    for (const auto& keyValue : map) {
        // This is only validating that there are a sufficient number of uniforms
        // for the layout. The unifrom offset info is in the value of the binding map.
        const Descriptor::bindingMapValue& value = keyValue.second;
        UNIFORM_INDEX offset = value.second.empty() ? 0 : *value.second.rbegin();

        auto it = validationMap.find(value.first);
        if (it != validationMap.end()) {
            it->second = (std::max)(it->second, offset);
        } else {
            validationMap[value.first] = offset;
        }
    }
    // Ensure the highest offset value is within the data vectors
    for (const auto& keyValue : validationMap) {
        if (!validateUniformOffsets(keyValue)) return false;
    }
    return true;
}

bool Uniform::Handler::validateUniformOffsets(const std::pair<DESCRIPTOR, UNIFORM_INDEX>& pair) {
    if (DESCRIPTOR_UNIFORM_ALL.count(pair.first)) {
        return pair.second < getUniforms(pair.first).size();
    }
    return true;
}

std::vector<VkDescriptorBufferInfo> Uniform::Handler::getWriteInfos(const Descriptor::bindingMapValue& value) {
    std::vector<VkDescriptorBufferInfo> infos;
    auto& uniforms = getUniforms(value.first);
    auto offsets = getBindingOffsets(value);
    for (const auto& offset : offsets) infos.push_back(uniforms[offset]->BUFFER_INFO.bufferInfo);
    return infos;
}

std::string Uniform::Handler::macroReplace(const Descriptor::bindingMap& bindingMap, std::string text) {
    for (const auto& keyValue : bindingMap) {
        if (DESCRIPTOR_UNIFORM_ALL.count(keyValue.second.first)) {
            // TODO: Ugh.
            const auto& macroName = getMacroName(keyValue.second.first);
            const auto& uniformCount = getUniforms(keyValue.second.first).size();

            auto regexStr = MACRO_REGEX_TEMPLATE;
            regexStr = helpers::replaceFirstOccurrence(MACRO_KEY, macroName, regexStr);
            std::regex macroRegex(regexStr);

            std::smatch matches;
            std::regex_search(text, matches, macroRegex);

            if (matches.empty()) continue;

            assert(matches.size() == MACRO_REGEX_GROUP + 1);
            UNIFORM_INDEX macroValue = std::stoi(matches[MACRO_REGEX_GROUP]);

            // If macro value is non-zero then just make sure there are enough uniforms
            if (macroValue != 0) {
                assert(macroValue <= uniformCount);
            } else {
                std::stringstream ss;
                for (uint8_t i = 0; i < matches.size(); i++) {
                    if (i == 0)
                        continue;
                    else if (i == 3)
                        ss << uniformCount;
                    else
                        ss << matches[i] << " ";
                }
                text = helpers::replaceFirstOccurrence(matches[0], ss.str(), text);
            }
        }
    }
    return text;
}