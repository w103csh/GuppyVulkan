
#include "InputHandler.h"
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

SCENE_INDEX_TYPE SceneHandler::makeScene(bool setActive, bool makeFaceSelection) {
    assert(inst_.pScenes_.size() < UINT8_MAX);
    SCENE_INDEX_TYPE offset = inst_.pScenes_.size();

    inst_.pScenes_.emplace_back(std::unique_ptr<Scene>(new Scene(offset)));
    if (setActive) inst_.activeSceneIndex_ = offset;

    // Selection
    if (makeFaceSelection && inst_.pSelectionManager_->pFace_ == nullptr) {
        // Face selection
        MeshCreateInfo createInfo = {};
        createInfo.isIndexed = false;
        createInfo.pMaterial = std::make_unique<Material>(Material::FLAGS::HIDE);

        std::unique_ptr<LineMesh> pFaceSelection = std::make_unique<FaceSelection>(&createInfo);
        // thread sync
        inst_.pSelectionManager_->faceSelectionOffset_ =
            inst_.getActiveScene()->moveMesh(inst_.settings_, inst_.ctx_, std::move(pFaceSelection))->getOffset();
    }

    // TODO: move selection meshes...

    return offset;
}

void SceneHandler::updatePipelineReferences(const PIPELINE_TYPE& type, const VkPipeline& pipeline) {
    for (auto& pScene : inst_.pScenes_) {
        switch (type) {
            case PIPELINE_TYPE::TRI_LIST_COLOR:
                for (auto& pMesh : pScene->colorMeshes_) pMesh->updatePipelineReferences(type, pipeline);
                break;
            case PIPELINE_TYPE::LINE:
                for (auto& pMesh : pScene->lineMeshes_) pMesh->updatePipelineReferences(type, pipeline);
                break;
            case PIPELINE_TYPE::TRI_LIST_TEX:
                for (auto& pMesh : pScene->texMeshes_) pMesh->updatePipelineReferences(type, pipeline);
                break;
        }
    }
}

const std::unique_ptr<Face>& SceneHandler::getFaceSelectionFace() { return inst_.pSelectionManager_->pFace_; }

void SceneHandler::select(const VkDevice& dev, const Ray& ray) {
    float tMin = T_MAX;  // This is relative to the distance between ray.e & ray.d
    Face face;

    inst_.pSelectionManager_->selectFace(ray, tMin, getActiveScene()->colorMeshes_, face);
    inst_.pSelectionManager_->selectFace(ray, tMin, getActiveScene()->texMeshes_, face);

    inst_.pSelectionManager_->updateFaceSelection(dev, (tMin < T_MAX) ? std::make_unique<Face>(face) : nullptr);
}

void SceneHandler::updateDescriptorSets(SCENE_INDEX_TYPE offset, bool isUpdate) {
    // auto& pScene = inst_.pScenes_[offset];

    //// destroy old uniforms first
    // if (isUpdate) {
    //    inst_.invalidRes_.push_back(
    //        {pScene->pDescResources_->pool, pScene->pDescResources_->texSets, pScene->pDynUBOResource_});
    //    pScene->pDynUBOResource_ = nullptr;
    //    pScene->pDescResources_->texSets.clear();

    //    pScene->createDynamicTexUniformBuffer(inst_.ctx_, inst_.settings_);
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