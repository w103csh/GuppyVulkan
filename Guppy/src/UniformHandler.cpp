
#include "UniformHandler.h"

#include "InputHandler.h"
#include "MeshHandler.h"
#include "Shell.h"

namespace {

const std::string UNIFORM_MACRO_ID_PREFIX = "UMI_";

//// TODO: what should this be???
// struct _UniformTag {
//    const char name[17] = "ubo tag";
//} tag;

}  // namespace

Uniform::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),
      managers_{
          // CAMERA
          Uniform::Manager<Camera::Default::Perspective::Base>  //
          {"Default Perspective Camera", DESCRIPTOR::CAMERA_PERSPECTIVE_DEFAULT, 5, "UMI_CAM_DEF_PERS"},
          // LIGHT
          Uniform::Manager<Light::Default::Positional::Base>  //
          {"Default Positional Light", DESCRIPTOR::LIGHT_POSITIONAL_DEFAULT, 20, "UMI_LGT_DEF_POS"},
          Uniform::Manager<Light::PBR::Positional::Base>  //
          {"PBR Positional Light", DESCRIPTOR::LIGHT_POSITIONAL_PBR, 20, "UMI_LGT_PBR_POS"},
          Uniform::Manager<Light::Default::Spot::Base>  //
          {"Default Spot Light", DESCRIPTOR::LIGHT_SPOT_DEFAULT, 20, "UMI_LGT_DEF_SPT"},
          // MISCELLANEOUS
          Uniform::Manager<Uniform::Default::Fog::Base>  //
          {"Default Fog", DESCRIPTOR::FOG_DEFAULT, 5, "UMI_UNI_DEF_FOG"}
          //
      },
      hasVisualHelpers(false) {}

void Uniform::Handler::init() {
    reset();

    camDefPersMgr().init(shell().context(), settings());
    lgtDefPosMgr().init(shell().context(), settings());
    lgtPbrPosMgr().init(shell().context(), settings());
    lgtDefSptMgr().init(shell().context(), settings());
    uniDefFogMgr().init(shell().context(), settings());

    createCameras();
    createLights();
    createMiscellaneous();
}

void Uniform::Handler::reset() {
    const auto& dev = shell().context().dev;
    // CAMERAS
    camDefPersMgr().destroy(dev);
    // LIGHTS
    lgtDefPosMgr().destroy(dev);
    lgtPbrPosMgr().destroy(dev);
    lgtDefSptMgr().destroy(dev);
    // MISCELLANEOUS
    uniDefFogMgr().destroy(dev);
}

void Uniform::Handler::createCameras() {
    const auto& dev = shell().context().dev;
    Camera::Default::Perspective::CreateInfo createInfo = {};

    createInfo.aspect = static_cast<float>(settings().initial_width) / static_cast<float>(settings().initial_height);
    camDefPersMgr().insert(dev, &createInfo);
    camDefPersMgr().insert(dev, &createInfo);

    assert(MAIN_CAMERA_OFFSET < camDefPersMgr().pItems.size());
}

void Uniform::Handler::createLights() {
    const auto& dev = shell().context().dev;
    Light::CreateInfo createInfo = {};

    // POSITIONAL DEFAULT
    createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(20.5f, 10.5f, -23.5f));
    lgtDefPosMgr().insert(dev, &createInfo);
    createInfo.model = helpers::affine(glm::vec3(1.0f), {-2.5f, 4.5f, -1.5f});
    lgtDefPosMgr().insert(dev, &createInfo);
    createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(-20.0f, 5.0f, -6.0f));
    lgtDefPosMgr().insert(dev, &createInfo);

    // POSITIONAL PBR (TODO: these being seperately created is really dumb!!! If this is
    // necessary there should be one set of positional lights, and multiple data buffers
    // for the one set...)
    createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(10.5f, 17.5f, -23.5f));
    // lgtPbrPosMgr().insert(dev, &createInfo);
    createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(22.5f, 5.5f, -7.5f));
    // lgtPbrPosMgr().insert(dev, &createInfo);

    // SPOT DEFAULT
    Light::Default::Spot::CreateInfo spotCreateInfo = {};
    spotCreateInfo.exponent = glm::radians(25.0f);
    spotCreateInfo.exponent = 25.0f;
    spotCreateInfo.model = helpers::viewToWorld({0.0f, 4.5f, 1.0f}, {0.0f, 0.0f, -1.5f}, UP_VECTOR);
    lgtDefSptMgr().insert(dev, &spotCreateInfo);
}

