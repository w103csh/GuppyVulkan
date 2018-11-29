
#include "FileLoader.h"
#include "Model.h"
#include "ModelHandler.h"
#include "Scene.h"

ModelHandler ModelHandler::inst_;

void ModelHandler::init(MyShell* sh) { inst_.sh_ = sh; }

void ModelHandler::makeModel(std::unique_ptr<Scene>& pScene, std::string modelPath, Material material, glm::mat4 model,
          bool async, std::function<void(Mesh*)> callback) {
    // Add a new model instance
    inst_.pModels_.emplace_back(std::make_unique<Model>(inst_.pModels_.size(), pScene, modelPath, model));

    if (async) {
        inst_.ldgColorFutures_[inst_.pModels_.back()->getHandlerOffset()] =
            std::async(std::launch::async, &Model::load, inst_.pModels_.back().get(), inst_.sh_, material, callback);
    } else {
        inst_.handleMeshes(pScene, inst_.pModels_.back(), inst_.pModels_.back()->load(inst_.sh_, material, callback));
    }
}

void ModelHandler::update(std::unique_ptr<Scene>& pScene) {
    inst_.checkFutures(pScene, inst_.ldgColorFutures_);
    inst_.checkFutures(pScene, inst_.ldgTexFutures_);
}