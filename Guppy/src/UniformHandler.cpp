
#include "UniformHandler.h"

#include <glm/glm.hpp>

#include "Shell.h"
// HANDLERS
#include "InputHandler.h"
#include "MeshHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"

namespace {

const std::string UNIFORM_MACRO_ID_PREFIX = "_U_";

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
          {"Default Perspective Camera", UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, 5, "_U_CAM_DEF_PERS"},
          // LIGHT
          Uniform::Manager<Light::Default::Positional::Base>  //
          {"Default Positional Light", UNIFORM::LIGHT_POSITIONAL_DEFAULT, 20, "_U_LGT_DEF_POS"},
          Uniform::Manager<Light::PBR::Positional::Base>  //
          {"PBR Positional Light", UNIFORM::LIGHT_POSITIONAL_PBR, 20, "_U_LGT_PBR_POS"},
          Uniform::Manager<Light::Default::Spot::Base>  //
          {"Default Spot Light", UNIFORM::LIGHT_SPOT_DEFAULT, 20, "_U_LGT_DEF_SPT"},
          // MISCELLANEOUS
          Uniform::Manager<Uniform::Default::Fog::Base>  //
          {"Default Fog", UNIFORM::FOG_DEFAULT, 5, "_U_UNI_DEF_FOG"},
          Uniform::Manager<Uniform::Default::Projector::Base>  //
          {"Default Projector", UNIFORM::PROJECTOR_DEFAULT, 5, "_U_UNI_DEF_PRJ"},
          //
      },
      hasVisualHelpers(false),
      mainCameraOffset_(0) {}

void Uniform::Handler::init() {
    reset();

    /* Get any overriden descriptor offsets from the render pass/pipeline
     *  handlers, so that when the descriptor set layouts are created
     *  the offsets manager is ready.
     */
    auto passOffsets = passHandler().makeUniformOffsetsMap();
    offsetsManager_.addOffsets(passOffsets, OffsetsManager::ADD_TYPE::RenderPass);
    passOffsets = pipelineHandler().makeUniformOffsetsMap();
    offsetsManager_.addOffsets(passOffsets, OffsetsManager::ADD_TYPE::Pipeline);

    camDefPersMgr().init(shell().context(), settings());
    lgtDefPosMgr().init(shell().context(), settings());
    lgtPbrPosMgr().init(shell().context(), settings());
    lgtDefSptMgr().init(shell().context(), settings());
    uniDefFogMgr().init(shell().context(), settings());
    uniDefPrjMgr().init(shell().context(), settings());

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
    uniDefPrjMgr().destroy(dev);
}

void Uniform::Handler::createCameras() {
    const auto& dev = shell().context().dev;
    Camera::Default::Perspective::CreateInfo createInfo = {};

    // MAIN
    createInfo.aspect = static_cast<float>(settings().initial_width) / static_cast<float>(settings().initial_height);
    camDefPersMgr().insert(dev, &createInfo);
    mainCameraOffset_ = camDefPersMgr().pItems.size() - 1;

    // 1
    createInfo.eye = {2.0f, -2.0f, 4.0f};
    camDefPersMgr().insert(dev, &createInfo);
    // mainCameraOffset_ = camDefPersMgr().pItems.size() - 1;

    assert(mainCameraOffset_ < camDefPersMgr().pItems.size());
}

void Uniform::Handler::createLights() {
    const auto& dev = shell().context().dev;
    Light::CreateInfo createInfo = {};

    // POSITIONAL
    // (TODO: these being seperately created is really dumb!!! If this is
    // necessary there should be one set of positional lights, and multiple data buffers
    // for the one set...)
    createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(20.5f, 10.5f, -23.5f));
    lgtDefPosMgr().insert(dev, &createInfo);
    lgtPbrPosMgr().insert(dev, &createInfo);
    createInfo.model = helpers::affine(glm::vec3(1.0f), {-2.5f, 4.5f, -1.5f});
    lgtDefPosMgr().insert(dev, &createInfo);
    lgtPbrPosMgr().insert(dev, &createInfo);
    createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(-20.0f, 5.0f, -6.0f));
    lgtDefPosMgr().insert(dev, &createInfo);
    lgtPbrPosMgr().insert(dev, &createInfo);
    createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(-100.0f, 10.0f, 100.0f));
    lgtDefPosMgr().insert(dev, &createInfo);
    lgtPbrPosMgr().insert(dev, &createInfo);

    // SPOT
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
    uniDefFogMgr().insert(dev);

    // PROJECTOR
    {
        glm::vec3 eye, center;

        eye = {2.0f, 4.0f, 0.0f};
        center = {-3.0f, 0.0f, 0.0f};

        auto view = glm::lookAt(eye, center, UP_VECTOR);
        // Don't forget to use the vulkan clip transform.
        auto proj = getMainCamera().getClip() * glm::perspective(glm::radians(30.0f), 1.0f, 0.2f, 1000.0f);

        // Normalized screen space transform (texture coord space) s,t,r : [0, 1]
        auto bias = glm::translate(glm::mat4{1.0f}, glm::vec3{0.5f});
        bias = glm::scale(bias, glm::vec3{0.5f});

        auto projector = bias * proj * view;

        uniDefPrjMgr().insert(dev, true, {{projector}});
    }
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