void Uniform::Handler::createMiscellaneous() {
    const auto& dev = shell().context().dev;
    // FOG
    uniDefFogMgr().insert(dev);
}

void Uniform::Handler::createVisualHelpers() {
    // DEFAULT POSITIONAL
    for (auto& pItem : lgtDefPosMgr().pItems) {
        auto& lgt = lgtDefPosMgr().getTypedItem<Light::Default::Positional::Base>(pItem);
        meshHandler().makeModelSpaceVisualHelper(lgt);
        hasVisualHelpers = true;
    }
    // PBR POSITIONAL
    for (auto& pItem : lgtPbrPosMgr().pItems) {
        auto& lgt = lgtPbrPosMgr().getTypedItem<Light::PBR::Positional::Base>(pItem);
        meshHandler().makeModelSpaceVisualHelper(lgt);
        hasVisualHelpers = true;
    }
    // DEFAULT SPOT
    for (auto& pItem : lgtDefSptMgr().pItems) {
        auto& lgt = lgtDefSptMgr().getTypedItem<Light::Default::Spot::Base>(pItem);
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
    for (auto& pItem : lgtDefPosMgr().pItems) {
        auto& lgt = lgtDefPosMgr().getTypedItem<Light::Default::Positional::Base>(pItem);
        lgt.update(camera.getCameraSpacePosition(lgt.getPosition()));
        update(lgt);
    }
    // lgtDefPosMgr().update(shell().context().dev);
    // PBR POSITIONAL
    for (auto& pItem : lgtPbrPosMgr().pItems) {
        auto& lgt = lgtPbrPosMgr().getTypedItem<Light::PBR::Positional::Base>(pItem);
        lgt.update(camera.getCameraSpacePosition(lgt.getPosition()));
        update(lgt);
    }
    // lgtDefPosMgr().update(shell().context().dev);
    // DEFAULT SPOT
    for (auto& pItem : lgtDefSptMgr().pItems) {
        auto& lgt = lgtDefSptMgr().getTypedItem<Light::Default::Spot::Base>(pItem);
        lgt.update(camera.getCameraSpaceDirection(lgt.getDirection()), camera.getCameraSpacePosition(lgt.getPosition()));
        update(lgt);
    }
    // lgtDefPosMgr().update(shell().context().dev);
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

void Uniform::Handler::shaderTextReplace(std::string& text) {
    auto replaceInfo = helpers::getMacroReplaceInfo(UNIFORM_MACRO_ID_PREFIX, text);
    for (auto& info : replaceInfo) {
        bool isValid = false;
        for (auto& manager : managers_) {
            if (std::get<0>(info).compare(std::visit(MacroName{}, manager)) == 0) {
                auto itemCount = std::visit(ItemCount{}, manager);
                auto reqCount = std::get<3>(info);
                if (reqCount > 0) {
                    // If the value for the macro in the shader text is greater than zero
                    // then just make sure there are enought uniforms to meet the requirement.
                    assert(reqCount <= itemCount && "Not enough uniforms");
                    isValid = true;
                } else if (reqCount == 0) {
                    // If the value for the macro in the shader text is zero
                    // then use all uniforms available.
                    if (itemCount > 0) {
                        helpers::macroReplace(info, static_cast<int>(itemCount), text);
                    }
                    isValid = true;
                }
                assert(isValid && "Macro value is expected to be positive for now");
            }
        }
        assert(isValid && "Could not find a uniform manager for the identifier");
    }
}