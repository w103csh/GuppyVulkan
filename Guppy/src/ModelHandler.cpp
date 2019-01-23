
#include "FileLoader.h"
#include "Model.h"
#include "ModelHandler.h"
#include "Scene.h"

ModelHandler ModelHandler::inst_;

void ModelHandler::init(Shell* sh, const Game::Settings& settings) {
    inst_.sh_ = sh;
    inst_.settings_ = settings;
}

void ModelHandler::makeModel(ModelCreateInfo* pCreateInfo, std::unique_ptr<Scene>& pScene) {
    if (pCreateInfo->needsTexture) {
        inst_.makeTextureModel(pCreateInfo, pScene);
    } else {
        inst_.makeColorModel(pCreateInfo, pScene);
    }
}

void ModelHandler::makeColorModel(ModelCreateInfo* pCreateInfo, std::unique_ptr<Scene>& pScene) {
    // Add a new model instance
    pCreateInfo->handlerOffset = pModels_.size();
    pModels_.emplace_back(std::make_unique<Model>(pCreateInfo, pScene->getOffset()));

    if (pCreateInfo->async) {
        ldgColorFutures_[pModels_.back()->getHandlerOffset()] = std::make_pair(
            std::async(std::launch::async, &Model::loadColor, pModels_.back().get(), sh_, pCreateInfo->materialInfo),
            std::move(pCreateInfo->callback));
    } else {
        handleMeshes(pScene, pModels_.back(), pModels_.back()->loadColor(sh_, pCreateInfo->materialInfo));
        pModels_.back()->postLoad(pCreateInfo->callback);
    }
}

void ModelHandler::makeTextureModel(ModelCreateInfo* pCreateInfo, std::unique_ptr<Scene>& pScene) {
    // Add a new model instance
    pCreateInfo->handlerOffset = pModels_.size();
    pModels_.emplace_back(std::make_unique<Model>(pCreateInfo, pScene->getOffset()));

    if (pCreateInfo->async) {
        ldgTexFutures_[pModels_.back()->getHandlerOffset()] = std::make_pair(
            std::async(std::launch::async, &Model::loadTexture, pModels_.back().get(), sh_, pCreateInfo->materialInfo),
            std::move(pCreateInfo->callback));
    } else {
        handleMeshes(pScene, pModels_.back(), pModels_.back()->loadTexture(sh_, pCreateInfo->materialInfo));
        pModels_.back()->postLoad(pCreateInfo->callback);
    }
}

void ModelHandler::update(std::unique_ptr<Scene>& pScene) {
    inst_.checkFutures(pScene, inst_.ldgColorFutures_);
    inst_.checkFutures(pScene, inst_.ldgTexFutures_);
}