
#include "UniformHandler.h"

#include <glm/glm.hpp>

#include "Shell.h"
// HANDLERS
#include "InputHandler.h"
#include "MeshHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"

namespace {

const std::set<std::string_view> MACRO_ID_PREFIXES = {
    "_U_",
    "_S_",
};

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
          {"Default Perspective Camera", UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, 6, "_U_CAM_DEF_PERS"},
          // LIGHT
          Uniform::Manager<Light::Default::Positional::Base>  //
          {"Default Positional Light", UNIFORM::LIGHT_POSITIONAL_DEFAULT, 20, "_U_LGT_DEF_POS"},
          Uniform::Manager<Light::PBR::Positional::Base>  //
          {"PBR Positional Light", UNIFORM::LIGHT_POSITIONAL_PBR, 20, "_U_LGT_PBR_POS"},
          Uniform::Manager<Light::Default::Spot::Base>  //
          {"Default Spot Light", UNIFORM::LIGHT_SPOT_DEFAULT, 20, "_U_LGT_DEF_SPT"},
          // MISCELLANEOUS
          Uniform::Manager<Uniform::Default::Fog::Base>  //
          {"Default Fog", UNIFORM::FOG_DEFAULT, 5, "_U_DEF_FOG"},
          Uniform::Manager<Uniform::Default::Projector::Base>  //
          {"Default Projector", UNIFORM::PROJECTOR_DEFAULT, 5, "_U_DEF_PRJ"},
          Uniform::Manager<Uniform::ScreenSpace::Default>  //
          {"Screen Space Default", UNIFORM::SCREEN_SPACE_DEFAULT, 5, "_U_SCR_DEF"},
          // STORAGE
          Uniform::Manager<Storage::PostProcess::Base>  //
          {"Storage Default", STORAGE_BUFFER::POST_PROCESS, 5, "_S_DEF_PSTPRC"},
          //
      },
      hasVisualHelpers(false),
      mainCameraOffset_(0) {}

std::vector<std::unique_ptr<Descriptor::Base>>& Uniform::Handler::getItems(const DESCRIPTOR& type) {
    // clang-format off
    if (std::visit(Descriptor::IsUniform{}, type)) {
        switch (std::visit(Descriptor::GetUniform{}, type)) {
            case UNIFORM::CAMERA_PERSPECTIVE_DEFAULT:   return camDefPersMgr().pItems;
            case UNIFORM::LIGHT_POSITIONAL_DEFAULT:     return lgtDefPosMgr().pItems;
            case UNIFORM::LIGHT_POSITIONAL_PBR:         return lgtPbrPosMgr().pItems;
            case UNIFORM::LIGHT_SPOT_DEFAULT:           return lgtDefSptMgr().pItems;
            case UNIFORM::FOG_DEFAULT:                  return uniDefFogMgr().pItems;
            case UNIFORM::PROJECTOR_DEFAULT:            return uniDefPrjMgr().pItems;
            case UNIFORM::SCREEN_SPACE_DEFAULT:         return uniScrDefMgr().pItems;
            default:                                    assert(false); exit(EXIT_FAILURE);
        }
    } else if (std::visit(Descriptor::IsStorageBuffer{}, type)) {
        switch (std::visit(Descriptor::GetStorageBuffer{}, type)) {
            case STORAGE_BUFFER::POST_PROCESS:          return strPstPrcMgr().pItems;
            default:                                    assert(false); exit(EXIT_FAILURE);
        }
    } else {
        assert(false); exit(EXIT_FAILURE);
    }
    // clang-format on
}

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

    // clang-format off
    uint8_t count = 0;
    camDefPersMgr().init(shell().context(), settings());    ++count;
    lgtDefPosMgr().init(shell().context(), settings());     ++count;
    lgtPbrPosMgr().init(shell().context(), settings());     ++count;
    lgtDefSptMgr().init(shell().context(), settings());     ++count;
    uniDefFogMgr().init(shell().context(), settings());     ++count;
    uniDefPrjMgr().init(shell().context(), settings());     ++count;
    uniScrDefMgr().init(shell().context(), settings());     ++count;
    strPstPrcMgr().init(shell().context(), settings());     ++count;
    assert(count == managers_.size());
    // clang-format on

    createCameras();
    createLights();
    createMiscellaneous();
}

