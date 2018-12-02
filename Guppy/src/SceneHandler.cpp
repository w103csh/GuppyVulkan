
#include "MyShell.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

SceneHandler SceneHandler::inst_;

SceneHandler::SceneHandler() : activeSceneIndex_() {}

void SceneHandler::init(const MyShell::Context& ctx, const Game::Settings& settings) {
    inst_.reset();
    inst_.ctx_ = ctx;
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
    updateDescriptorResources(offset, &uboResource);
    PipelineHandler::createPipelineResources(inst_.pScenes_.back()->plResources_);
    inst_.pScenes_.back()->create_draw_cmds(inst_.ctx_);

    return offset;
}

void SceneHandler::updateDescriptorResources(uint8_t offset, const UniformBufferResources* pUBORes) {
    auto& pScene = inst_.pScenes_[offset];
    bool isUpdate = (pUBORes == nullptr);

    // destroy old uniforms first
    if (isUpdate) {
        pScene->pDescResources_->pDynUBORes = pScene->pDynUBOResource_;  // move for destruction
        pScene->pDynUBOResource_ = nullptr;
    }

    pScene->createDynamicTexUniformBuffer(inst_.ctx_, inst_.settings_);

    // ubo info can be resused for now.
    const auto uboInfos = isUpdate ? pScene->pDescResources_->uboInfos : std::vector<VkDescriptorBufferInfo>({pUBORes->info});
    // dynamic ubo cannot be reused.
    const auto dynUboInfos = std::vector<VkDescriptorBufferInfo>({pScene->pDynUBOResource_->info});

    auto pNewRes = PipelineHandler::createDescriptorResources(uboInfos, dynUboInfos, 1, TextureHandler::getCount());

    if (isUpdate) {
        inst_.oldDescRes_.emplace_back(std::move(pScene->pDescResources_));
    }
    pScene->pDescResources_ = std::move(pNewRes);

    // Allocate descriptor sets for meshes that are ready ...
    for (auto& pMesh : pScene->texMeshes_) {
        if (pMesh->getStatus() == STATUS::READY) {
            pMesh->tryCreateDescriptorSets(pScene->pDescResources_);
        }
    }
}

void SceneHandler::destroyDescriptorResources(std::unique_ptr<DescriptorResources>& pRes) {
    vkDestroyDescriptorPool(inst_.ctx_.dev, pRes->pool, nullptr);
    if (pRes->pDynUBORes != nullptr) {
        vkDestroyBuffer(inst_.ctx_.dev, pRes->pDynUBORes->buffer, nullptr);
        vkFreeMemory(inst_.ctx_.dev, pRes->pDynUBORes->memory, nullptr);
    }
}

void SceneHandler::cleanupOldResources() {
    for (auto& pRes : inst_.oldDescRes_) inst_.destroyDescriptorResources(pRes);
    inst_.oldDescRes_.clear();
}