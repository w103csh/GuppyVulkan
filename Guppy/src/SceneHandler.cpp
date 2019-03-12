
#include "SceneHandler.h"

#include "InputHandler.h"
#include "MeshHandler.h"
#include "Shell.h"
#include "TextureHandler.h"

Scene::Handler::Handler(Game* pGame) : Game::Handler(pGame), activeSceneIndex_() {}

Scene::Handler::~Handler() = default;  // Required in this file for inner-class forward declaration of "SelectionManager"

void Scene::Handler::init() { reset(); }

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

void Scene::Handler::updatePipelineReferences(const PIPELINE& type, const VkPipeline& pipeline) {
    // Not concerned with speed here at the moment. If recreating pipelines
    // becomes something that is needed a lot then redo this.
    for (auto& pScene : pScenes_) {
        for (auto& pMesh : meshHandler().getColorMeshes())
            if (pMesh->PIPELINE_TYPE == type) pMesh->updatePipelineReferences(type, pipeline);
        for (auto& pMesh : meshHandler().getLineMeshes())
            if (pMesh->PIPELINE_TYPE == type) pMesh->updatePipelineReferences(type, pipeline);
        for (auto& pMesh : meshHandler().getTextureMeshes())
            if (pMesh->PIPELINE_TYPE == type) pMesh->updatePipelineReferences(type, pipeline);
    }
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