
#include "FileLoader.h"
#include "Model.h"
#include "ModelHandler.h"
#include "Scene.h"

ModelHandler ModelHandler::inst_;

void ModelHandler::init(MyShell* sh) { inst_.sh_ = sh; }

void ModelHandler::makeModel(std::unique_ptr<Scene>& pScene, std::string modelPath, Material material, glm::mat4 model, bool async,
                             Model::CALLBK callback) {
    // Add a new model instance
    inst_.pModels_.emplace_back(std::make_unique<Model>(inst_.pModels_.size(), pScene, modelPath, model));

    if (async) {
        inst_.ldgColorFutures_[inst_.pModels_.back()->getHandlerOffset()] =
            std::make_pair(std::async(std::launch::async, &Model::loadColor, inst_.pModels_.back().get(), inst_.sh_, material),
                           std::move(callback));
    } else {
        inst_.handleMeshes(pScene, inst_.pModels_.back(), inst_.pModels_.back()->loadColor(inst_.sh_, material));
        inst_.pModels_.back()->postLoad(callback);
    }
}

void ModelHandler::makeModel(std::unique_ptr<Scene>& pScene, std::string modelPath, Material material, glm::mat4 model,
                             std::shared_ptr<Texture::Data> pTexture, bool async, Model::CALLBK callback) {
    // Add a new model instance
    inst_.pModels_.emplace_back(std::make_unique<Model>(inst_.pModels_.size(), pScene, modelPath, model));

    if (async) {
        inst_.ldgTexFutures_[inst_.pModels_.back()->getHandlerOffset()] = std::make_pair(
            std::async(std::launch::async, &Model::loadTexture, inst_.pModels_.back().get(), inst_.sh_, material, pTexture),
            std::move(callback));
    } else {
        inst_.handleMeshes(pScene, inst_.pModels_.back(), inst_.pModels_.back()->loadTexture(inst_.sh_, material, pTexture));
        inst_.pModels_.back()->postLoad(callback);
    }
}

void ModelHandler::update(std::unique_ptr<Scene>& pScene) {
    inst_.checkFutures(pScene, inst_.ldgColorFutures_);
    inst_.checkFutures(pScene, inst_.ldgTexFutures_);
}