void Uniform::Handler::reset() {
    const auto& dev = shell().context().dev;
    // clang-format off
    uint8_t count = 0;
    camDefPersMgr().destroy(dev);   ++count;
    lgtDefPosMgr().destroy(dev);    ++count;
    lgtPbrPosMgr().destroy(dev);    ++count;
    lgtDefSptMgr().destroy(dev);    ++count;
    uniDefFogMgr().destroy(dev);    ++count;
    uniDefPrjMgr().destroy(dev);    ++count;
    uniScrDefMgr().destroy(dev);    ++count;
    strPstPrcMgr().destroy(dev);    ++count;
    assert(count == managers_.size());
    // clang-format on
}

void Uniform::Handler::createCameras() {
    const auto& dev = shell().context().dev;

    Camera::Default::Perspective::CreateInfo createInfo = {};

    assert(shell().context().imageCount == 3);  // Potential imageCount problem
    createInfo.dataCount = shell().context().imageCount;

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

    assert(shell().context().imageCount == 3);  // Potential imageCount problem
    createInfo.dataCount = shell().context().imageCount;

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
    spotCreateInfo.dataCount = shell().context().imageCount;
    spotCreateInfo.exponent = glm::radians(25.0f);
    spotCreateInfo.exponent = 25.0f;
    spotCreateInfo.model = helpers::viewToWorld({0.0f, 4.5f, 1.0f}, {0.0f, 0.0f, -1.5f}, UP_VECTOR);
    lgtDefSptMgr().insert(dev, &spotCreateInfo);
}

void Uniform::Handler::createMiscellaneous() {
    const auto& dev = shell().context().dev;

    // FOG
    {
        uniDefFogMgr().insert(dev);
        uniDefFogMgr().insert(dev);
    }

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

    // SCREEN SPACE
    {
        uniScrDefMgr().insert(dev);

        assert(shell().context().imageCount == 3);  // Potential imageCount problem

        Buffer::CreateInfo info = {shell().context().imageCount, false};
        strPstPrcMgr().insert(dev, &info);
    }
}

