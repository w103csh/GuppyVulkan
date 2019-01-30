
#include "SceneHandler.h"

#include "InputHandler.h"
#include "SelectionManager.h"
#include "Shell.h"
#include "TextureHandler.h"

Scene::Handler::Handler(Game* pGame)
    : Game::Handler(pGame), activeSceneIndex_(), pSelectionManager_(std::make_unique<Selection::Manager>(std::ref(*this))) {}

Scene::Handler::~Handler() = default;  // Required in this file for inner-class forward declaration of "SelectionManager"

void Scene::Handler::init() { reset(); }

void Scene::Handler::reset() {
    for (auto& scene : pScenes_) scene->destroy();
    pScenes_.clear();
}

SCENE_INDEX_TYPE Scene::Handler::makeScene(bool setActive, bool makeFaceSelection) {
    assert(pScenes_.size() < UINT8_MAX);
    SCENE_INDEX_TYPE offset = pScenes_.size();

    pScenes_.push_back(std::make_unique<Scene::Base>(std::ref(*this), offset));
    if (setActive) activeSceneIndex_ = offset;

    // Selection
    if (makeFaceSelection) pSelectionManager_->addFaceSelection(getActiveScene());
    // TODO: move selection meshes...

    return offset;
}

void Scene::Handler::updatePipelineReferences(const PIPELINE& type, const VkPipeline& pipeline) {
    // Not concerned with speed here at the moment. If recreating pipelines
    // becomes something that is needed a lot then this is not great.
    for (auto& pScene : pScenes_) {
        for (auto& pMesh : pScene->colorMeshes_)
            if (pMesh->PIPELINE_TYPE == type) pMesh->updatePipelineReferences(type, pipeline);
        for (auto& pMesh : pScene->lineMeshes_)
            if (pMesh->PIPELINE_TYPE == type) pMesh->updatePipelineReferences(type, pipeline);
        for (auto& pMesh : pScene->texMeshes_)
            if (pMesh->PIPELINE_TYPE == type) pMesh->updatePipelineReferences(type, pipeline);
    }
}

const std::unique_ptr<Face>& Scene::Handler::getFaceSelection() { return pSelectionManager_->getFace(); }

void Scene::Handler::select(const Ray& ray) {
    float tMin = T_MAX;  // This is relative to the distance between ray.e & ray.d
    Face face;

    // TODO: pass the scene in, or better, fix it...
    pSelectionManager_->selectFace(ray, tMin, getActiveScene()->colorMeshes_, face);
    pSelectionManager_->selectFace(ray, tMin, getActiveScene()->texMeshes_, face);

    pSelectionManager_->updateFaceSelection((tMin < T_MAX) ? std::make_unique<Face>(face) : nullptr);
}

void Scene::Handler::updateDescriptorSets(SCENE_INDEX_TYPE offset, bool isUpdate) {
    // auto& pScene = pScenes_[offset];

    //// destroy old uniforms first
    // if (isUpdate) {
    //    invalidRes_.push_back(
    //        {pScene->pDescResources_->pool, pScene->pDescResources_->texSets, pScene->pDynUBOResource_});
    //    pScene->pDynUBOResource_ = nullptr;
    //    pScene->pDescResources_->texSets.clear();

    //    pScene->createDynamicTexUniformBuffer(shell().context(), settings());
    //    pScene->pDescResources_->dynUboInfos = {pScene->pDynUBOResource_->info};
    //    pScene->pDescResources_->texCount = pScene->pDynUBOResource_->count;
    //    pScene->pDescResources_->texSets.resize(pScene->pDynUBOResource_->count);
    //}

    //// Allocate descriptor sets for meshes that are ready ...
    // for (auto& pMesh : pScene->texMeshes_) {
    //    if (pMesh->getStatus() == STATUS::READY) {
    //        pMesh->tryCreateDescriptorSets(pScene->pDescResources_);
    //    }
    //}
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