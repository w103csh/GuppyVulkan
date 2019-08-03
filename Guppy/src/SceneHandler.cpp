
#include "SceneHandler.h"

#include <algorithm>

#include "Axes.h"
#include "Box.h"
#include "Model.h"
#include "Plane.h"
#include "Shell.h"
// HANDLERS
#include "InputHandler.h"
#include "MeshHandler.h"
#include "ModelHandler.h"
#include "RenderPassHandler.h"  /////////////////////////// Remove me after pass dependent things are not longer in the scene.
#include "TextureHandler.h"

Scene::Handler::Handler(Game* pGame) : Game::Handler(pGame), activeSceneIndex_() {}

Scene::Handler::~Handler() = default;  // Required in this file for inner-class forward declaration of "SelectionManager"

void Scene::Handler::init() {
    reset();

    bool suppress = false;

    makeScene(true, (!suppress || false));

    // TODO: move this into a scene already!!!

    // Create info structs
    Mesh::CreateInfo meshInfo;
    Model::CreateInfo modelInfo;
    AxesCreateInfo axesInfo;
    Instance::Default::CreateInfo defInstInfo;
    Material::Default::CreateInfo defMatInfo;
    Material::PBR::CreateInfo pbrMatInfo;

    BoundingBoxMinMax groundPlane_bbmm;
    uint32_t count = 4;

    // ORIGIN AXES
    if (!suppress || false) {
        axesInfo = {};
        axesInfo.lineSize = 500.f;
        axesInfo.showNegative = true;
        defInstInfo = {};
        defMatInfo = {};
        defMatInfo.shininess = 23.123f;
        meshHandler().makeLineMesh<Axes>(&axesInfo, &defMatInfo, &defInstInfo);
    }

    // GROUND PLANE (TEXTURE)
    if (!suppress || false) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
        meshInfo.selectable = false;
        defInstInfo = {};
        defInstInfo.data.push_back({helpers::affine(glm::vec3{500.0f}, {}, -M_PI_2_FLT, CARDINAL_X)});
        defMatInfo = {};
        defMatInfo.pTexture = textureHandler().getTexture(Texture::HARDWOOD_ID);
        defMatInfo.repeat = 800.0f;
        defMatInfo.specularCoeff *= 0.5f;
        defMatInfo.shininess = Material::SHININESS::EGGSHELL;
        auto& pGroundPlane = meshHandler().makeTextureMesh<Plane::Texture>(&meshInfo, &defMatInfo, &defInstInfo);
        groundPlane_bbmm = pGroundPlane->getBoundingBoxMinMax();
    }

    // GROUND PLANE (COLOR)
    if (!suppress && false) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
        meshInfo.selectable = false;
        defInstInfo = {};
        defInstInfo.data.push_back({helpers::affine(glm::vec3{2000.0f}, {}, -M_PI_2_FLT, CARDINAL_X)});
        defMatInfo = {};
        defMatInfo.shininess = Material::SHININESS::EGGSHELL;
        defMatInfo.color = {0.5f, 0.5f, 0.5f};
        auto& pGroundPlane = meshHandler().makeColorMesh<Plane::Color>(&meshInfo, &defMatInfo, &defInstInfo);
        groundPlane_bbmm = pGroundPlane->getBoundingBoxMinMax();
    }

    // PLAIN OLD NON-TRANSFORMED PLANE (TEXTURE)
    if (!suppress && false) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
        meshInfo.selectable = true;
        defInstInfo = {};
        defMatInfo = {};
        defMatInfo.pTexture = textureHandler().getTexture(Texture::VULKAN_ID);
        meshHandler().makeTextureMesh<Plane::Texture>(&meshInfo, &defMatInfo, &defInstInfo);
    }

    // PROJECT PLANE (I AM PASS DEPENDENT AND SHOULDN'T BE HERE!!!! REMOVE INCLUDE IF YOU CAN.)
    if (!suppress || false) {
        std::set<PASS> activePassTypes;
        passHandler().getActivePassTypes(activePassTypes);
        // TODO: these types of checks are not great. The active passes should be able to change at runtime.
        if (std::find(activePassTypes.begin(), activePassTypes.end(), PASS::SAMPLER_PROJECT) != activePassTypes.end()) {
            meshInfo = {};
            meshInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
            meshInfo.selectable = false;
            defInstInfo = {};
            defInstInfo.data.push_back({helpers::affine(glm::vec3{2.0f}, glm::vec3{0.5f, 3.0f, 0.5f})});
            // defInstInfo.data.push_back(
            //    {helpers::affine(glm::vec3{5.0f}, glm::vec3{10.5f, 10.0f, 10.5f}, -M_PI_2_FLT, CARDINAL_Y)});
            // defInstInfo.data.push_back(
            //    {helpers::affine(glm::vec3{2.0f}, glm::vec3{4.5f, -5.0f, 13.5f}, -M_PI_2_FLT, CARDINAL_Z)});
            defMatInfo = {};
            defMatInfo.shininess = Material::SHININESS::EGGSHELL;
            defMatInfo.pTexture = textureHandler().getTexture(RenderPass::PROJECT_2D_ARRAY_TEXTURE_ID);
            defMatInfo.color = {0.5f, 0.5f, 0.5f};
            meshHandler().makeTextureMesh<Plane::Texture>(&meshInfo, &defMatInfo, &defInstInfo);
        }
    }

    // SKYBOX
    if (!suppress || false) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::CUBE;
        meshInfo.selectable = false;
        meshInfo.geometryCreateInfo.invert = true;
        defInstInfo = {};
        defInstInfo.data.push_back({helpers::affine(glm::vec3{10.0f})});
        defMatInfo = {};
        defMatInfo.flags |= Material::FLAG::SKYBOX;
        auto& skybox = meshHandler().makeColorMesh<Box::Color>(&meshInfo, &defMatInfo, &defInstInfo);
    }

    // BOX (TEXTURE)
    if (!suppress || false) {
        meshInfo = {};
        // meshInfo.pipelineType = PIPELINE::PARALLAX_SIMPLE;
        meshInfo.pipelineType = PIPELINE::PARALLAX_STEEP;
        // meshInfo.pipelineType = PIPELINE::PBR_TEX;
        defInstInfo = {};
        defInstInfo.data.push_back(
            {helpers::affine(glm::vec3{1.0f}, glm::vec3{0.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
        defMatInfo = {};
        defMatInfo.pTexture = textureHandler().getTexture(Texture::MYBRICK_ID);
        defMatInfo.shininess = Material::SHININESS::EGGSHELL;
        auto& boxTexture1 = meshHandler().makeTextureMesh<Box::Texture>(&meshInfo, &defMatInfo, &defInstInfo);
        boxTexture1->putOnTop(groundPlane_bbmm);
        meshHandler().updateMesh(boxTexture1);
        meshHandler().makeTangentSpaceVisualHelper(boxTexture1.get());
    }
    // BOX (PBR_TEX)
    if (!suppress || false) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::PBR_TEX;
        // meshInfo.model = helpers::affine(glm::vec3{1.0f}, glm::vec3{1.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f,
        // 0.0f, 1.0f});
        defInstInfo = {};
        defInstInfo.data.push_back(
            {helpers::affine(glm::vec3{1.0f}, glm::vec3{1.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
        pbrMatInfo = {};
        pbrMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::METAL;
        pbrMatInfo.color = {1.0f, 1.0f, 0.0f};
        pbrMatInfo.roughness = 0.86f;  // 0.43f;
        pbrMatInfo.pTexture = textureHandler().getTexture(Texture::VULKAN_ID);
        auto& boxTexture2 = meshHandler().makeTextureMesh<Box::Texture>(&meshInfo, &pbrMatInfo, &defInstInfo);
        boxTexture2->putOnTop(groundPlane_bbmm);
        meshHandler().updateMesh(boxTexture2);
        meshHandler().makeTangentSpaceVisualHelper(boxTexture2.get());
    }
    // BOX (PBR_COLOR)
    if (!suppress || false) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::PBR_COLOR;
        // meshInfo.model = helpers::affine(glm::vec3{1.0f}, glm::vec3{2.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f,
        // 0.0f, 1.0f});
        defInstInfo = {};
        defInstInfo.data.push_back(
            {helpers::affine(glm::vec3{1.0f}, glm::vec3{2.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
        pbrMatInfo = {};
        pbrMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR;
        pbrMatInfo.color = {1.0f, 1.0f, 0.0f};
        auto& boxPbrColor = meshHandler().makeColorMesh<Box::Color>(&meshInfo, &pbrMatInfo, &defInstInfo);
        boxPbrColor->putOnTop(groundPlane_bbmm);
        meshHandler().updateMesh(boxPbrColor);
    }
    // BOX (TRI_LIST_COLOR)
    if (!suppress || false) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
        // meshInfo.model = helpers::affine(glm::vec3{1.0f}, glm::vec3{5.0f, 0.0f, 3.5f}, M_PI_2_FLT, glm::vec3{-1.0f,
        // 0.0f, 1.0f});
        // meshInfo.model =
        //    helpers::affine(glm::vec3{1.0f}, glm::vec3{-1.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f});
        defInstInfo = {};
        defInstInfo.data.push_back(
            {helpers::affine(glm::vec3{1.0f}, glm::vec3{-1.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
        defMatInfo = {};
        defMatInfo.color = {1.0f, 1.0f, 0.0f};
        auto& boxDefColor1 = meshHandler().makeColorMesh<Box::Color>(&meshInfo, &defMatInfo, &defInstInfo);
        boxDefColor1->putOnTop(groundPlane_bbmm);
        meshHandler().updateMesh(boxDefColor1);
    }
    // BOX (CUBE)
    if (!suppress || false) {
        meshInfo = {};
        defInstInfo = {};
        defInstInfo.data.push_back(
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
        meshInfo.pipelineType = PIPELINE::CUBE;
        defMatInfo.pTexture = textureHandler().getTexture(Texture::SKYBOX_ID);
        meshHandler().makeColorMesh<Box::Color>(&meshInfo, &defMatInfo, &defInstInfo);
        ////
        // defInstInfo = {};
        // defInstInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{0.0f, 1.0f, 0.0f}, M_PI_2_FLT,
        // CARDINAL_Z)}); meshHandler().makeColorMesh<Plane::Color>(&meshInfo, &defMatInfo, &defInstInfo);
        ////
        // meshInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
        // defMatInfo = {};
        // defMatInfo.color = {0x2E / 255.0f, 0x40 / 255.0f, 0x53 / 255.0f};
        // defInstInfo = {};
        // defInstInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{1.0f, 0.0f, 0.0f})});
        // meshHandler().makeColorMesh<Plane::Color>(&meshInfo, &defMatInfo, &defInstInfo);
        ////
        // meshInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
        // defMatInfo = {};
        // defMatInfo.pTexture = textureHandler().getTextureByName(Texture::STATUE_ID);
        // defInstInfo = {};
        // defInstInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{2.0f, 0.0f, 0.0f})});
        // meshHandler().makeTextureMesh<Plane::Texture>(&meshInfo, &defMatInfo, &defInstInfo);
    }

    // BURNT ORANGE TORUS
    if (!suppress || false) {
        // MODEL
        modelInfo = {};
        modelInfo.async = false;
        modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
        modelInfo.modelPath = TORUS_MODEL_PATH;
        modelInfo.smoothNormals = true;
        defInstInfo = {};
        defInstInfo.data.push_back({helpers::affine(glm::vec3{0.07f})});
        // MATERIAL
        if (false) {
            modelInfo.pipelineType = PIPELINE::PBR_COLOR;
            pbrMatInfo = {};
            pbrMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::METAL;
            pbrMatInfo.color = {0.8f, 0.3f, 0.0f};
            pbrMatInfo.roughness = 0.9f;
            modelHandler().makeColorModel(&modelInfo, &pbrMatInfo, &defInstInfo);
        } else if (true) {
            modelInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
            defMatInfo = {};
            defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR;
            defMatInfo.ambientCoeff = {0.8f, 0.3f, 0.0f};
            defMatInfo.color = {0.8f, 0.3f, 0.0f};
            // defMatInfo.ambientCoeff = glm::vec3{0.2f};
            // defMatInfo.specularCoeff = glm::vec3{1.0f};
            // defMatInfo.color = {0.9f, 0.3f, 0.2f};
            // defMatInfo.shininess = 25.0f;
            modelHandler().makeColorModel(&modelInfo, &defMatInfo, &defInstInfo);
        } else {
            modelInfo.pipelineType = PIPELINE::CUBE;
            defMatInfo = {};
            pbrMatInfo.color = {0.8f, 0.3f, 0.0f};
            defMatInfo.flags |= Material::FLAG::REFLECT;
            defMatInfo.reflectionFactor = 0.85f;
            defMatInfo.pTexture = textureHandler().getTexture(Texture::SKYBOX_ID);
            modelHandler().makeColorModel(&modelInfo, &defMatInfo, &defInstInfo);
        }
    }

    count = 100;
    // GRASS
    if (!suppress || false) {
        // MODEL
        modelInfo = {};
        modelInfo.pipelineType = PIPELINE::BP_TEX_CULL_NONE;
        modelInfo.async = false;
        // modelInfo.callback = [groundPlane_bbmm](auto pModel) {};
        modelInfo.visualHelper = false;
        modelInfo.modelPath = GRASS_LP_MODEL_PATH;
        modelInfo.smoothNormals = false;
        defInstInfo = {};
        defInstInfo.update = false;
        std::srand(static_cast<unsigned>(time(0)));
        float x, z;
        float f1 = 0.03f;
        // float f1 = 0.2f;
        float f2 = f1 * 5.0f;
        float low = f2 * 0.05f;
        float high = f2 * 3.0f;
        defInstInfo.data.reserve(static_cast<size_t>(count) * static_cast<size_t>(count) * 4);
        for (uint32_t i = 0; i < count; i++) {
            x = static_cast<float>(i) * f2;
            // x += low + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (high - low)));
            for (uint32_t j = 0; j < count; j++) {
                z = static_cast<float>(j) * f2;
                // z += low + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (high - low)));
                auto scale = glm::vec3{f1};
                auto translate1 = glm::vec3{x, 0.0f, z};
                auto translate2 = glm::vec3{x, 0.0f, -z};
                defInstInfo.data.push_back({helpers::affine(scale, translate1)});
                if (i == 0 && j == 0) continue;
                defInstInfo.data.push_back({helpers::affine(scale, -translate1)});
                if (i == 0) continue;
                defInstInfo.data.push_back({helpers::affine(scale, translate2)});
                defInstInfo.data.push_back({helpers::affine(scale, -translate2)});
            }
        }
        // MATERIAL
        defMatInfo = {};
        modelHandler().makeTextureModel(&modelInfo, &defMatInfo, &defInstInfo);
    }

    count = 5;
    // ORANGE
    if (!suppress || false) {
        // MODEL
        modelInfo = {};
        modelInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
        modelInfo.async = false;
        modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
        modelInfo.visualHelper = true;
        modelInfo.modelPath = ORANGE_MODEL_PATH;
        modelInfo.smoothNormals = true;
        defInstInfo = {};
        defInstInfo.update = false;
        defInstInfo.data.reserve(static_cast<size_t>(count) * static_cast<size_t>(count));
        for (uint32_t i = 0; i < count; i++) {
            float x = 4.0f + (static_cast<float>(i) * 2.0f);
            for (uint32_t j = 0; j < count; j++) {
                float z = -2.0f - (static_cast<float>(j) * 2.0f);
                defInstInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, {x, 0.0f, z})});
            }
        }
        // MATERIAL
        defMatInfo = {};
        modelHandler().makeTextureModel(&modelInfo, &defMatInfo, &defInstInfo);
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
        modelInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
        modelInfo.async = false;
        modelInfo.modelPath = MED_H_MODEL_PATH;
        modelInfo.smoothNormals = false;
        modelInfo.visualHelper = true;
        defInstInfo = {};
        defInstInfo.data.reserve(static_cast<size_t>(count) * static_cast<size_t>(count));
        for (uint32_t i = 0; i < count; i++) {
            float x = -6.5f - (static_cast<float>(i) * 6.5f);
            for (uint32_t j = 0; j < count; j++) {
                float z = -(static_cast<float>(j) * 6.5f);
                defInstInfo.data.push_back({helpers::affine(glm::vec3{0.0175f}, {x, 0.0f, z}, M_PI_4_FLT, CARDINAL_Y)});
            }
        }
        // MATERIAL
        defMatInfo = {};
        defMatInfo.pTexture = textureHandler().getTexture(Texture::MEDIEVAL_HOUSE_ID);
        modelHandler().makeTextureModel(&modelInfo, &defMatInfo, &defInstInfo);
    }
    // MEDIEVAL HOUSE (PBR_TEX)
    if (!suppress || false) {
        modelInfo = {};
        modelInfo.pipelineType = PIPELINE::PBR_TEX;
        modelInfo.async = true;
        modelInfo.modelPath = MED_H_MODEL_PATH;
        modelInfo.smoothNormals = false;
        modelInfo.visualHelper = false;
        defInstInfo = {};
        defInstInfo.data.reserve(static_cast<size_t>(count) * static_cast<size_t>(count));
        for (uint32_t i = 0; i < count; i++) {
            float x = -6.5f - (static_cast<float>(i) * 6.5f);
            for (uint32_t j = 0; j < count; j++) {
                float z = (static_cast<float>(j) * 6.5f);
                defInstInfo.data.push_back({helpers::affine(glm::vec3{0.0175f}, {x, 0.0f, z}, M_PI_4_FLT, CARDINAL_Y)});
            }
        }
        defInstInfo.data.push_back({helpers::affine(glm::vec3{0.0175f}, {-6.5f, 0.0f, -6.5f}, M_PI_4_FLT, CARDINAL_Y)});
        // MATERIAL
        pbrMatInfo = {};
        pbrMatInfo.pTexture = textureHandler().getTexture(Texture::MEDIEVAL_HOUSE_ID);
        modelHandler().makeTextureModel(&modelInfo, &pbrMatInfo, &defInstInfo);
    }

    // PIG
    if (!suppress || false) {
        // MODEL
        modelInfo = {};
        modelInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
        modelInfo.async = true;
        modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
        modelInfo.modelPath = PIG_MODEL_PATH;
        modelInfo.smoothNormals = true;
        defInstInfo = {};
        defInstInfo.data.push_back({helpers::affine(glm::vec3{2.0f}, {0.0f, 0.0f, -4.0f})});
        // MATERIAL
        defMatInfo = {};
        defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::MODE_BLINN_PHONG;
        defMatInfo.ambientCoeff = {0.8f, 0.8f, 0.8f};
        defMatInfo.color = {0.8f, 0.8f, 0.8f};
        modelHandler().makeColorModel(&modelInfo, &defMatInfo, &defInstInfo);
    }
}

void Scene::Handler::reset() {
    for (auto& scene : pScenes_) scene->destroy();
    pScenes_.clear();
}

SCENE_INDEX_TYPE Scene::Handler::makeScene(bool setActive, bool makeFaceSelection) {
    assert(pScenes_.size() < UINT8_MAX);
    SCENE_INDEX_TYPE offset = pScenes_.size();

    pScenes_.push_back(std::make_unique<Scene::Base>(std::ref(*this), offset, makeFaceSelection));
    if (setActive) activeSceneIndex_ = offset;

    return offset;
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

void Scene::Handler::cleanup() {
    for (auto& res : invalidRes_) {
        // dynamic buffer
        if (res.pDynUBORes != nullptr) {
            vkDestroyBuffer(shell().context().dev, res.pDynUBORes->buffer, nullptr);
            vkFreeMemory(shell().context().dev, res.pDynUBORes->memory, nullptr);
        }
        // descriptor sets
        if (!res.texSets.empty()) assert(res.pool != VK_NULL_HANDLE);
        for (auto& sets : res.texSets) {
            if (!sets.empty()) {
                vk::assert_success(
                    vkFreeDescriptorSets(shell().context().dev, res.pool, static_cast<uint32_t>(sets.size()), sets.data()));
            }
            sets.clear();
        }
    }
    invalidRes_.clear();
}