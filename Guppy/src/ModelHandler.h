#ifndef MODEL_HANDLER_H
#define MODEL_HANDLER_H

#include <future>
#include <unordered_map>
#include <vector>

#include "FileLoader.h"
#include "Game.h"
#include "Instance.h"
#include "Material.h"
#include "Mesh.h"
#include "MeshHandler.h"  // TODO: including this is sketchy
#include "Model.h"
#include "ModelMesh.h"
#include "Scene.h"

namespace Model {

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame);

    void init() override{};
    inline void destroy() override { reset(); }

    template <typename TMaterialCreateInfo, typename TInstanceCreateInfo>
    void makeColorModel(Model::CreateInfo* pCreateInfo, TMaterialCreateInfo* pMaterialCreateInfo,
                        TInstanceCreateInfo* pInstanceCreateInfo) {
        pCreateInfo->handlerOffset = pModels_.size();  // TODO: really?

        // INSTANCE
        auto& pInstanceData = meshHandler().makeInstanceData(pInstanceCreateInfo);

        pModels_.emplace_back(std::make_unique<Model::Base>(std::ref(*this), pCreateInfo, pInstanceData));

        if (pCreateInfo->async) {
            ldgColorFutures_[pModels_.back()->getOffset()] = std::make_pair(  //
                std::async(std::launch::async, &Handler::loadColor<TMaterialCreateInfo>, this, std::ref(*pModels_.back()),
                           *pMaterialCreateInfo, pInstanceData),
                std::move(pCreateInfo->callback)  //
            );
        } else {
            handleMeshes(pModels_.back(), loadColor(std::ref(*pModels_.back()), *pMaterialCreateInfo, pInstanceData),
                         pCreateInfo->callback);
        }
    }

    template <typename TMaterialCreateInfo, typename TInstanceCreateInfo>
    void makeTextureModel(Model::CreateInfo* pCreateInfo, TMaterialCreateInfo* pMaterialCreateInfo,
                          TInstanceCreateInfo* pInstanceCreateInfo) {
        pCreateInfo->handlerOffset = pModels_.size();  // TODO: really?

        // INSTANCE
        auto& pInstanceData = meshHandler().makeInstanceData(pInstanceCreateInfo);

        pModels_.emplace_back(std::make_unique<Model::Base>(std::ref(*this), pCreateInfo, pInstanceData));

        if (pCreateInfo->async) {
            ldgTexFutures_[pModels_.back()->getOffset()] = std::make_pair(  //
                std::async(std::launch::async, &Handler::loadTexture<TMaterialCreateInfo>, this, std::ref(*pModels_.back()),
                           *pMaterialCreateInfo, pInstanceData),
                std::move(pCreateInfo->callback)  //
            );
        } else {
            handleMeshes(pModels_.back(), loadTexture(std::ref(*pModels_.back()), *pMaterialCreateInfo, pInstanceData),
                         pCreateInfo->callback);
        }
    }

    std::unique_ptr<Model::Base>& getModel(Model::INDEX offset) {
        assert(offset < pModels_.size());
        return pModels_[offset];
    }

    void update(std::unique_ptr<Scene::Base>& pScene);

   private:
    void reset() override{};

    FileLoader::tinyobj_data loadData(Model::Base& model);

    template <typename TMaterialCreateInfo>
    std::vector<Mesh::Color*> loadColor(Model::Base& model, TMaterialCreateInfo materialCreateInfo,
                                        std::shared_ptr<Instance::Base>& pInstanceData) {
        auto data = loadData(model);
        auto createInfo = model.getMeshCreateInfo();

        // Determine amount and type of meshes
        std::vector<Mesh::Color*> pMeshes;

        if (data.materials.empty()) {
            makeColorMesh(model, pMeshes, &materialCreateInfo, pInstanceData);
        } else {
            for (auto& m : data.materials) {
                makeColorMesh(model, pMeshes, &materialCreateInfo, pInstanceData);
                // TODO: Doing this after creation forces a second copy to device memory immediately
                pMeshes.back()->getMaterial()->setTinyobjData(m);
            }
        }

        // Load .obj data into mesh
        // (The map types have comparison predicates that smooth or not)
        if (model.smoothNormals_) {
            FileLoader::loadObjData<unique_vertices_map_smoothing>(data, pMeshes);
        } else {
            FileLoader::loadObjData<unique_vertices_map_non_smoothing>(data, pMeshes);
        }

        for (auto& pMesh : pMeshes) assert(pMesh->getVertexCount());  // ensure something was loaded

        return pMeshes;
    }

    template <typename TMaterialCreateInfo>
    std::vector<Mesh::Texture*> loadTexture(Model::Base& model, TMaterialCreateInfo materialCreateInfo,
                                            std::shared_ptr<Instance::Base>& pInstanceData) {
        auto data = loadData(model);
        auto createInfo = model.getMeshCreateInfo();

        // Determine amount and type of meshes
        std::vector<Mesh::Texture*> pMeshes;

        if (data.materials.empty()) {
            makeTextureMesh(model, pMeshes, &materialCreateInfo, pInstanceData);
        } else {
            auto modelDirectory = helpers::getFilePath(model.modelPath_);
            for (auto& tinyobj_mat : data.materials) {
                makeTexture(tinyobj_mat, modelDirectory, &materialCreateInfo);
                makeTextureMesh(model, pMeshes, &materialCreateInfo, pInstanceData);
                materialCreateInfo.pTexture = nullptr;
                // TODO: Doing this after creation forces a second copy to device memory immediately
                pMeshes.back()->getMaterial()->setTinyobjData(tinyobj_mat);
            }
        }

        // Load .obj data into mesh
        // (The map types have comparison predicates that smooth or not)
        if (model.smoothNormals_) {
            FileLoader::loadObjData<unique_vertices_map_smoothing>(data, pMeshes);
        } else {
            FileLoader::loadObjData<unique_vertices_map_non_smoothing>(data, pMeshes);
        }

        for (auto& pMesh : pMeshes) assert(pMesh->getVertexCount());  // ensure something was loaded

        return pMeshes;
    }

    template <typename TMaterialCreateInfo>
    void makeColorMesh(Model::Base& model, std::vector<Mesh::Color*>& pMeshes, TMaterialCreateInfo* pMaterialCreateInfo,
                       std::shared_ptr<Instance::Base>& pInstanceData) {
        auto createInfo = model.getMeshCreateInfo();
        auto& pMesh = meshHandler().make<Model::ColorMesh>(meshHandler().colorMeshes_, &createInfo, pMaterialCreateInfo,
                                                           pInstanceData);
        model.addOffset(pMesh);
        pMeshes.push_back(pMesh.get());
    }

    template <typename TMaterialCreateInfo>
    void makeTextureMesh(Model::Base& model, std::vector<Mesh::Texture*>& pMeshes, TMaterialCreateInfo* pMaterialCreateInfo,
                         std::shared_ptr<Instance::Base>& pInstanceData) {
        auto createInfo = model.getMeshCreateInfo();
        auto& pMesh = meshHandler().make<Model::TextureMesh>(meshHandler().texMeshes_, &createInfo, pMaterialCreateInfo,
                                                             pInstanceData);
        model.addOffset(pMesh);
        pMeshes.push_back(pMesh.get());
    }

    // thread sync
    template <typename TMesh>
    void handleMeshes(std::unique_ptr<Model::Base>& pModel, std::vector<TMesh*>&& pMeshes, Model::CBACK& callback) {
        for (auto pMesh : pMeshes) {
            assert(pMesh->getStatus() == STATUS::PENDING_BUFFERS);
            pMesh->prepare();
        }
        pModel->postLoad(callback);
        // Add a visual helper mesh
        if (pModel->visualHelper_) {
            for (auto pMesh : pMeshes) meshHandler().makeTangentSpaceVisualHelper(pMesh, pModel->visualHelperLineSize_);
        }
    }

    void makeTexture(const tinyobj::material_t& tinyobj_mat, const std::string& modelDirectory,
                     Material::CreateInfo* pCreateInfo);

    // Mesh futures
    template <typename T>
    void checkFutures(std::unique_ptr<Scene::Base>& pScene, std::unordered_map<Model::INDEX, T>& futuresMap) {
        // Check futures
        if (!futuresMap.empty()) {
            for (auto it = futuresMap.begin(); it != futuresMap.end();) {
                // Only check futures for models in the scene.
                // if (getModel(it->first)->getSceneOffset() != pScene->getOffset()) {
                //    ++it;
                //    continue;
                //}

                auto& future = it->second.first;  // loading future

                // Check the status but don't wait...
                auto status = future.wait_for(std::chrono::seconds(0));

                if (status == std::future_status::ready) {
                    auto& callback = it->second.second;  // makeModel callback
                    auto& pModel = getModel(it->first);  // model for future & callback

                    // Add the model meshes to the scene.
                    handleMeshes(pModel, future.get(), callback);

                    // Remove the future from the list if all goes well.
                    it = futuresMap.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    std::vector<std::unique_ptr<Model::Base>> pModels_;
    std::unordered_map<Model::INDEX, std::pair<std::future<std::vector<Mesh::Color*>>, Model::CBACK>> ldgColorFutures_;
    std::unordered_map<Model::INDEX, std::pair<std::future<std::vector<Mesh::Texture*>>, Model::CBACK>> ldgTexFutures_;
};

}  // namespace Model

#endif  // !MODEL_HANDLER_H