void Uniform::Handler::createVisualHelpers() {
    // DEFAULT POSITIONAL
    for (uint32_t i = 0; i < lgtDefPosMgr().pItems.size(); i++) {
        auto& lgt = lgtDefPosMgr().getTypedItem(i);
        meshHandler().makeModelSpaceVisualHelper(lgt);
        hasVisualHelpers = true;
    }
    // PBR POSITIONAL
    for (uint32_t i = 0; i < lgtPbrPosMgr().pItems.size(); i++) {
        auto& lgt = lgtPbrPosMgr().getTypedItem(i);
        meshHandler().makeModelSpaceVisualHelper(lgt);
        hasVisualHelpers = true;
    }
    // DEFAULT SPOT
    for (uint32_t i = 0; i < lgtDefSptMgr().pItems.size(); i++) {
        auto& lgt = lgtDefSptMgr().getTypedItem(i);
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

void Uniform::Handler::attachSwapchain() {
    // POST-PROCESS
    // TODO: Test stuff remove me !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    auto& computeData = strPstPrcMgr().getTypedItem(0);
    computeData.setImageSize(shell().context().extent.height * shell().context().extent.width);
    update(computeData);

    // Any items that rely on the number of framebuffers, should be validated here. This needs to
    // be thought through.
}

void Uniform::Handler::update() {
    const auto frameIndex = passHandler().getFrameIndex();

    // MAIN CAMERA
    auto& camera = getMainCamera();
    camera.update(InputHandler::getPosDir(), InputHandler::getLookDir(), frameIndex);
    update(camera, static_cast<int>(frameIndex));

    // DEFAULT POSITIONAL
    for (uint32_t i = 0; i < lgtDefPosMgr().pItems.size(); i++) {
        auto& lgt = lgtDefPosMgr().getTypedItem(i);
        lgt.update(camera.getCameraSpacePosition(lgt.getPosition()), frameIndex);
        update(lgt, static_cast<int>(frameIndex));
    }
    // lgtDefPosMgr().update(shell().context().dev);
    // PBR POSITIONAL
    for (uint32_t i = 0; i < lgtPbrPosMgr().pItems.size(); i++) {
        auto& lgt = lgtPbrPosMgr().getTypedItem(i);
        lgt.update(camera.getCameraSpacePosition(lgt.getPosition()), frameIndex);
        update(lgt, static_cast<int>(frameIndex));
    }
    // lgtDefPosMgr().update(shell().context().dev);

    // DEFAULT SPOT
    for (uint32_t i = 0; i < lgtDefSptMgr().pItems.size(); i++) {
        auto& lgt = lgtDefSptMgr().getTypedItem(i);
        lgt.update(camera.getCameraSpaceDirection(lgt.getDirection()), camera.getCameraSpacePosition(lgt.getPosition()),
                   frameIndex);
        update(lgt, static_cast<int>(frameIndex));
    }

    // POST-PROCESS
    auto& computeData = strPstPrcMgr().getTypedItem(0);
    computeData.reset(frameIndex);
    update(computeData, static_cast<int>(frameIndex));
}

uint32_t Uniform::Handler::getDescriptorCount(const DESCRIPTOR& descType, const Uniform::offsets& offsets) {
    return getDescriptorCount(getItems(descType), offsets);
}

uint32_t Uniform::Handler::getDescriptorCount(const std::vector<ItemPointer<Descriptor::Base>>& pItems,
                                              const Uniform::offsets& offsets) const {
    auto resolvedOffsets = getBindingOffsets(pItems, offsets);
    return static_cast<uint32_t>(resolvedOffsets.size());
}

std::set<uint32_t> Uniform::Handler::getBindingOffsets(const std::vector<ItemPointer<Descriptor::Base>>& pItems,
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

bool Uniform::Handler::validateUniformOffsets(const std::pair<DESCRIPTOR, index>& pair) {
    if (std::visit(Descriptor::HasOffsets{}, pair.first)) {
        return pair.second < getItems(pair.first).size();
    }
    return true;
}

void Uniform::Handler::getWriteInfos(const DESCRIPTOR& descType, const Uniform::offsets& offsets,
                                     Descriptor::Set::ResourceInfo& setResInfo) {
    auto& pItems = getItems(descType);
    // All of the offset needed in a list.
    auto resolvedOffsets = getBindingOffsets(pItems, offsets);
    // Check for enough uniforms for highest offset
    assert(*std::prev(resolvedOffsets.end()) < pItems.size());

    setResInfo.descCount = static_cast<uint32_t>(resolvedOffsets.size());
    setResInfo.bufferInfos.resize(resolvedOffsets.size() * setResInfo.uniqueDataSets);

    // Set the buffer infos
    uint32_t i = 0;
    for (const auto& offset : resolvedOffsets) {
        if (resolvedOffsets.size() > 1)  //
            auto x = 1;
        pItems[offset]->setDescriptorInfo(setResInfo, i++);
    }
}

void Uniform::Handler::shaderTextReplace(const Descriptor::Set::textReplaceTuples& replaceTuples, std::string& text) const {
    for (const auto& macroIdPrefix : MACRO_ID_PREFIXES) {
        auto replaceInfo = helpers::getMacroReplaceInfo(macroIdPrefix, text);
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
}