uint32_t Uniform::Handler::getDescriptorCount(const DESCRIPTOR& descType, const Uniform::offsets& offsets) {
    return getDescriptorCount(getItems(descType), offsets);
}

uint32_t Uniform::Handler::getDescriptorCount(const std::vector<ItemPointer<Uniform::Base>>& pItems,
                                              const Uniform::offsets& offsets) const {
    auto resolvedOffsets = getBindingOffsets(pItems, offsets);
    return static_cast<uint32_t>(resolvedOffsets.size());
}

std::set<uint32_t> Uniform::Handler::getBindingOffsets(const std::vector<ItemPointer<Uniform::Base>>& pItems,
                                                       const Uniform::offsets& offsets) const {
    assert(offsets.size());
    const auto& lowest = *offsets.begin();
    if (lowest == Descriptor::Set::OFFSET_ALL) {
        std::set<uint32_t> resolvedOffsets;
        for (int i = 0; i < pItems.size(); i++) resolvedOffsets.insert(i);
        assert(resolvedOffsets.size());
        return resolvedOffsets;
    }
    assert(lowest >= 0 && *offsets.rbegin() < pItems.size());
    return offsets;
}

bool Uniform::Handler::validatePipelineLayout(const Descriptor::bindingMap& map) {
    std::map<DESCRIPTOR, index> validationMap;
    // Find the highest offset value for each descriptor type.
    for (const auto& keyValue : map) {
        // This is only validating that there are a sufficient number of uniforms
        // for the layout. The unifrom offset info is in the value of the binding map.
        const Descriptor::bindingMapValue& value = keyValue.second;
        index offset = std::get<1>(value).empty() ? 0 : *std::get<1>(value).rbegin();

        auto it = validationMap.find(std::get<0>(value));
        if (it != validationMap.end()) {
            it->second = (std::max)(it->second, offset);
        } else {
            validationMap[std::get<0>(value)] = offset;
        }
    }
    // Ensure the highest offset value is within the data vectors
    for (const auto& keyValue : validationMap) {
        if (!validateUniformOffsets(keyValue)) return false;
    }
    return true;
}

bool Uniform::Handler::validateUniformOffsets(const std::pair<DESCRIPTOR, index>& pair) {
    if (std::visit(Descriptor::IsUniform{}, pair.first)) {
        return pair.second < getItems(pair.first).size();
    }
    return true;
}

std::vector<VkDescriptorBufferInfo> Uniform::Handler::getWriteInfos(const DESCRIPTOR& descType,
                                                                    const Uniform::offsets& offsets) {
    std::vector<VkDescriptorBufferInfo> infos;
    auto& pItems = getItems(descType);
    auto resolvedOffsets = getBindingOffsets(pItems, offsets);
    assert(*std::prev(resolvedOffsets.end()) < pItems.size());  // Not enough uniforms for highest offset
    for (const auto& offset : resolvedOffsets) infos.push_back(pItems[offset]->BUFFER_INFO.bufferInfo);
    return infos;
}

void Uniform::Handler::shaderTextReplace(const Descriptor::Set::textReplaceTuples& replaceTuples, std::string& text) const {
    auto replaceInfo = helpers::getMacroReplaceInfo(UNIFORM_MACRO_ID_PREFIX, text);
    for (auto& info : replaceInfo) {
        bool isValid = false;
        // Find the manager in charge of the descriptor type.
        for (auto& manager : managers_) {
            if (std::get<0>(info) == std::visit(GetMacroName{}, manager)) {
                // Find the item count for the offsets.
                const auto& pItems = std::visit(GetItems{}, manager);
                const auto& descType = std::visit(GetType{}, manager);
                int itemCount = -1;
                for (const auto& tuple : replaceTuples) {
                    auto search = std::get<2>(tuple).map().find(descType);
                    if (search != std::get<2>(tuple).map().end()) {
                        itemCount = static_cast<int>(getDescriptorCount(pItems, search->second));
                    }
                }
                assert(itemCount != -1);
                // TODO: not sure about below anymore. It was written a long time ago at this point.
                auto reqCount = std::get<3>(info);
                if (reqCount > 0) {
                    // If the value for the macro in the shader text is greater than zero
                    // then just make sure there are enough uniforms to meet the requirement.
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