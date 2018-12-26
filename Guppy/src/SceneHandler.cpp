
#include "InputHandler.h"
#include "Shell.h"
#include "SceneHandler.h"
#include "SelectionManager.h"
#include "TextureHandler.h"

SceneHandler SceneHandler::inst_;

SceneHandler::SceneHandler() : activeSceneIndex_(), pSelectionManager_(std::make_unique<SelectionManager>()) {}

SceneHandler::~SceneHandler() = default;  // Required in this file for inner-class forward declaration of "SelectionManager"

void SceneHandler::init(Shell* sh, const Game::Settings& settings) {
    inst_.reset();
    inst_.sh_ = sh;
    inst_.ctx_ = sh->context();
    inst_.settings_ = settings;
}

void SceneHandler::reset() {
    for (auto& scene : pScenes_) scene->destroy(ctx_.dev);
    pScenes_.clear();
}

SCENE_INDEX_TYPE SceneHandler::makeScene(const UniformBufferResources& uboResource, bool setActive) {
    assert(inst_.pScenes_.size() < UINT8_MAX);
    SCENE_INDEX_TYPE offset = inst_.pScenes_.size();

    inst_.pScenes_.emplace_back(std::unique_ptr<Scene>(new Scene(offset)));
    if (setActive) inst_.activeSceneIndex_ = offset;

    // External sync: TextureHandler (starting descriptor set size, texture flags)
    inst_.pScenes_.back()->createDynamicTexUniformBuffer(inst_.ctx_, inst_.settings_);
    inst_.pScenes_.back()->pDescResources_ = PipelineHandler::createDescriptorResources(
        {uboResource.info}, {inst_.pScenes_.back()->pDynUBOResource_->info}, 1, TextureHandler::getCount());
    updateDescriptorResources(offset, false);

    PipelineHandler::createPipelineResources(inst_.pScenes_.back()->plResources_);
    inst_.sh_->updateUIResources((*inst_.pScenes_.back()->pDescResources_), inst_.pScenes_.back()->plResources_);

    inst_.pScenes_.back()->createDrawCmds(inst_.ctx_);

    // Selection
    if (inst_.pSelectionManager_->pFace_ == nullptr) {
        // Face selection
        MeshCreateInfo createInfo = {};
        createInfo.isIndexed = false;
        createInfo.pMaterial = std::make_unique<Material>(Material::FLAGS::HIDE);

        std::unique_ptr<LineMesh> pFaceSelection = std::make_unique<FaceSelection>(&createInfo);
        inst_.pSelectionManager_->faceSelectionOffset_ =
            inst_.getActiveScene()->moveMesh(inst_.settings_, inst_.ctx_, std::move(pFaceSelection))->getOffset();
    }

    // TODO: move selection meshes...

    return offset;
}

const std::unique_ptr<Face>& SceneHandler::getFaceSelectionFace() { return inst_.pSelectionManager_->pFace_; }

void SceneHandler::select(const VkDevice& dev, const Ray& ray) {
    float tMin = T_MAX;  // This is relative to the distance between ray.e & ray.d
    Face face;

    inst_.pSelectionManager_->selectFace(ray, tMin, getActiveScene()->colorMeshes_, face);
    inst_.pSelectionManager_->selectFace(ray, tMin, getActiveScene()->texMeshes_, face);

    inst_.pSelectionManager_->updateFaceSelection(dev, (tMin < T_MAX) ? std::make_unique<Face>(face) : nullptr);
}

void SceneHandler::updateDescriptorResources(SCENE_INDEX_TYPE offset, bool isUpdate) {
    auto& pScene = inst_.pScenes_[offset];

    // destroy old uniforms first
    if (isUpdate) {
        inst_.invalidRes_.push_back(
            {pScene->pDescResources_->pool, pScene->pDescResources_->texSets, pScene->pDynUBOResource_});
        pScene->pDynUBOResource_ = nullptr;
        pScene->pDescResources_->texSets.clear();

        pScene->createDynamicTexUniformBuffer(inst_.ctx_, inst_.settings_);
        pScene->pDescResources_->dynUboInfos = {pScene->pDynUBOResource_->info};
        pScene->pDescResources_->texCount = pScene->pDynUBOResource_->count;
        pScene->pDescResources_->texSets.resize(pScene->pDynUBOResource_->count);
    }

    // Allocate descriptor sets for meshes that are ready ...
    for (auto& pMesh : pScene->texMeshes_) {
        if (pMesh->getStatus() == STATUS::READY) {
            pMesh->tryCreateDescriptorSets(pScene->pDescResources_);
        }
    }
}

void SceneHandler::destroyDescriptorResources(std::unique_ptr<DescriptorResources>& pRes) {
    vkDestroyDescriptorPool(inst_.ctx_.dev, pRes->pool, nullptr);
}

void SceneHandler::cleanupInvalidResources() {
    for (auto& res : inst_.invalidRes_) {
        // dynamic buffer
        if (res.pDynUBORes != nullptr) {
            vkDestroyBuffer(inst_.ctx_.dev, res.pDynUBORes->buffer, nullptr);
            vkFreeMemory(inst_.ctx_.dev, res.pDynUBORes->memory, nullptr);
        }
        // descriptor sets
        if (!res.texSets.empty()) assert(res.pool != VK_NULL_HANDLE);
        for (auto& sets : res.texSets) {
            if (!sets.empty()) {
                vk::assert_success(
                    vkFreeDescriptorSets(inst_.ctx_.dev, res.pool, static_cast<uint32_t>(sets.size()), sets.data()));
            }
            sets.clear();
        }
    }
    inst_.invalidRes_.clear();
}