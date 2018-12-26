
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
    // Add a new model instance
    pCreateInfo->handlerOffset = inst_.pModels_.size();
    inst_.pModels_.emplace_back(std::make_unique<Model>(pCreateInfo, pScene->getOffset()));

    if (pCreateInfo->async) {
        inst_.ldgColorFutures_[inst_.pModels_.back()->getHandlerOffset()] = std::make_pair(
            std::async(std::launch::async, &Model::loadColor, inst_.pModels_.back().get(), inst_.sh_, pCreateInfo->material),
            std::move(pCreateInfo->callback));
    } else {
        inst_.handleMeshes(pScene, inst_.pModels_.back(),
                           inst_.pModels_.back()->loadColor(inst_.sh_, pCreateInfo->material));
        inst_.pModels_.back()->postLoad(pCreateInfo->callback);
    }
}

void ModelHandler::makeModel(ModelCreateInfo* pCreateInfo, std::unique_ptr<Scene>& pScene,
                             std::shared_ptr<Texture::Data> pTexture) {
    // Add a new model instance
    pCreateInfo->handlerOffset = inst_.pModels_.size();
    inst_.pModels_.emplace_back(std::make_unique<Model>(pCreateInfo, pScene->getOffset()));

    if (pCreateInfo->async) {
        inst_.ldgTexFutures_[inst_.pModels_.back()->getHandlerOffset()] =
            std::make_pair(std::async(std::launch::async, &Model::loadTexture, inst_.pModels_.back().get(), inst_.sh_,
                                      pCreateInfo->material, pTexture),
                           std::move(pCreateInfo->callback));
    } else {
        inst_.handleMeshes(pScene, inst_.pModels_.back(),
                           inst_.pModels_.back()->loadTexture(inst_.sh_, pCreateInfo->material, pTexture));
        inst_.pModels_.back()->postLoad(pCreateInfo->callback);
    }
}

void ModelHandler::update(std::unique_ptr<Scene>& pScene) {
    inst_.checkFutures(pScene, inst_.ldgColorFutures_);
    inst_.checkFutures(pScene, inst_.ldgTexFutures_);
}