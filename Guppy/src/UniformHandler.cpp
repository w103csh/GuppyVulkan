/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "UniformHandler.h"

#include <glm/glm.hpp>
#include <sstream>

#include "RenderPassManager.h"
#include "Shell.h"
// HANDLERS
#include "InputHandler.h"
#include "MeshHandler.h"
#include "PipelineHandler.h"
#include "PassHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

namespace {

const std::set<std::string_view> MACRO_ID_PREFIXES = {
    "_U_",
    "_S_",
    "_UD_",
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
          Uniform::Manager<Camera::Perspective::Default::Base>  //
          {"Default Perspective Camera", UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, 4 * 3, "_U_CAM_DEF_PERS"},
          Uniform::Manager<Camera::Perspective::CubeMap::Base>  //
          {"Default Perspective Cube Map Camera", UNIFORM::CAMERA_PERSPECTIVE_CUBE_MAP, 3, "_U_CAM_PERS_CUBE"},
          // LIGHT
          Uniform::Manager<Light::Default::Directional::Base>  //
          {"Default Directional Light", UNIFORM::LIGHT_DIRECTIONAL_DEFAULT, 3, "_U_LGT_DEF_DIR"},
          Uniform::Manager<Light::Default::Positional::Base>  //
          {"Default Positional Light", UNIFORM::LIGHT_POSITIONAL_DEFAULT, 40, "_U_LGT_DEF_POS"},
          Uniform::Manager<Light::PBR::Positional::Base>  //
          {"PBR Positional Light", UNIFORM::LIGHT_POSITIONAL_PBR, 40, "_U_LGT_PBR_POS"},
          Uniform::Manager<Light::Default::Spot::Base>  //
          {"Default Spot Light", UNIFORM::LIGHT_SPOT_DEFAULT, 20, "_U_LGT_DEF_SPT"},
          Uniform::Manager<Light::Shadow::Positional::Base>  //
          {"Shadow Positional Light", UNIFORM::LIGHT_POSITIONAL_SHADOW, 3, "_U_LGT_SHDW_POS"},
          Uniform::Manager<Light::Shadow::Cube::Base>  //
          {"Shadow Cube Light", UNIFORM::LIGHT_CUBE_SHADOW, 9, "_U_LGT_SHDW_CUBE"},
          // MISCELLANEOUS
          Uniform::Manager<Default::Fog::Base>  //
          {"Default Fog", UNIFORM::FOG_DEFAULT, 5, "_U_DEF_FOG"},
          Uniform::Manager<Default::Projector::Base>  //
          {"Default Projector", UNIFORM::PROJECTOR_DEFAULT, 5, "_U_DEF_PRJ"},
          Uniform::Manager<ScreenSpace::Default>  //
          {"Screen Space Default", UNIFORM::SCREEN_SPACE_DEFAULT, 5, "_U_SCR_DEF"},
          Uniform::Manager<Deferred::SSAO>  //
          {"Deferred SSAO", UNIFORM::DEFERRED_SSAO, 5, "_U_DFR_SSAO"},
          Uniform::Manager<Shadow::Base>  //
          {"Shadow Data", UNIFORM::DEFERRED_SSAO, 1, "_U_SHDW_DATA"},
          // TESSELLATION
          Uniform::Manager<Tessellation::Default::Base>  //
          {"Tessellation Data", UNIFORM::TESS_DEFAULT, 2, "_U_TESS_DEF"},
          // GEOMETRY
          Uniform::Manager<Geometry::Default::Base>  //
          {"Geomerty Wireframe", UNIFORM::GEOMETRY_DEFAULT, 1, "_U_GEOM_WF"},
          // PARTICLE
          Uniform::Manager<Particle::Wave::Base>  //
          {"Particle Wave", UNIFORM::PRTCL_WAVE, 3, "_U_PRTCL_WAVE"},
          // STORAGE
          Uniform::Manager<Storage::PostProcess::Base>  //
          {"Post Process Data", STORAGE_BUFFER::POST_PROCESS, 5, "_S_DEF_PSTPRC"},
          // CDLOD
          Uniform::Manager<Uniform::Cdlod::QuadTree::Base>  //
          {"CDLOD Quad Tree Data", UNIFORM::CDLOD_QUAD_TREE, 1, "_U_CDLOD_QDTR"},
          //
      },
      managersDynamic_{
          // TESSELLATION
          UniformDynamic::Tessellation::Phong::Manager  //
          {"Tessellation Phong Data", UNIFORM_DYNAMIC::TESS_PHONG, 10, true, "_UD_TESS_PHONG"},
          // OCEAN
          UniformDynamic::Ocean::SimulationDispatch::Manager  //
          {"Ocean Simulation Dispatch Data", UNIFORM_DYNAMIC::OCEAN_DISPATCH, 2, true, "_UD_OCN_DISPATCH"},
          UniformDynamic::Ocean::SimulationDraw::Manager  //
          {"Ocean Simulation Draw Data", UNIFORM_DYNAMIC::OCEAN_DRAW, 6, true, "_UD_OCN_DRAW"},
      },
      activeCameraOffset_(BAD_OFFSET),
      mainCameraOffset_(BAD_OFFSET),
      debugCameraOffset_(BAD_OFFSET),
      hasVisualHelpers_(false) {}

std::vector<std::unique_ptr<Descriptor::Base>>& Uniform::Handler::getItems(const DESCRIPTOR& type) {
    // clang-format off
    if (std::visit(Descriptor::IsUniform{}, type)) {
        switch (std::visit(Descriptor::GetUniform{}, type)) {
            case UNIFORM::CAMERA_PERSPECTIVE_DEFAULT:   return camPersDefMgr().pItems;
            case UNIFORM::CAMERA_PERSPECTIVE_CUBE_MAP:  return camPersCubeMgr().pItems;
            case UNIFORM::LIGHT_DIRECTIONAL_DEFAULT:    return lgtDefDirMgr().pItems;
            case UNIFORM::LIGHT_POSITIONAL_DEFAULT:     return lgtDefPosMgr().pItems;
            case UNIFORM::LIGHT_POSITIONAL_PBR:         return lgtPbrPosMgr().pItems;
            case UNIFORM::LIGHT_SPOT_DEFAULT:           return lgtDefSptMgr().pItems;
            case UNIFORM::LIGHT_POSITIONAL_SHADOW:      return lgtShdwPosMgr().pItems;
            case UNIFORM::LIGHT_CUBE_SHADOW:            return lgtShdwCubeMgr().pItems;
            case UNIFORM::FOG_DEFAULT:                  return uniDefFogMgr().pItems;
            case UNIFORM::PROJECTOR_DEFAULT:            return uniDefPrjMgr().pItems;
            case UNIFORM::SCREEN_SPACE_DEFAULT:         return uniScrDefMgr().pItems;
            case UNIFORM::DEFERRED_SSAO:                return uniDfrSSAOMgr().pItems;
            case UNIFORM::TESS_DEFAULT:                 return uniTessDefMgr().pItems;
            case UNIFORM::GEOMETRY_DEFAULT:             return uniGeomDefMgr().pItems;
            case UNIFORM::PRTCL_WAVE:                   return uniWaveMgr().pItems;
            case UNIFORM::SHADOW_DATA:                  return uniShdwDataMgr().pItems;
            case UNIFORM::CDLOD_QUAD_TREE:              return cdlodQdTrMgr().pItems;
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
    auto passOffsets = passHandler().renderPassMgr().makeUniformOffsetsMap();
    offsetsManager_.addOffsets(passOffsets, OffsetsManager::ADD_TYPE::RenderPass);
    passOffsets = pipelineHandler().makeUniformOffsetsMap();
    offsetsManager_.addOffsets(passOffsets, OffsetsManager::ADD_TYPE::Pipeline);

    // clang-format off
    uint8_t count = 0;
    camPersDefMgr().init(shell().context());    ++count;
    camPersCubeMgr().init(shell().context());   ++count;
    lgtDefDirMgr().init(shell().context());     ++count;
    lgtDefPosMgr().init(shell().context());     ++count;
    lgtPbrPosMgr().init(shell().context());     ++count;
    lgtDefSptMgr().init(shell().context());     ++count;
    lgtShdwPosMgr().init(shell().context());    ++count;
    lgtShdwCubeMgr().init(shell().context());   ++count;
    uniDefFogMgr().init(shell().context());     ++count;
    uniDefPrjMgr().init(shell().context());     ++count;
    uniScrDefMgr().init(shell().context());     ++count;
    uniDfrSSAOMgr().init(shell().context());    ++count;
    uniShdwDataMgr().init(shell().context());   ++count;
    uniTessDefMgr().init(shell().context());    ++count;
    uniGeomDefMgr().init(shell().context());    ++count;
    uniWaveMgr().init(shell().context());       ++count;
    strPstPrcMgr().init(shell().context());     ++count;
    cdlodQdTrMgr().init(shell().context());     ++count;
    // DYNAMIC
    tessPhongMgr().init(shell().context());     ++count;
    ocnSimDpchMgr().init(shell().context());    ++count;
    ocnSimDrawMgr().init(shell().context());    ++count;
    assert(count == managers_.size() + managersDynamic_.size());
    // clang-format on

    createCameras();
    createLights();
    createMiscellaneous();
    createTessellationData();
}

void Uniform::Handler::reset() {
    // clang-format off
    uint8_t count = 0;
    camPersDefMgr().destroy(shell().context());   ++count;
    camPersCubeMgr().destroy(shell().context());  ++count;
    lgtDefDirMgr().destroy(shell().context());    ++count;
    lgtDefPosMgr().destroy(shell().context());    ++count;
    lgtPbrPosMgr().destroy(shell().context());    ++count;
    lgtDefSptMgr().destroy(shell().context());    ++count;
    lgtShdwPosMgr().destroy(shell().context());   ++count;
    lgtShdwCubeMgr().destroy(shell().context());  ++count;
    uniDefFogMgr().destroy(shell().context());    ++count;
    uniDefPrjMgr().destroy(shell().context());    ++count;
    uniScrDefMgr().destroy(shell().context());    ++count;
    uniDfrSSAOMgr().destroy(shell().context());   ++count;
    uniShdwDataMgr().destroy(shell().context());  ++count;
    uniTessDefMgr().destroy(shell().context());   ++count;
    uniGeomDefMgr().destroy(shell().context());   ++count;
    uniWaveMgr().destroy(shell().context());      ++count;
    strPstPrcMgr().destroy(shell().context());    ++count;
    cdlodQdTrMgr().destroy(shell().context());    ++count;
    // DYNAMIC
    tessPhongMgr().destroy(shell().context());    ++count;
    ocnSimDpchMgr().destroy(shell().context());   ++count;
    ocnSimDrawMgr().destroy(shell().context());   ++count;
    assert(count == managers_.size() + managersDynamic_.size());
    // clang-format on
}

void Uniform::Handler::frame() {
    const auto frameIndex = passHandler().renderPassMgr().getFrameIndex();
    const auto& playerInfo = shell().inputHandler().getInfo().players[0];

    // ACTIVE CAMERA
    auto& camera = getActiveCamera();
    float movementFactor = 10.0f;
    camera.update(playerInfo.moveDir, playerInfo.lookDir, frameIndex);
    update(camera, static_cast<int>(frameIndex));

    // DEFAULT DIRECTIONAL
    assert(lgtDefDirMgr().pItems.size() == 1);  // At the moment the shaders/offset manager are setup to handle one.
    for (uint32_t i = 0; i < lgtDefDirMgr().pItems.size(); i++) {
        auto& lgt = lgtDefDirMgr().getTypedItem(i);
        lgt.update(camera.getCameraSpaceDirection(lgt.direction), frameIndex);
        update(lgt, static_cast<int>(frameIndex));
    }

    // DEFAULT POSITIONAL
    for (uint32_t i = 0; i < lgtDefPosMgr().pItems.size(); i++) {
        auto& lgt = lgtDefPosMgr().getTypedItem(i);
        lgt.update(camera.getCameraSpacePosition(lgt.getPosition()), frameIndex);
        // lgt.update(glm::vec3(lgt.getPosition()), frameIndex);
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

    // SHADOW
    {
        auto cameraToWorld = camera.getCameraSpaceToWorldSpaceTransform();
        // POSITIONAL
        if (false) {
            auto& lgtShdwPos = lgtShdwPosMgr().getTypedItem(0);
            lgtShdwPos.update(cameraToWorld, frameIndex);
            update(lgtShdwPos, static_cast<int>(frameIndex));
        }
        // CUBE
        if (true) {
            auto& pScene = sceneHandler().getActiveScene();
            const auto elapsed = shell().getElapsedTime<float>();

            const auto RotateLight = [this, frameIndex = frameIndex](const glm::mat4& mRot, Mesh::Line* pIndicator,
                                                                     Light::Shadow::Cube::Base& lgtShdwCube,
                                                                     const uint32_t index) {
                auto newPos = glm::vec3(mRot * glm::vec4(lgtShdwCube.getPosition(), 1.0f));
                // Light
                lgtShdwCube.setPosition(newPos, frameIndex);
                update(lgtShdwCube, static_cast<int>(frameIndex));
                // Indicator
                if (pIndicator) {
                    pIndicator->transform(
                        glm::translate(glm::mat4(1.0f), newPos - pIndicator->getWorldSpacePosition({}, index)), index);
                    meshHandler().updateInstanceData(pIndicator->getInstanceDataInfo());
                }
            };

            bool rotate = false;

            for (uint32_t i = 0; i < static_cast<uint32_t>(lgtShdwCubeMgr().pItems.size()); i++) {
                auto& lgtShdwCube = lgtShdwCubeMgr().getTypedItem(i);
                Mesh::Line* pIndicator = (pScene->posLgtCubeShdwOffset != Mesh::BAD_OFFSET)
                                             ? meshHandler().getLineMesh(pScene->posLgtCubeShdwOffset).get()
                                             : nullptr;

                if (i == 0 && rotate) {
                    // 0 initial position: {-0.0f, 0.5f, -2.0f}
                    auto mRot = glm::rotate(glm::mat4(1.0f), (elapsed / 2.0f) * glm::two_pi<float>(),
                                            CARDINAL_Y + glm::vec3(-0.0f, 0.5f, -2.0f));
                    RotateLight(mRot, pIndicator, lgtShdwCube, i);
                } else if (i == 1 && rotate) {
                    // 1 initial position: {-4.0f, 3.5f, 5.0f}
                    auto mRot = glm::rotate(glm::mat4(1.0f), (elapsed / 25.0f) * glm::two_pi<float>(), CARDINAL_Y);
                    RotateLight(mRot, pIndicator, lgtShdwCube, i);
                } else if (i == 2 && rotate) {
                    // 2 initial position: {-2.0f, -2.5f, -4.3f}
                    auto mRot = glm::rotate(glm::mat4(1.0f), (elapsed / 15.0f) * glm::two_pi<float>(), CARDINAL_Y);
                    RotateLight(mRot, pIndicator, lgtShdwCube, i);
                }

                // Update camera space state
                lgtShdwCube.update(camera.getCameraSpacePosition(lgtShdwCube.getPosition()), frameIndex);
                update(lgtShdwCube, static_cast<int>(frameIndex));
            }
        }
    }
}

void Uniform::Handler::createCameras() {
    const auto& dev = shell().context().dev;

    Camera::Perspective::Default::CreateInfo defInfo = {};

    assert(shell().context().imageCount == 3);  // Potential imageCount problem
    defInfo.dataCount = shell().context().imageCount;

    // 0 (MAIN)
    {
        defInfo.aspect = static_cast<float>(settings().initialWidth) / static_cast<float>(settings().initialHeight);
        // createInfo.center = glm::vec3{-0.5f, 2.0f, 1.0f};
        // defInfo.eye = {4.0f, 6.0f, 4.0f};
        defInfo.eye = {100.0f, 100.0f, 100.0f};
        defInfo.f = 55000.0f;

        //// Cdlod defautls
        // defInfo.eye = {12823.0f, 1865.0f, -8423.0f};
        // defInfo.f = 45000.0f;

        camPersDefMgr().insert(dev, &defInfo);
        mainCameraOffset_ = camPersDefMgr().pItems.size() - 1;
        activeCameraOffset_ = mainCameraOffset_;
    }

    // 1 (PROJECTOR)
    {
        defInfo.eye = {2.0f, -2.0f, 4.0f};
        camPersDefMgr().insert(dev, &defInfo);
        // mainCameraOffset_ = camDefPersMgr().pItems.size() - 1;
    }

    // 2 (SHADOW)
    {
        assert(camPersDefMgr().pItems.size() == 2);  // sister assert for lights

        defInfo.eye = {5.0f, 10.5f, 0.0f};
        defInfo.center = {-2.0f, 0.0f, 0.0f};
        defInfo.n = 1.0f;
        defInfo.f = 21.0f;
        // createInfo.fov = 180.0f;
        camPersDefMgr().insert(dev, &defInfo);
        // mainCameraOffset_ = camDefPersMgr().pItems.size() - 1;
    }

    // CUBE MAP
    {
        Camera::Perspective::CubeMap::CreateInfo cubeInfo = {};
        assert(shell().context().imageCount == 3);  // Potential imageCount problem
        cubeInfo.dataCount = shell().context().imageCount;

        // 0
        camPersCubeMgr().insert(dev, &cubeInfo);
    }

    // (DEBUG)
    {
        defInfo.aspect = static_cast<float>(settings().initialWidth) / static_cast<float>(settings().initialHeight);
        defInfo.eye = {50.0f, 50.0f, 50.0f};
        defInfo.center = {};
        defInfo.n = 0.1f;
        defInfo.f = 45000.0f;
        // defInfo.eye = {0.0f, 0.0f, 1.0f};
        // defInfo.center = {};
        // defInfo.n = 0.000001f;
        // defInfo.f = 2.0f;
        camPersDefMgr().insert(dev, &defInfo);
        debugCameraOffset_ = camPersDefMgr().pItems.size() - 1;
    }

    assert(mainCameraOffset_ != BAD_OFFSET);
    assert(activeCameraOffset_ == mainCameraOffset_);
}

void Uniform::Handler::createLights() {
    const auto& dev = shell().context().dev;

    // DIRECTIONAL
    Light::Default::Directional::CreateInfo defDirInfo = {};
    assert(shell().context().imageCount == 3);  // Potential imageCount problem
    defDirInfo.dataCount = shell().context().imageCount;
    // MOON
    // defDirInfo.direction = glm::normalize(glm::vec3(0, 0.1f, 1.0f));  // direction to the light(s) (world space)
    defDirInfo.direction = glm::normalize(glm::vec3(0, 1.0f, 1.0f));
    lgtDefDirMgr().insert(dev, &defDirInfo);

    Light::CreateInfo lightCreateInfo = {};

    assert(shell().context().imageCount == 3);  // Potential imageCount problem
    lightCreateInfo.dataCount = shell().context().imageCount;

    // POSITIONAL
    if (true) {
        // (TODO: these being seperately created is really dumb!!! If this is
        // necessary there should be one set of positional lights, and multiple data buffers
        // for the one set...)
        // lightCreateInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(20.5f, 10.5f, -23.5f));
        lightCreateInfo.model = helpers::affine(glm::vec3(1.0f), camPersDefMgr().getTypedItem(2).getWorldSpacePosition());
        lgtDefPosMgr().insert(dev, &lightCreateInfo);
        lgtPbrPosMgr().insert(dev, &lightCreateInfo);
        lightCreateInfo.model = helpers::affine(glm::vec3(1.0f), {-2.5f, 4.5f, -1.5f});
        lgtDefPosMgr().insert(dev, &lightCreateInfo);
        lgtPbrPosMgr().insert(dev, &lightCreateInfo);
        lightCreateInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(-20.0f, 5.0f, -6.0f));
        lgtDefPosMgr().insert(dev, &lightCreateInfo);
        lgtPbrPosMgr().insert(dev, &lightCreateInfo);
        lightCreateInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(-100.0f, 10.0f, 100.0f));
        lgtDefPosMgr().insert(dev, &lightCreateInfo);
        lgtPbrPosMgr().insert(dev, &lightCreateInfo);
    }

    //// Bloom test
    // createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(-7.0f, 4.0f, 2.5f));
    // lgtDefPosMgr().insert(dev, &createInfo);
    // lgtPbrPosMgr().insert(dev, &createInfo);
    // createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(0.0f, 4.0f, 2.5f));
    // lgtDefPosMgr().insert(dev, &createInfo);
    // lgtPbrPosMgr().insert(dev, &createInfo);
    // createInfo.model = helpers::affine(glm::vec3(1.0f), glm::vec3(7.0f, 4.0f, 2.5f));
    // lgtDefPosMgr().insert(dev, &createInfo);
    // lgtPbrPosMgr().insert(dev, &createInfo);

    // SPOT
    if (true) {
        Light::Default::Spot::CreateInfo spotCreateInfo = {};
        spotCreateInfo.dataCount = shell().context().imageCount;
        spotCreateInfo.exponent = glm::radians(25.0f);
        spotCreateInfo.exponent = 25.0f;
        spotCreateInfo.model = helpers::viewToWorld({0.0f, 4.5f, 1.0f}, {0.0f, 0.0f, -1.5f}, UP_VECTOR);
        lgtDefSptMgr().insert(dev, &spotCreateInfo);
    }

    // SHADOW
    {
        // POSITIONAL
        if (false) {
            uint32_t shadowCamIndex = 2;
            assert(shadowCamIndex == 2);  // sister assert for camera

            auto& camera = camPersDefMgr().getTypedItem(shadowCamIndex);

            Light::Shadow::Positional::CreateInfo lightShadowCreateInfo = {};
            lightShadowCreateInfo.dataCount = shell().context().imageCount;
            lightShadowCreateInfo.proj = helpers::getBias() * camera.getMVP();
            lightShadowCreateInfo.mainCameraSpaceToWorldSpace = getMainCamera().getCameraSpaceToWorldSpaceTransform();

            lgtShdwPosMgr().insert(dev, &lightShadowCreateInfo);
        }

        // CUBE
        if (true) {
            Light::Shadow::Cube::CreateInfo cubeInfo = {};

            assert(shell().context().imageCount == 3);  // Potential imageCount problem
            cubeInfo.dataCount = shell().context().imageCount;
            cubeInfo.n = 0.1f;
            cubeInfo.f = 20.0f;

            cubeInfo.position = {-0.0f, 0.5f, -2.0f};
            lgtShdwCubeMgr().insert(dev, &cubeInfo);

            cubeInfo.position = {-4.0f, 3.5f, 5.0f};
            lgtShdwCubeMgr().insert(dev, &cubeInfo);

            // cubeInfo.La *= 0.5;
            // cubeInfo.L *= 0.5;
            // cubeInfo.position = {3.3f, -4.5f, -3.3f};
            // lgtShdwCubeMgr().insert(dev, &cubeInfo);

            auto shdwCubMapInfo = Texture::MakeCubeMapTex(Texture::Shadow::MAP_CUBE_ARRAY_ID, SAMPLER::CLAMP_TO_BORDER_DEPTH,
                                                          512, static_cast<uint32_t>(lgtShdwCubeMgr().pItems.size()));
            textureHandler().make(&shdwCubMapInfo);
        }
    }
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
        auto proj = Camera::VULKAN_CLIP_MAT4 * glm::perspective(glm::radians(30.0f), 1.0f, 0.2f, 1000.0f);

        auto projector = helpers::getBias() * proj * view;
        uniDefPrjMgr().insert(dev, true, {{projector}});
    }

    // SCREEN SPACE
    {
        uniScrDefMgr().insert(dev);

        assert(shell().context().imageCount == 3);  // Potential imageCount problem

        Buffer::CreateInfo info = {shell().context().imageCount, false};
        strPstPrcMgr().insert(dev, &info);
    }

    // DEFERRED
    {
        uniDfrSSAOMgr().insert(dev);
        auto& ssaoKernel = uniDfrSSAOMgr().getTypedItem(0);
        ssaoKernel.init();
        update(ssaoKernel);
    }

    // SHADOW
    {
        // The values are const and set in the constructor.
        uniShdwDataMgr().insert(dev, true);
    }

    // GEOMETRY
    {
        // WIREFRAME
        { uniGeomDefMgr().insert(dev, true); }
    }
}

void Uniform::Handler::createTessellationData() {
    const auto& dev = shell().context().dev;

    // ARC
    {
        Tessellation::Default::DATA data = {};
        data.outerLevel[Tessellation::Bezier::STRIPS] = 1;
        data.outerLevel[Tessellation::Bezier::SEGMENTS] = 4;
        uniTessDefMgr().insert(dev, true, {data});
    }

    // TRIANGLE
    {
        Tessellation::Default::DATA data = {};
        data.outerLevel[0] = 3;
        data.outerLevel[1] = 3;
        data.outerLevel[2] = 3;
        data.innerLevel[0] = 4;
        uniTessDefMgr().insert(dev, true, {data});
    }
}

void Uniform::Handler::createVisualHelpers() {
    // DEFAULT POSITIONAL
    for (uint32_t i = 0; i < lgtDefPosMgr().pItems.size(); i++) {
        auto& lgt = lgtDefPosMgr().getTypedItem(i);
        meshHandler().makeModelSpaceVisualHelper(lgt);
        hasVisualHelpers_ = true;
    }
    // PBR POSITIONAL
    for (uint32_t i = 0; i < lgtPbrPosMgr().pItems.size(); i++) {
        auto& lgt = lgtPbrPosMgr().getTypedItem(i);
        meshHandler().makeModelSpaceVisualHelper(lgt);
        hasVisualHelpers_ = true;
    }
    // DEFAULT SPOT
    for (uint32_t i = 0; i < lgtDefSptMgr().pItems.size(); i++) {
        auto& lgt = lgtDefSptMgr().getTypedItem(i);
        meshHandler().makeModelSpaceVisualHelper(lgt);
        hasVisualHelpers_ = true;
    }
    // for (auto& light : posLights) {
    //    meshCreateInfo = {};
    //    meshCreateInfo.model = light.getModel();
    //    std::unique_ptr<LineMesh> pHelper = std::make_unique<VisualHelper>(&meshCreateInfo, 0.5f);
    //    SceneHandler::getActiveScene()->moveMesh(settings(), shell().context(), std::move(pHelper));
    //}
    // for (auto& light : spotLights) {
    //    meshCreateInfo = {};
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

    // GEOMETRY (WIREFRAME)
    auto& geomWireframe = uniGeomDefMgr().getTypedItem(0);
    geomWireframe.updateViewport(shell().context().extent);
    update(geomWireframe);
}

void Uniform::Handler::cycleCamera() {
    getActiveCamera().updateAllFrames();
    update(getActiveCamera());
    activeCameraOffset_ =
        (hasDebugCamera() && activeCameraOffset_ == mainCameraOffset_) ? debugCameraOffset_ : mainCameraOffset_;
}

void Uniform::Handler::moveToDebugCamera() {
    assert(hasDebugCamera());
    getMainCamera().takeCameraData(getDebugCamera());
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
    // All of the needed offsets in a list.
    auto resolvedOffsets = getBindingOffsets(pItems, offsets);
    // Check for enough uniforms for highest offset
    assert(*std::prev(resolvedOffsets.end()) < pItems.size());

    setResInfo.descCount = static_cast<uint32_t>(resolvedOffsets.size());
    setResInfo.bufferInfos.resize(resolvedOffsets.size() * setResInfo.uniqueDataSets);

    // Set the buffer infos
    uint32_t i = 0;
    for (const auto& offset : resolvedOffsets) {
        auto sMsg = Descriptor::GetPerframeBufferWarning(descType, pItems[offset]->BUFFER_INFO, setResInfo);
        if (sMsg.size()) shell().log(Shell::LogPriority::LOG_WARN, sMsg.c_str());
        pItems[offset]->setDescriptorInfo(setResInfo, i++);
    }
}

void Uniform::Handler::textReplaceShader(const Descriptor::Set::textReplaceTuples& replaceTuples,
                                         const std::string_view& fileName, std::string& text) const {
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
                    if (itemCount == -1) {
                        std::stringstream ss;
                        ss << "Could not find text to replace for: \"" << std::visit(Descriptor::GetTypeString{}, descType)
                           << "\" in file: \"" << fileName << "\".";
                        shell().log(Shell::LogPriority::LOG_WARN, ss.str().c_str());
                    }
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