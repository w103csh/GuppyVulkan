#ifndef MODEL_HANDLER_H
#define MODEL_HANDLER_H

#include <future>
#include <unordered_map>
#include <vector>

#include "Axes.h"
#include "Game.h"
#include "Model.h"
#include "Scene.h"
#include "Vertex.h"

class ColorMesh;
class TextureMesh;

namespace Model {

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame);

    void init() override{};
    inline void destroy() { reset(); }

    void makeModel(Model::CreateInfo* pCreateInfo, std::unique_ptr<Scene::Base>& pScene);

    std::unique_ptr<Model::Base>& getModel(Model::INDEX offset) {
        assert(offset < pModels_.size());
        return pModels_[offset];
    }

    void update(std::unique_ptr<Scene::Base>& pScene);

   private:
    void reset() override{};

    void makeColorModel(Model::CreateInfo* pCreateInfo, std::unique_ptr<Scene::Base>& pScene);
    void makeTextureModel(Model::CreateInfo* pCreateInfo, std::unique_ptr<Scene::Base>& pScene);

    // LOADING
    std::vector<ColorMesh*> loadColor(const Material::Info& materialInfo, Model::Base& model);
    std::vector<TextureMesh*> loadTexture(const Material::Info& materialInfo, Model::Base& model);

    // thread sync
    template <typename T>
    void handleMeshes(std::unique_ptr<Scene::Base>& pScene, std::unique_ptr<Model::Base>& pModel,
                      std::vector<T*>&& pMeshes) {
        for (auto& pMesh : pMeshes) {
            // Turn mesh into a unique pointer
            auto p = std::unique_ptr<T>(pMesh);

            // Add a visual helper mesh
            if (pModel->visualHelper_) {
                MeshCreateInfo meshCreateInfo = {};
                meshCreateInfo.isIndexed = false;
                meshCreateInfo.model = p->getData().model;
                meshCreateInfo.pipelineType = pModel->PIPELINE_TYPE;
                std::unique_ptr<LineMesh> pVH = std::make_unique<VisualHelper>(&meshCreateInfo, p);

                auto& rp = pScene->moveMesh(std::move(pVH));
                pModel->addOffset(rp);  // Separate visual helper offset???
            }

            // Add mesh to scene
            auto& rp = pScene->moveMesh(std::move(p));
            // Store the offset of the mesh into the scene buffers after moving.
            pModel->addOffset(rp);
        }
    }

    // Mesh futures
    template <typename T>
    void checkFutures(std::unique_ptr<Scene::Base>& pScene, std::unordered_map<Model::INDEX, T>& futuresMap) {
        // Check futures
        if (!futuresMap.empty()) {
            for (auto it = futuresMap.begin(); it != futuresMap.end();) {
                // Only check futures for models in the scene.
                if (getModel(it->first)->getSceneOffset() != pScene->getOffset()) {
                    ++it;
                    continue;
                }

                auto& future = it->second.first;  // loading future

                // Check the status but don't wait...
                auto status = future.wait_for(std::chrono::seconds(0));

                if (status == std::future_status::ready) {
                    auto& callback = it->second.second;  // makeModel callback
                    auto& pModel = getModel(it->first);  // model for future & callback

                    // Add the model meshes to the scene.
                    handleMeshes(pScene, pModel, future.get());
                    pModel->postLoad(callback);

                    // Remove the future from the list if all goes well.
                    it = futuresMap.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    std::vector<std::unique_ptr<Model::Base>> pModels_;
    std::unordered_map<Model::INDEX, std::pair<std::future<std::vector<ColorMesh*>>, Model::CBACK>> ldgColorFutures_;
    std::unordered_map<Model::INDEX, std::pair<std::future<std::vector<TextureMesh*>>, Model::CBACK>> ldgTexFutures_;
};

}  // namespace Model

#endif  // !MODEL_HANDLER_H
