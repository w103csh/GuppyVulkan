
#include "InputHandler.h"
#include "Shell.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

SceneHandler SceneHandler::inst_;

SceneHandler::SceneHandler() : activeSceneIndex_() {}

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

uint8_t SceneHandler::makeScene(const UniformBufferResources& uboResource, bool setActive) {
    assert(inst_.pScenes_.size() < UINT8_MAX);
    uint8_t offset = inst_.pScenes_.size();

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

    return offset;
}

void SceneHandler::updateDescriptorResources(uint8_t offset, bool isUpdate) {
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