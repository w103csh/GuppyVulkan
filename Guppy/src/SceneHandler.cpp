/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "SceneHandler.h"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

#include "Arc.h"
#include "Axes.h"
#include "Box.h"
#include "Model.h"
#include "Plane.h"
#include "Shell.h"
#include "Stars.h"
#include "Torus.h"
// HANDLERS
#include "InputHandler.h"
#include "MeshHandler.h"
#include "ModelHandler.h"
#include "ParticleHandler.h"
// Remove me after pass dependent things are no longer in the scene.
#include "RenderPassHandler.h"
#include "TextureHandler.h"
#include "UniformHandler.h"

Scene::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),  //
      cdlodDbgRenderer_(*this),
      activeSceneIndex_() {}

// Required in this file for inner-class forward declaration of SelectionManager
Scene::Handler::~Handler() = default;

void Scene::Handler::init() {
    reset();

    const auto& ctx = shell().context();

#if VK_USE_PLATFORM_MACOS_MVK
    bool suppress = false;
    bool deferred = false;
    bool particles = false;
    bool makeFaceSelectionMesh = false;
#else
    bool suppress = true;
    bool deferred = true;
    bool particles = true;
    bool makeFaceSelectionMesh = false;
#endif

    auto& pScene = makeScene(true, makeFaceSelectionMesh);

    // RENDERERS
    cdlodDbgRenderer_.init(nullptr, nullptr);

    if (deferred) {
        Mesh::Arc::CreateInfo arcInfo;
        Mesh::AxesCreateInfo axesInfo;
        Mesh::GenericCreateInfo colorInfo;
        Mesh::CreateInfo meshInfo;
        Mesh::Plane::CreateInfo planeInfo;
        Mesh::Torus::CreateInfo torusInfo;
        Model::CreateInfo modelInfo;
        Instance::Obj3d::CreateInfo instObj3dInfo;
        Material::Default::CreateInfo defMatInfo;
        Obj3d::BoundingBoxMinMax groundPlane_bbmm;

        // ORIGIN AXES
        if (!suppress || true) {
            axesInfo = {};
            axesInfo.pipelineType = GRAPHICS::DEFERRED_MRT_LINE;
            axesInfo.lineSize = 500.f;
            axesInfo.showNegative = true;
            instObj3dInfo = {};
            defMatInfo = {};
            defMatInfo.flags = Material::FLAG::PER_VERTEX_COLOR | Material::FLAG::MODE_FLAT_SHADE;
            auto offset = meshHandler().makeLineMesh<Mesh::Axes>(&axesInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addMeshIndex(MESH::LINE, offset);
        }

        // GROUND PLANE (COLOR)
        if (!suppress || false) {
            planeInfo = {};
            planeInfo.pipelineType = GRAPHICS::DEFERRED_MRT_COLOR;
            planeInfo.selectable = false;
            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3{2000.0f}, {}, -M_PI_2_FLT, CARDINAL_X)});
            // instObj3dInfo.data.push_back(
            //    {helpers::affine(glm::vec3{2000.0f}, {0.0f, -4.0f, 0.0f}, -M_PI_2_FLT, CARDINAL_X)});
            defMatInfo = {};
            defMatInfo.shininess = Material::SHININESS_MILDLY_SHINY;
            // defMatInfo.color = {0.0f, 1.0f, 0.0f};
            defMatInfo.color = {0.8f, 0.8f, 0.85f};
            // defMatInfo.color = {194.0f / 255.0f, 207.0f / 255.0f, 134.0f / 255.0f};
            auto& pGroundPlane = meshHandler().makeColorMesh<Mesh::Plane::Color>(&planeInfo, &defMatInfo, &instObj3dInfo);
            // auto& pGroundPlane = meshHandler().makeColorMesh<Mesh::Box::Color>(&meshInfo, &defMatInfo, &defInstInfo);
            auto offset = pGroundPlane->getOffset();
            pScene->addMeshIndex(MESH::COLOR, offset);
            groundPlane_bbmm = pGroundPlane->getBoundingBoxMinMax();
        }

        // GROUND PLANE (TEXTURE)
        if (!suppress && false) {
            planeInfo = {};
            planeInfo.pipelineType = GRAPHICS::DEFERRED_MRT_TEX;
            planeInfo.selectable = false;
            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3{500.0f}, {}, -M_PI_2_FLT, CARDINAL_X)});
            defMatInfo = {};
            defMatInfo.pTexture = textureHandler().getTexture(Texture::HARDWOOD_ID);
            defMatInfo.repeat = 800.0f;
            defMatInfo.specularCoeff *= 0.5f;
            defMatInfo.shininess = Material::SHININESS_EGGSHELL;
            auto& pGroundPlane =
                meshHandler().makeTextureMesh<Mesh::Plane::Texture>(&planeInfo, &defMatInfo, &instObj3dInfo);
            auto offset = pGroundPlane->getOffset();
            pScene->addMeshIndex(MESH::TEXTURE, offset);
            groundPlane_bbmm = pGroundPlane->getBoundingBoxMinMax();
        }

        // DEBUG CAMERA
        if (!suppress || uniformHandler().hasDebugCamera()) {
            const auto& debugCamera = uniformHandler().getDebugCamera();
            // Set the frustum mesh info.
            Mesh::Frustum::CreateInfo frustumMeshInfo = {};
            // frustumMeshInfo.pipelineType = GRAPHICS::DEFERRED_MRT_COLOR;
            frustumMeshInfo.pipelineType = GRAPHICS::DEFERRED_MRT_WF_COLOR;
            frustumMeshInfo.selectable = false;
            frustumMeshInfo.settings.geometryInfo.doubleSided = true;
            frustumMeshInfo.frustumInfo = debugCamera.getFrustumInfo();
            // Set the material info.
            defMatInfo = {};
            defMatInfo.shininess = Material::SHININESS_EGGSHELL;
            defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::MODE_FLAT_SHADE;
            defMatInfo.color = {0.0f, 1.0f, 0.0f};
            // Set the 3d object info, which in this case comes from the debug camera.
            instObj3dInfo = {};
            instObj3dInfo.data.push_back({debugCamera.getModel()});
            // Make the mesh and add it to the scene.
            auto& pFrustum = meshHandler().makeColorMesh<Mesh::Frustum::Base>(&frustumMeshInfo, &defMatInfo, &instObj3dInfo);
            pScene->debugFrustumOffset = pFrustum->getOffset();
            pScene->addMeshIndex(MESH::COLOR, pScene->debugFrustumOffset);
        }

        // SHADOW BOX
        if (!suppress || false) {
            meshInfo = {};
            meshInfo.pipelineType = GRAPHICS::DEFERRED_MRT_COLOR;
            meshInfo.selectable = false;
            meshInfo.settings.geometryInfo.doubleSided = true;
            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3(20.0f), glm::vec3(-0.0f, 0.5f, -2.0f))});
            defMatInfo = {};
            defMatInfo.shininess = Material::SHININESS_EGGSHELL;
            defMatInfo.color = {0.8f, 0.8f, 0.85f};
            auto& pGroundPlane = meshHandler().makeColorMesh<Mesh::Box::Color>(&meshInfo, &defMatInfo, &instObj3dInfo);
            auto offset = pGroundPlane->getOffset();
            pScene->addMeshIndex(MESH::COLOR, offset);
        }

        // SHADOW LIGHT INDICATORS
        if (!suppress || true) {
            axesInfo = {};
            axesInfo.pipelineType = GRAPHICS::DEFERRED_MRT_LINE;
            axesInfo.lineSize = 1.0f;
            axesInfo.showNegative = true;
            instObj3dInfo = {};
            for (uint32_t i = 0; i < static_cast<uint32_t>(uniformHandler().lgtShdwCubeMgr().pItems.size()); i++) {
                instObj3dInfo.data.push_back(
                    {helpers::affine(glm::vec3(0.5f), uniformHandler().lgtShdwCubeMgr().getTypedItem(i).getPosition())});
            }
            defMatInfo = {};
            defMatInfo.flags = Material::FLAG::PER_VERTEX_COLOR | Material::FLAG::MODE_FLAT_SHADE;
            pScene->posLgtCubeShdwOffset =
                meshHandler().makeLineMesh<Mesh::Axes>(&axesInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addMeshIndex(MESH::LINE, pScene->posLgtCubeShdwOffset);
        }

        // STARS
        if (!suppress || true) {
            meshInfo = {};
            // meshInfo.pipelineType = GRAPHICS::DEFERRED_MRT_PT;
            meshInfo.pipelineType = GRAPHICS::CUBE_MAP_PT;
            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3(1000.0f))});
            defMatInfo = {};
            defMatInfo.flags = Material::FLAG::PER_VERTEX_COLOR | Material::FLAG::MODE_FLAT_SHADE;
            pScene->starsOffset =
                meshHandler().makeColorMesh<Mesh::Stars>(&meshInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addMeshIndex(MESH::COLOR, pScene->starsOffset);
        }

        // MOON
        if (!suppress || true) {
            planeInfo = {};
            // planeInfo.pipelineType = GRAPHICS::DEFERRED_MRT_TEX;
            planeInfo.pipelineType = GRAPHICS::CUBE_MAP_TEX;
            planeInfo.planeInfo.width = 150.0f;
            planeInfo.planeInfo.height = 150.0f;

            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::moveAndRotateTo(
                {}, uniformHandler().lgtDefDirMgr().getTypedItem(0).direction * 1000.0f, UP_VECTOR)});

            defMatInfo = {};
            defMatInfo.flags |= Material::FLAG::MODE_FLAT_SHADE;
            defMatInfo.pTexture = textureHandler().getTexture(Texture::BRIGHT_MOON_ID);
            pScene->moonOffset =
                meshHandler().makeTextureMesh<Mesh::Plane::Texture>(&planeInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addMeshIndex(MESH::TEXTURE, pScene->moonOffset);
        }

        // SKYBOX
        if (!suppress || true) {
            meshInfo = {};
            meshInfo.pipelineType = GRAPHICS::DEFERRED_MRT_SKYBOX;
            meshInfo.selectable = false;
            meshInfo.settings.geometryInfo.reverseFaceWinding = true;
            instObj3dInfo = {};
            defMatInfo = {};
            defMatInfo.pTexture = textureHandler().getTexture(Texture::SKYBOX_NIGHT_ID);
            auto& skybox = meshHandler().makeColorMesh<Mesh::Box::Color>(&meshInfo, &defMatInfo, &instObj3dInfo);
            auto offset = skybox->getOffset();
            pScene->addMeshIndex(MESH::COLOR, offset);
        }

        // PLAIN OLD NON-TRANSFORMED PLANE (TEXTURE)
        if (!suppress && false) {
            planeInfo = {};
            planeInfo.pipelineType = GRAPHICS::DEFERRED_MRT_TEX;
            planeInfo.selectable = true;
            planeInfo.settings.geometryInfo.doubleSided = true;
            instObj3dInfo = {};
            // defInstInfo.data.push_back({helpers::affine(glm::vec3{5.0f})});
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, {-2.0f, 1.0f, 2.0f})});
            defMatInfo = {};
            // defMatInfo.pTexture = textureHandler().getTexture(Texture::NEON_BLUE_TUX_GUPPY_ID);
            defMatInfo.pTexture = textureHandler().getTexture(Texture::FIRE_ID);
            auto offset =
                meshHandler().makeTextureMesh<Mesh::Plane::Texture>(&planeInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addMeshIndex(MESH::TEXTURE, offset);
        }

        // WAVE
        if (!suppress || false) {
            if (particles) {
                planeInfo = {};
                // planeInfo.pipelineType = GRAPHICS::DEFERRED_MRT_WF_COLOR;
                planeInfo.pipelineType = GRAPHICS::PRTCL_WAVE_DEFERRED;
                planeInfo.selectable = true;
                planeInfo.settings.geometryInfo.faceVertexColorsRGB = true;
                // planeInfo.settings.indexVertices = false;
                planeInfo.settings.geometryInfo.doubleSided = true;
                planeInfo.settings.geometryInfo.transform =
                    helpers::affine(glm::vec3{5.0f, 1.0f, 1.0f}, {4.0f, 2.0f, 2.0f}, -M_PI_2_FLT, CARDINAL_X);
                planeInfo.planeInfo.horzDivs = 200;
                planeInfo.planeInfo.vertDivs = 1;
                instObj3dInfo = {};
                defMatInfo = {};
                defMatInfo.color = {0.5f, 0.25f, 0.75f};
                // defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR;
                // defMatInfo.pTexture = textureHandler().getTexture(Texture::FIRE_ID);
                auto offset =
                    meshHandler().makeColorMesh<Mesh::Plane::Color>(&planeInfo, &defMatInfo, &instObj3dInfo)->getOffset();
                pScene->addMeshIndex(MESH::COLOR, offset);
            }
        }

        // TRIANGLE
        if ((!suppress && false) && ctx.tessellationShadingEnabled) {
            instObj3dInfo = {};
            defMatInfo = {};
            defMatInfo.flags = Material::FLAG::PER_VERTEX_COLOR;

            // GRAPHICS::TRI_LIST_COLOR is the default
            colorInfo.name = "Tessellated Triangle Test";
            colorInfo.pipelineType = GRAPHICS::TESS_PHONG_TRI_COLOR_WF_DEFERRED;

            colorInfo.faces.push_back({});
            colorInfo.faces.back()[0].position = {0.5, 2.0f, 0.0f};
            colorInfo.faces.back()[0].color = COLOR_RED;
            colorInfo.faces.back()[1].position = {2.0, 4.0f, 0.0f};
            colorInfo.faces.back()[1].color = COLOR_GREEN;
            colorInfo.faces.back()[2].position = {3.5, 2.0f, 0.0f};
            colorInfo.faces.back()[2].color = COLOR_BLUE;

            auto offset = meshHandler().makeColorMesh<Mesh::Color>(&colorInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addMeshIndex(MESH::COLOR, offset);
        }

        // BURNT ORANGE TORUS
        if (!suppress && false) {
            torusInfo = {};
            torusInfo.pipelineType = GRAPHICS::DEFERRED_MRT_COLOR;
            // INSTANCE
            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3{0.07f}, glm::vec3{-1.0f, 2.0f, -1.0f})});
            // MATERIAL
            defMatInfo = {};
            defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR;
            defMatInfo.ambientCoeff = {0.8f, 0.3f, 0.0f};
            defMatInfo.color = {0.8f, 0.3f, 0.0f};
            auto offset =
                meshHandler().makeColorMesh<Mesh::Torus::Color>(&torusInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addMeshIndex(MESH::COLOR, offset);
        }

        // ICOSAHEDRON
        if (!suppress && false) {  // TODO: GRAPHICS::TESS_PHONG_TRI_COLOR_DEFERRED This requires two dynamic uniforms now!
            modelInfo = {};
            modelInfo.async = false;
            modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
            modelInfo.modelPath = ICOSAHEDRON_MODEL_PATH;
            // modelInfo.pipelineType = GRAPHICS::DEFERRED_MRT_COLOR;
            // This doesn't work right... Need to learn about cubic bezier trianlges/surfaces.
            modelInfo.pipelineType = GRAPHICS::TESS_PHONG_TRI_COLOR_DEFERRED;
            modelInfo.selectable = true;
            modelInfo.settings.geometryInfo.smoothNormals = true;
            modelInfo.settings.geometryInfo.faceVertexColorsRGB = true;

            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3{2.0f}, {1.0, 0.0, -4.0}, -M_PI_2_FLT, CARDINAL_X)});

            defMatInfo = {};
            defMatInfo.flags = Material::FLAG::PER_VERTEX_COLOR;

            modelInfo.settings.geometryInfo.smoothNormals = true;
            auto offset = modelHandler().makeColorModel(&modelInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addModelIndex(offset);

            // second one
            {
                modelInfo.pipelineType = GRAPHICS::DEFERRED_MRT_COLOR;
                instObj3dInfo = {};
                instObj3dInfo.data.push_back({helpers::affine(glm::vec3{2.0f}, {-1.0, 0.0, 0.0}, -M_PI_2_FLT, CARDINAL_X)});

                modelInfo.settings.geometryInfo.smoothNormals = false;
                offset = modelHandler().makeColorModel(&modelInfo, &defMatInfo, &instObj3dInfo)->getOffset();
                pScene->addModelIndex(offset);
            }
        }

        // ARC
        if ((!suppress || false) && ctx.tessellationShadingEnabled) {
            arcInfo = {};
            instObj3dInfo = {};
            defMatInfo = {};
            defMatInfo.flags |= Material::FLAG::MODE_FLAT_SHADE;
            defMatInfo.color = {0.2f, 0.4f, 0.8f};
            // defMatInfo.flags = Material::FLAG::PER_VERTEX_COLOR;

            // GRAPHICS::DEFERRED_BEZIER_4 is the default
            arcInfo.controlPoints.push_back({{2.0f, 1.0f, 0.0f}});                  // p0 (start)
            arcInfo.controlPoints.push_back({{2.0f, 1.0f, 2.0f * (2.0f / 3.0f)}});  // p1
            arcInfo.controlPoints.push_back({{2.0f * (2.0f / 3.0f), 1.0f, 2.0f}});  // p2
            arcInfo.controlPoints.push_back({{0.0f, 1.0f, 2.0f}});                  // p3 (end)
            auto offset = meshHandler().makeLineMesh<Mesh::Arc>(&arcInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addMeshIndex(MESH::LINE, offset);

            arcInfo.controlPoints.clear();
            arcInfo.controlPoints.push_back({{2.0f, 1.0f, 0.0f}});                   // p0 (start)
            arcInfo.controlPoints.push_back({{2.0f, 1.0f, -2.0f * (2.0f / 3.0f)}});  // p1
            arcInfo.controlPoints.push_back({{4.0f, 1.0f, -2.0f * (2.0f / 3.0f)}});  // p1
            arcInfo.controlPoints.push_back({{4.0f, 1.0f, 0.0f}});                   // p3 (end)
            offset = meshHandler().makeLineMesh<Mesh::Arc>(&arcInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addMeshIndex(MESH::LINE, offset);
        }

        // BOX
        if (!suppress || false) {
            if (true) {
                meshInfo = {};
                meshInfo.pipelineType = GRAPHICS::DEFERRED_MRT_COLOR;
                instObj3dInfo = {};
                bool shadowTest = true;
                // float factorScale = 150.0f, factorTranslate = 100.0f;
                float factorScale = 1.0f, factorTranslate = 1.0f;
                if (shadowTest) {
                    instObj3dInfo.data.push_back(
                        {helpers::affine(factorScale * glm::vec3{1.0f}, factorTranslate * glm::vec3{-2.0f, 0.0f, 1.5f})});
                    instObj3dInfo.data.push_back(
                        {helpers::affine(factorScale * glm::vec3{1.5f}, factorTranslate * glm::vec3{-2.0f, 0.0f, -2.5f})});
                    instObj3dInfo.data.push_back(
                        {helpers::affine(factorScale * glm::vec3{0.6f}, factorTranslate * glm::vec3{0.0f, 2.0f, 0.0f})});
                    instObj3dInfo.data.push_back(
                        {helpers::affine(factorScale * glm::vec3{1.6f}, factorTranslate * glm::vec3{0.0f, 3.0f, -3.5f})});

                    instObj3dInfo.data.push_back(
                        {helpers::affine(factorScale * glm::vec3{0.3f}, factorTranslate * glm::vec3{5.0f, 5.0f, 0.0f})});
                    instObj3dInfo.data.push_back(
                        {helpers::affine(factorScale * glm::vec3{2.0f}, factorTranslate * glm::vec3{7.0f, -1.0f, -0.4f})});
                    instObj3dInfo.data.push_back(
                        {helpers::affine(factorScale * glm::vec3{1.6f}, factorTranslate * glm::vec3{1.0f, -4.0f, 6.5f})});
                    instObj3dInfo.data.push_back(
                        {helpers::affine(factorScale * glm::vec3{0.4f}, factorTranslate * glm::vec3{3.0f, -5.0f, -4.5f})});
                    instObj3dInfo.data.push_back(
                        {helpers::affine(factorScale * glm::vec3{1.2f}, factorTranslate * glm::vec3{-3.0f, 5.0f, -1.5f})});
                } else {
                    instObj3dInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{0.0f, 0.0f, 0.0f}, M_PI_2_FLT,
                                                                  glm::vec3{1.0f, 0.0f, 1.0f})});
                }
                defMatInfo = {};
                defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR;
                defMatInfo.color = {0.0f, 0.0f, 1.0f};
                auto& boxColor = meshHandler().makeColorMesh<Mesh::Box::Color>(&meshInfo, &defMatInfo, &instObj3dInfo);
                auto offset = boxColor->getOffset();
                pScene->addMeshIndex(MESH::COLOR, offset);
                if (!shadowTest) {
                    boxColor->putOnTop(groundPlane_bbmm);
                    meshHandler().updateMesh(boxColor);
                }
            }
            if (false && ctx.geometryShadingEnabled) {
                meshInfo = {};
                meshInfo.pipelineType = GRAPHICS::DEFERRED_MRT_WF_COLOR;
                instObj3dInfo = {};
                instObj3dInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{-2.0f, 0.0f, -2.0f}, M_PI_2_FLT,
                                                              glm::vec3{1.0f, 0.0f, 1.0f})});
                defMatInfo = {};
                defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR;
                defMatInfo.color = {0.0f, 1.0f, 1.0f};
                auto& boxColor = meshHandler().makeColorMesh<Mesh::Box::Color>(&meshInfo, &defMatInfo, &instObj3dInfo);
                auto offset = boxColor->getOffset();
                pScene->addMeshIndex(MESH::COLOR, offset);
                boxColor->putOnTop(groundPlane_bbmm);
                meshHandler().updateMesh(boxColor);
            }
            // SKYBOX NIGHT
            if (false) {
                meshInfo = {};
                // meshInfo.pipelineType = GRAPHICS::DEFERRED_MRT_TEX;
                meshInfo.pipelineType = GRAPHICS::CUBE_MAP_TEX;
                meshInfo.settings.geometryInfo.doubleSided = true;
                instObj3dInfo = {};
                // instObj3dInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{-3.5f, 0.0f, 0.5f}, M_PI_2_FLT,
                //                                              glm::vec3{1.0f, 0.0f, 1.0f})});
                instObj3dInfo.data.push_back({helpers::affine(glm::vec3{900.0f})});
                defMatInfo = {};
                defMatInfo.flags = Material::FLAG::PER_TEXTURE_COLOR | Material::FLAG::MODE_FLAT_SHADE;
                defMatInfo.pTexture = textureHandler().getTexture(Texture::STATUE_ID);
                auto& boxColor = meshHandler().makeTextureMesh<Mesh::Box::Texture>(&meshInfo, &defMatInfo, &instObj3dInfo);
                auto offset = boxColor->getOffset();
                pScene->moonOffset = offset;
                pScene->addMeshIndex(MESH::TEXTURE, offset);

                meshInfo.pipelineType = GRAPHICS::DEFERRED_MRT_TEX;
                offset =
                    meshHandler().makeTextureMesh<Mesh::Box::Texture>(&meshInfo, &defMatInfo, &instObj3dInfo)->getOffset();
                pScene->addMeshIndex(MESH::TEXTURE, offset);
                // boxColor->putOnTop(groundPlane_bbmm);
                // meshHandler().updateMesh(boxColor);
            }
            // REFLECT REFRACT
            if (false) {
                meshInfo = {};
                meshInfo.pipelineType = GRAPHICS::DEFERRED_MRT_COLOR_RFL_RFR;

                instObj3dInfo = {};
                // instObj3dInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{0.0f, 7.0f, 0.0f}, M_PI_2_FLT,
                //                                              glm::vec3{1.0f, 0.0f, 1.0f})});
                instObj3dInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{0.0f, 1.0f, -1.0f})});

                defMatInfo = {};
                // defMatInfo.color = {0x2E / 255.0f, 0x40 / 255.0f, 0x53 / 255.0f};
                defMatInfo.color = COLOR_RED;
                defMatInfo.reflectionFactor = 0.85f;
                defMatInfo.flags |= Material::FLAG::REFLECT | Material::FLAG::REFRACT;
                defMatInfo.pTexture = textureHandler().getTexture(Texture::SKYBOX_NIGHT_ID);
                defMatInfo.flags |= Material::FLAG::MODE_FLAT_SHADE;

                auto offset =
                    meshHandler().makeColorMesh<Mesh::Box::Color>(&meshInfo, &defMatInfo, &instObj3dInfo)->getOffset();
                pScene->addMeshIndex(MESH::COLOR, offset);
            }
        }

        // PIG
        if (!suppress || false) {
            // MODEL
            modelInfo = {};
            if (false && ctx.geometryShadingEnabled) {
                modelInfo.pipelineType = GRAPHICS::GEOMETRY_SILHOUETTE_DEFERRED;
                modelInfo.settings.needAdjacenyList = true;
            } else {
                modelInfo.pipelineType = GRAPHICS::DEFERRED_MRT_COLOR;
            }
            modelInfo.async = false;
            modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
            modelInfo.modelPath = PIG_MODEL_PATH;
            modelInfo.settings.geometryInfo.smoothNormals = true;
            // INSTANCE
            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3{3.0f}, {-2.0f, 0.0f, 4.0f})});
            // MATERIAL
            defMatInfo = {};
            defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::MODE_BLINN_PHONG;
            defMatInfo.ambientCoeff = {0.8f, 0.8f, 0.8f};
            defMatInfo.color = {0.8f, 0.8f, 0.8f};
            auto offset = modelHandler().makeColorModel(&modelInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addModelIndex(offset);
        }

        // DRAGON
        if (!suppress && false) {
            // MODEL
            modelInfo = {};
            modelInfo.async = false;
            modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
            modelInfo.modelPath = DRAGON_MODEL_PATH;
            modelInfo.settings.geometryInfo.smoothNormals = true;
            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3{0.07f}, {}, -M_PI_2_FLT, CARDINAL_X)});
            modelInfo.pipelineType = GRAPHICS::DEFERRED_MRT_COLOR;
            defMatInfo = {};
            defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR;
            // defMatInfo.ambientCoeff = {0.8f, 0.3f, 0.0f};
            // defMatInfo.color = {0.8f, 0.3f, 0.0f};
            // defMatInfo.ambientCoeff = glm::vec3{0.2f};
            // defMatInfo.specularCoeff = glm::vec3{1.0f};
            // defMatInfo.color = {0.9f, 0.3f, 0.2f};
            // defMatInfo.shininess = 25.0f;
            auto offset = modelHandler().makeColorModel(&modelInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addModelIndex(offset);
        }

        uint32_t count = 5;
        // MEDIEVAL HOUSE (TRI_COLOR_TEX)
        if (!suppress && false) {
            modelInfo = {};
            modelInfo.pipelineType = GRAPHICS::DEFERRED_MRT_TEX;
            modelInfo.async = false;
            modelInfo.modelPath = MED_H_MODEL_PATH;
            modelInfo.settings.geometryInfo.smoothNormals = false;
            modelInfo.settings.doVisualHelper = false;  // true;
            instObj3dInfo = {};
            instObj3dInfo.data.reserve(static_cast<size_t>(count) * static_cast<size_t>(count));
            for (uint32_t i = 0; i < count; i++) {
                float x = -6.5f - (static_cast<float>(i) * 6.5f);
                for (uint32_t j = 0; j < count; j++) {
                    float z = -(static_cast<float>(j) * 6.5f);
                    instObj3dInfo.data.push_back(
                        {helpers::affine(glm::vec3{0.0175f}, {x, 0.0f, z}, M_PI_4_FLT, CARDINAL_Y)});
                    instObj3dInfo.data.push_back(
                        {helpers::affine(glm::vec3{0.0175f}, {x, 0.0f, -z}, M_PI_4_FLT, CARDINAL_Y)});
                }
            }
            // MATERIAL
            defMatInfo = {};
            defMatInfo.pTexture = textureHandler().getTexture(Texture::MEDIEVAL_HOUSE_ID);
            auto offset = modelHandler().makeTextureModel(&modelInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addModelIndex(offset);
        }

    } else {
        // Create info structs
        Mesh::AxesCreateInfo axesInfo;
        Mesh::CreateInfo meshInfo;
        Model::CreateInfo modelInfo;
        Mesh::Plane::CreateInfo planeInfo;
        Instance::Obj3d::CreateInfo instObj3dInfo;
        Material::Default::CreateInfo defMatInfo;
        Material::PBR::CreateInfo pbrMatInfo;
        Obj3d::BoundingBoxMinMax groundPlane_bbmm;
        uint32_t count = 4;

        // ORIGIN AXES
        if (!suppress || false) {
            axesInfo = {};
            axesInfo.lineSize = 500.f;
            axesInfo.showNegative = true;
            instObj3dInfo = {};
            defMatInfo = {};
            defMatInfo.shininess = 23.123f;
            auto offset = meshHandler().makeLineMesh<Mesh::Axes>(&axesInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addMeshIndex(MESH::LINE, offset);
        }

        // GROUND PLANE (TEXTURE)
        if (!suppress || false) {
            planeInfo = {};
            planeInfo.pipelineType = GRAPHICS::TRI_LIST_TEX;
            planeInfo.selectable = false;
            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3{500.0f}, {}, -M_PI_2_FLT, CARDINAL_X)});
            defMatInfo = {};
            defMatInfo.pTexture = textureHandler().getTexture(Texture::HARDWOOD_ID);
            defMatInfo.repeat = 800.0f;
            defMatInfo.specularCoeff *= 0.5f;
            defMatInfo.shininess = Material::SHININESS_EGGSHELL;
            auto& pGroundPlane =
                meshHandler().makeTextureMesh<Mesh::Plane::Texture>(&planeInfo, &defMatInfo, &instObj3dInfo);
            auto offset = pGroundPlane->getOffset();
            pScene->addMeshIndex(MESH::TEXTURE, offset);
            groundPlane_bbmm = pGroundPlane->getBoundingBoxMinMax();
        }

        // GROUND PLANE (COLOR)
        if (!suppress && false) {
            planeInfo = {};
            planeInfo.pipelineType = GRAPHICS::TRI_LIST_COLOR;
            planeInfo.selectable = false;
            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3{2000.0f}, {}, -M_PI_2_FLT, CARDINAL_X)});
            defMatInfo = {};
            defMatInfo.shininess = Material::SHININESS_EGGSHELL;
            defMatInfo.color = {0.5f, 0.5f, 0.5f};
            auto& pGroundPlane = meshHandler().makeColorMesh<Mesh::Plane::Color>(&planeInfo, &defMatInfo, &instObj3dInfo);
            auto offset = pGroundPlane->getOffset();
            pScene->addMeshIndex(MESH::COLOR, offset);
            groundPlane_bbmm = pGroundPlane->getBoundingBoxMinMax();
        }

        // PLAIN OLD NON-TRANSFORMED PLANE (TEXTURE)
        if (!suppress && false) {
            planeInfo = {};
            planeInfo.pipelineType = GRAPHICS::TRI_LIST_TEX;
            planeInfo.selectable = true;
            instObj3dInfo = {};
            defMatInfo = {};
            defMatInfo.pTexture = textureHandler().getTexture(Texture::VULKAN_ID);
            auto offset =
                meshHandler().makeTextureMesh<Mesh::Plane::Texture>(&planeInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addMeshIndex(MESH::TEXTURE, offset);
        }

        // PROJECT PLANE (I AM PASS DEPENDENT AND SHOULDN'T BE HERE!!!! REMOVE INCLUDE IF YOU CAN.)
        if (!suppress && (DO_PROJECTOR || false)) {
            std::set<PASS> activePassTypes;
            passHandler().getActivePassTypes(activePassTypes);
            // TODO: these types of checks are not great. The active passes should be able to change at runtime.
            if (std::find(activePassTypes.begin(), activePassTypes.end(), PASS::SAMPLER_PROJECT) != activePassTypes.end()) {
                planeInfo = {};
                planeInfo.pipelineType = GRAPHICS::TRI_LIST_TEX;
                planeInfo.selectable = false;
                instObj3dInfo = {};
                instObj3dInfo.data.push_back({helpers::affine(glm::vec3{2.0f}, glm::vec3{0.5f, 3.0f, 0.5f})});
                // defInstInfo.data.push_back(
                //    {helpers::affine(glm::vec3{5.0f}, glm::vec3{10.5f, 10.0f, 10.5f}, -M_PI_2_FLT, CARDINAL_Y)});
                // defInstInfo.data.push_back(
                //    {helpers::affine(glm::vec3{2.0f}, glm::vec3{4.5f, -5.0f, 13.5f}, -M_PI_2_FLT, CARDINAL_Z)});
                defMatInfo = {};
                defMatInfo.shininess = Material::SHININESS_EGGSHELL;
                defMatInfo.pTexture = textureHandler().getTexture(RenderPass::PROJECT_2D_ARRAY_TEXTURE_ID);
                defMatInfo.color = {0.5f, 0.5f, 0.5f};
                auto offset = meshHandler()
                                  .makeTextureMesh<Mesh::Plane::Texture>(&planeInfo, &defMatInfo, &instObj3dInfo)
                                  ->getOffset();
                pScene->addMeshIndex(MESH::TEXTURE, offset);
            }
        }

        // SKYBOX
        if (!suppress || false) {
            meshInfo = {};
            meshInfo.pipelineType = GRAPHICS::CUBE;
            meshInfo.selectable = false;
            meshInfo.settings.geometryInfo.reverseFaceWinding = true;
            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3{10.0f})});
            defMatInfo = {};
            defMatInfo.flags |= Material::FLAG::SKYBOX;
            auto& skybox = meshHandler().makeColorMesh<Mesh::Box::Color>(&meshInfo, &defMatInfo, &instObj3dInfo);
            auto offset = skybox->getOffset();
            pScene->addMeshIndex(MESH::COLOR, offset);
        }

        // BOX (TEXTURE)
        if (!suppress && false) {
            /**
             * THIS NO LONGER WORKS BECAUSE OF CHANGES MADE TO THE DYNAMIC UNIFORM BIND DATA RETRIEVAL PROCESS.
             *
             *  This is because DESCRIPTOR_SET::SAMPLER_PARALLAX uses two COMBINED_SAMPLER::MATERIAL's in the set
             * create info. This doesn't really make sense and was a bad workaround at the time. The real fix for this
             * problem is to completely decouple textures from materials once and for all. The coupling was a bad assumption
             * made from my time working at Autodesk about how materials and textures should have a 1 to 1 relationship. This
             * is obvisouly not the case.
             */

            meshInfo = {};
            // meshInfo.pipelineType = GRAPHICS::PARALLAX_SIMPLE;
            meshInfo.pipelineType = GRAPHICS::PARALLAX_STEEP;
            // meshInfo.pipelineType = GRAPHICS::PBR_TEX;
            instObj3dInfo = {};
            instObj3dInfo.data.push_back(
                {helpers::affine(glm::vec3{1.0f}, glm::vec3{0.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
            defMatInfo = {};
            defMatInfo.pTexture = textureHandler().getTexture(Texture::MYBRICK_ID);
            defMatInfo.shininess = Material::SHININESS_EGGSHELL;
            auto& boxTexture1 = meshHandler().makeTextureMesh<Mesh::Box::Texture>(&meshInfo, &defMatInfo, &instObj3dInfo);
            auto offset = boxTexture1->getOffset();
            pScene->addMeshIndex(MESH::TEXTURE, offset);
            boxTexture1->putOnTop(groundPlane_bbmm);
            meshHandler().updateMesh(boxTexture1);
            meshHandler().makeTangentSpaceVisualHelper(boxTexture1.get());
        }
        // BOX (PBR_TEX)
        if (!suppress || false) {
            meshInfo = {};
            meshInfo.pipelineType = GRAPHICS::PBR_TEX;
            // meshInfo.model = helpers::affine(glm::vec3{1.0f}, glm::vec3{1.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f,
            // 0.0f, 1.0f});
            instObj3dInfo = {};
            instObj3dInfo.data.push_back(
                {helpers::affine(glm::vec3{1.0f}, glm::vec3{1.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
            pbrMatInfo = {};
            pbrMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::METAL;
            pbrMatInfo.color = {1.0f, 1.0f, 0.0f};
            pbrMatInfo.roughness = 0.86f;  // 0.43f;
            pbrMatInfo.pTexture = textureHandler().getTexture(Texture::VULKAN_ID);
            auto& boxTexture2 = meshHandler().makeTextureMesh<Mesh::Box::Texture>(&meshInfo, &pbrMatInfo, &instObj3dInfo);
            auto offset = boxTexture2->getOffset();
            pScene->addMeshIndex(MESH::TEXTURE, offset);
            boxTexture2->putOnTop(groundPlane_bbmm);
            meshHandler().updateMesh(boxTexture2);
            meshHandler().makeTangentSpaceVisualHelper(boxTexture2.get());
        }
        // BOX (PBR_COLOR)
        if (!suppress || false) {
            meshInfo = {};
            meshInfo.pipelineType = GRAPHICS::PBR_COLOR;
            // meshInfo.model = helpers::affine(glm::vec3{1.0f}, glm::vec3{2.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f,
            // 0.0f, 1.0f});
            instObj3dInfo = {};
            instObj3dInfo.data.push_back(
                {helpers::affine(glm::vec3{1.0f}, glm::vec3{2.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
            pbrMatInfo = {};
            pbrMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR;
            pbrMatInfo.color = {1.0f, 1.0f, 0.0f};
            auto& boxPbrColor = meshHandler().makeColorMesh<Mesh::Box::Color>(&meshInfo, &pbrMatInfo, &instObj3dInfo);
            auto offset = boxPbrColor->getOffset();
            pScene->addMeshIndex(MESH::COLOR, offset);
            boxPbrColor->putOnTop(groundPlane_bbmm);
            meshHandler().updateMesh(boxPbrColor);
        }
        // BOX (TRI_LIST_COLOR)
        if (!suppress || false) {
            meshInfo = {};
            meshInfo.pipelineType = GRAPHICS::TRI_LIST_COLOR;
            // meshInfo.model = helpers::affine(glm::vec3{1.0f}, glm::vec3{5.0f, 0.0f, 3.5f}, M_PI_2_FLT, glm::vec3{-1.0f,
            // 0.0f, 1.0f});
            // meshInfo.model =
            //    helpers::affine(glm::vec3{1.0f}, glm::vec3{-1.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f});
            instObj3dInfo = {};
            instObj3dInfo.data.push_back(
                {helpers::affine(glm::vec3{1.0f}, glm::vec3{-1.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
            defMatInfo = {};
            defMatInfo.color = {1.0f, 1.0f, 0.0f};
            auto& boxDefColor1 = meshHandler().makeColorMesh<Mesh::Box::Color>(&meshInfo, &defMatInfo, &instObj3dInfo);
            auto offset = boxDefColor1->getOffset();
            pScene->addMeshIndex(MESH::COLOR, offset);
            boxDefColor1->putOnTop(groundPlane_bbmm);
            meshHandler().updateMesh(boxDefColor1);
        }
        // BOX (CUBE)
        if (!suppress || false) {
            meshInfo = {};
            instObj3dInfo = {};
            instObj3dInfo.data.push_back(
                {helpers::affine(glm::vec3{1.0f}, glm::vec3{0.0f, 7.0f, 0.0f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
            defMatInfo = {};
            // defMatInfo.color = {0x2E / 255.0f, 0x40 / 255.0f, 0x53 / 255.0f};
            defMatInfo.color = COLOR_RED;
            if (false) {  // reflect defaults
                defMatInfo.flags |= Material::FLAG::REFLECT;
                defMatInfo.eta = 0.94f;
                defMatInfo.reflectionFactor = 0.85f;
            } else {  // reflect defaults
                defMatInfo.flags |= Material::FLAG::REFLECT | Material::FLAG::REFRACT;
                defMatInfo.eta = 0.94f;
                defMatInfo.reflectionFactor = 0.1f;
            }
            meshInfo.pipelineType = GRAPHICS::CUBE;
            defMatInfo.pTexture = textureHandler().getTexture(Texture::SKYBOX_ID);
            auto offset = meshHandler().makeColorMesh<Mesh::Box::Color>(&meshInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addMeshIndex(MESH::COLOR, offset);
        }

        // BURNT ORANGE TORUS
        if (!suppress || false) {
            // MODEL
            modelInfo = {};
            modelInfo.async = false;
            modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
            modelInfo.modelPath = TORUS_MODEL_PATH;
            modelInfo.settings.geometryInfo.smoothNormals = true;
            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3{0.07f})});
            // MATERIAL
            if (false) {
                modelInfo.pipelineType = GRAPHICS::PBR_COLOR;
                pbrMatInfo = {};
                pbrMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::METAL;
                pbrMatInfo.color = {0.8f, 0.3f, 0.0f};
                pbrMatInfo.roughness = 0.9f;
                auto offset = modelHandler().makeColorModel(&modelInfo, &pbrMatInfo, &instObj3dInfo)->getOffset();
                pScene->addModelIndex(offset);
            } else if (true) {
                modelInfo.pipelineType = GRAPHICS::TRI_LIST_COLOR;
                defMatInfo = {};
                defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR;
                defMatInfo.ambientCoeff = {0.8f, 0.3f, 0.0f};
                defMatInfo.color = {0.8f, 0.3f, 0.0f};
                // defMatInfo.ambientCoeff = glm::vec3{0.2f};
                // defMatInfo.specularCoeff = glm::vec3{1.0f};
                // defMatInfo.color = {0.9f, 0.3f, 0.2f};
                // defMatInfo.shininess = 25.0f;
                auto offset = modelHandler().makeColorModel(&modelInfo, &defMatInfo, &instObj3dInfo)->getOffset();
                pScene->addModelIndex(offset);
            } else {
                modelInfo.pipelineType = GRAPHICS::CUBE;
                defMatInfo = {};
                pbrMatInfo.color = {0.8f, 0.3f, 0.0f};
                defMatInfo.flags |= Material::FLAG::REFLECT;
                defMatInfo.reflectionFactor = 0.85f;
                defMatInfo.pTexture = textureHandler().getTexture(Texture::SKYBOX_ID);
                auto offset = modelHandler().makeColorModel(&modelInfo, &defMatInfo, &instObj3dInfo)->getOffset();
                pScene->addModelIndex(offset);
            }
        }

        // DRAGON
        if (!suppress && false) {
            // MODEL
            modelInfo = {};
            modelInfo.async = false;
            modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
            modelInfo.modelPath = DRAGON_MODEL_PATH;
            modelInfo.settings.geometryInfo.smoothNormals = false;
            instObj3dInfo = {};
            // defInstInfo.data.push_back({helpers::affine(glm::vec3{0.07f})});
            modelInfo.pipelineType = GRAPHICS::TRI_LIST_COLOR;
            defMatInfo = {};
            defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR;
            // defMatInfo.ambientCoeff = {0.8f, 0.3f, 0.0f};
            // defMatInfo.color = {0.8f, 0.3f, 0.0f};
            // defMatInfo.ambientCoeff = glm::vec3{0.2f};
            // defMatInfo.specularCoeff = glm::vec3{1.0f};
            // defMatInfo.color = {0.9f, 0.3f, 0.2f};
            // defMatInfo.shininess = 25.0f;
            auto offset = modelHandler().makeColorModel(&modelInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addModelIndex(offset);
        }

        count = 100;
        // GRASS
        if (!suppress || false) {
            // MODEL
            modelInfo = {};
            modelInfo.pipelineType = GRAPHICS::BP_TEX_CULL_NONE;
            modelInfo.async = false;
            // modelInfo.callback = [groundPlane_bbmm](auto pModel) {};
            modelInfo.settings.doVisualHelper = false;
            modelInfo.modelPath = GRASS_LP_MODEL_PATH;
            modelInfo.settings.geometryInfo.smoothNormals = false;
            instObj3dInfo = {};
            instObj3dInfo.update = false;
            std::srand(static_cast<unsigned>(time(0)));
            float x, z;
            float f1 = 0.03f;
            // float f1 = 0.2f;
            float f2 = f1 * 5.0f;
            // float low = f2 * 0.05f;
            // float high = f2 * 3.0f;
            instObj3dInfo.data.reserve(static_cast<size_t>(count) * static_cast<size_t>(count) * 4);
            for (uint32_t i = 0; i < count; i++) {
                x = static_cast<float>(i) * f2;
                // x += low + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (high - low)));
                for (uint32_t j = 0; j < count; j++) {
                    z = static_cast<float>(j) * f2;
                    // z += low + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (high - low)));
                    auto scale = glm::vec3{f1};
                    auto translate1 = glm::vec3{x, 0.0f, z};
                    auto translate2 = glm::vec3{x, 0.0f, -z};
                    instObj3dInfo.data.push_back({helpers::affine(scale, translate1)});
                    if (i == 0 && j == 0) continue;
                    instObj3dInfo.data.push_back({helpers::affine(scale, -translate1)});
                    if (i == 0) continue;
                    instObj3dInfo.data.push_back({helpers::affine(scale, translate2)});
                    instObj3dInfo.data.push_back({helpers::affine(scale, -translate2)});
                }
            }
            // MATERIAL
            defMatInfo = {};
            auto offset = modelHandler().makeTextureModel(&modelInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addModelIndex(offset);
        }

        count = 5;
        // ORANGE
        if (!suppress || false) {
            // MODEL
            modelInfo = {};
            modelInfo.pipelineType = GRAPHICS::TRI_LIST_TEX;
            modelInfo.async = false;
            modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
            modelInfo.settings.doVisualHelper = true;
            modelInfo.modelPath = ORANGE_MODEL_PATH;
            modelInfo.settings.geometryInfo.smoothNormals = true;
            instObj3dInfo = {};
            instObj3dInfo.update = false;
            instObj3dInfo.data.reserve(static_cast<size_t>(count) * static_cast<size_t>(count));
            for (uint32_t i = 0; i < count; i++) {
                float x = 4.0f + (static_cast<float>(i) * 2.0f);
                for (uint32_t j = 0; j < count; j++) {
                    float z = -2.0f - (static_cast<float>(j) * 2.0f);
                    instObj3dInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, {x, 0.0f, z})});
                }
            }
            // MATERIAL
            defMatInfo = {};
            auto offset = modelHandler().makeTextureModel(&modelInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addModelIndex(offset);
        }

        // PEAR
        if (!suppress || false) {
            //// modelCreateInfo.pipelineType = PIPELINE_TYPE::TRI_LIST_TEX;
            //// modelCreateInfo.async = true;
            //// modelCreateInfo.material = {};
            //// modelCreateInfo.modelPath = PEAR_MODEL_PATH;
            //// modelCreateInfo.model = helpers::affine();
            //// ModelHandler::makeModel(&modelCreateInfo, SceneHandler::getActiveScene(), nullptr);
        }

        count = 5;
        // MEDIEVAL HOUSE (TRI_COLOR_TEX)
        if (!suppress || false) {
            modelInfo = {};
            modelInfo.pipelineType = GRAPHICS::TRI_LIST_TEX;
            modelInfo.async = false;
            modelInfo.modelPath = MED_H_MODEL_PATH;
            modelInfo.settings.geometryInfo.smoothNormals = false;
            modelInfo.settings.doVisualHelper = true;
            instObj3dInfo = {};
            instObj3dInfo.data.reserve(static_cast<size_t>(count) * static_cast<size_t>(count));
            for (uint32_t i = 0; i < count; i++) {
                float x = -6.5f - (static_cast<float>(i) * 6.5f);
                for (uint32_t j = 0; j < count; j++) {
                    float z = -(static_cast<float>(j) * 6.5f);
                    instObj3dInfo.data.push_back(
                        {helpers::affine(glm::vec3{0.0175f}, {x, 0.0f, z}, M_PI_4_FLT, CARDINAL_Y)});
                }
            }
            // MATERIAL
            defMatInfo = {};
            defMatInfo.pTexture = textureHandler().getTexture(Texture::MEDIEVAL_HOUSE_ID);
            auto offset = modelHandler().makeTextureModel(&modelInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addModelIndex(offset);
        }
        // MEDIEVAL HOUSE (PBR_TEX)
        if (!suppress || false) {
            modelInfo = {};
            modelInfo.pipelineType = GRAPHICS::PBR_TEX;
            modelInfo.async = true;
            modelInfo.modelPath = MED_H_MODEL_PATH;
            modelInfo.settings.geometryInfo.smoothNormals = false;
            modelInfo.settings.doVisualHelper = false;
            instObj3dInfo = {};
            instObj3dInfo.data.reserve(static_cast<size_t>(count) * static_cast<size_t>(count));
            for (uint32_t i = 0; i < count; i++) {
                float x = -6.5f - (static_cast<float>(i) * 6.5f);
                for (uint32_t j = 0; j < count; j++) {
                    float z = (static_cast<float>(j) * 6.5f);
                    instObj3dInfo.data.push_back(
                        {helpers::affine(glm::vec3{0.0175f}, {x, 0.0f, z}, M_PI_4_FLT, CARDINAL_Y)});
                }
            }
            instObj3dInfo.data.push_back(
                {helpers::affine(glm::vec3{0.0175f}, {-6.5f, 0.0f, -6.5f}, M_PI_4_FLT, CARDINAL_Y)});
            // MATERIAL
            pbrMatInfo = {};
            pbrMatInfo.pTexture = textureHandler().getTexture(Texture::MEDIEVAL_HOUSE_ID);
            auto offset = modelHandler().makeTextureModel(&modelInfo, &pbrMatInfo, &instObj3dInfo)->getOffset();
            pScene->addModelIndex(offset);
        }

        // PIG
        if (!suppress || false) {
            // MODEL
            modelInfo = {};
            modelInfo.pipelineType = GRAPHICS::TRI_LIST_COLOR;
            modelInfo.async = true;
            modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
            modelInfo.modelPath = PIG_MODEL_PATH;
            modelInfo.settings.geometryInfo.smoothNormals = true;
            instObj3dInfo = {};
            instObj3dInfo.data.push_back({helpers::affine(glm::vec3{2.0f}, {0.0f, 0.0f, -4.0f})});
            // MATERIAL
            defMatInfo = {};
            defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::MODE_BLINN_PHONG;
            defMatInfo.ambientCoeff = {0.8f, 0.8f, 0.8f};
            defMatInfo.color = {0.8f, 0.8f, 0.8f};
            auto offset = modelHandler().makeColorModel(&modelInfo, &defMatInfo, &instObj3dInfo)->getOffset();
            pScene->addModelIndex(offset);
        }
    }

    if (particles) {
        particleHandler().create();
    }
}

void Scene::Handler::frame() {
    auto& pScene = pScenes_.at(activeSceneIndex_);
    const auto elapsed = shell().getElapsedTime<float>();

    if (false) {
        const float timeInterval = 4800.0f;

        // STARS
        auto& stars = meshHandler().getColorMesh(pScene->starsOffset);
        auto starsRot = glm::rotate(glm::mat4(1.0f), glm::pi<float>() / ((timeInterval * 4.0f) / elapsed), -CARDINAL_X);
        stars->transform(starsRot);
        meshHandler().updateMesh(stars);

        // MOON
        auto& moonLight = uniformHandler().lgtDefDirMgr().getTypedItem(0);
        auto& moon = meshHandler().getTextureMesh(pScene->moonOffset);
        auto moonRot = glm::rotate(glm::mat4(1.0f), glm::pi<float>() / (timeInterval / elapsed), -CARDINAL_X);
        moonLight.direction = glm::mat3(moonRot) * moonLight.direction;
        moon->transform(moonRot);
        meshHandler().updateMesh(moon);
    }

    // DEBUG CAMERA
    if (uniformHandler().hasDebugCamera() && pScene->debugFrustumOffset != Mesh::BAD_OFFSET) {
        auto& debugCamera = uniformHandler().getDebugCamera();
        auto& debugFrustum = meshHandler().getColorMesh(pScene->debugFrustumOffset);
        debugFrustum->setModel(debugCamera.getModel());
        meshHandler().updateMesh(debugFrustum);
    }
}

void Scene::Handler::reset() {
    for (auto& scene : pScenes_) scene->destroy();
    cdlodDbgRenderer_.destroy();
    pScenes_.clear();
}

std::unique_ptr<Scene::Base>& Scene::Handler::makeScene(bool setActive, bool makeFaceSelection) {
    assert(pScenes_.size() < UINT8_MAX);
    index offset = pScenes_.size();

    pScenes_.push_back(std::make_unique<Scene::Base>(std::ref(*this), offset, makeFaceSelection));
    if (setActive) activeSceneIndex_ = offset;

    return pScenes_.back();
}

std::unique_ptr<Mesh::Color>& Scene::Handler::getColorMesh(size_t sceneOffset, size_t meshOffset) {
    // return pScenes_[sceneOffset]->getColorMesh(meshOffset);
    assert(false);
    return meshHandler().getColorMesh(meshOffset);
}

std::unique_ptr<Mesh::Line>& Scene::Handler::getLineMesh(size_t sceneOffset, size_t meshOffset) {
    // return pScenes_[sceneOffset]->getLineMesh(meshOffset);
    assert(false);
    return meshHandler().getLineMesh(meshOffset);
}

std::unique_ptr<Mesh::Texture>& Scene::Handler::getTextureMesh(size_t sceneOffset, size_t meshOffset) {
    // return pScenes_[sceneOffset]->getTextureMesh(meshOffset);
    assert(false);
    return meshHandler().getTextureMesh(meshOffset);
}

void Scene::Handler::recordRenderer(const PASS passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                                    const vk::CommandBuffer& cmd) {
    if (std::visit(Pipeline::IsGraphics{}, pPipelineBindData->type)) {
        auto graphicsType = std::visit(Pipeline::GetGraphics{}, pPipelineBindData->type);
        switch (graphicsType) {
            case GRAPHICS::CDLOD_WF_DEFERRED:
                cdlodDbgRenderer_.update(pPipelineBindData, cmd);
                break;
            default:
                assert(false);
        }
    } else {
        assert(false);
    }
}

void Scene::Handler::cleanup() {
    // There used to be something here...
}