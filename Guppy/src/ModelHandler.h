#ifndef MODEL_HANDLER_H
#define MODEL_HANDLER_H

#include <future>
#include <unordered_map>
#include <vector>

#include "Axes.h"
#include "Model.h"
#include "Singleton.h"
#include "Vertex.h"

class ColorMesh;
class Shell;
class TextureMesh;

class ModelHandler : Singleton {
   public:
    static void init(Shell* sh, const Game::Settings& settings);
    static inline void destroy() { inst_.reset(); }

    // color
    static void makeModel(ModelCreateInfo* pCreateInfo, std::unique_ptr<Scene>& pScene);
    // texture
    static void makeModel(ModelCreateInfo* pCreateInfo, std::unique_ptr<Scene>& pScene,
                          std::shared_ptr<Texture::Data> pTexture);

    static std::unique_ptr<Model>& getModel(MODEL_INDEX offset) {
        assert(offset < inst_.pModels_.size());
        return inst_.pModels_[offset];
    }

    static void update(std::unique_ptr<Scene>& pScene);

   private:
    ModelHandler() : sh_(nullptr){};  // Prevent construction
    ~ModelHandler(){};                // Prevent construction
    static ModelHandler inst_;
    void reset() override{};

    Shell* sh_;                // TODO: shared_ptr
    Game::Settings settings_;  // TODO: shared_ptr

    // Mesh futures
    template <typename T>
    void checkFutures(std::unique_ptr<Scene>& pScene, std::unordered_map<MODEL_INDEX, T>& futuresMap) {
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

    template <typename T>
    void handleMeshes(std::unique_ptr<Scene>& pScene, std::unique_ptr<Model>& pModel, std::vector<T*>&& pMeshes) {
        for (auto& pMesh : pMeshes) {
            // Turn mesh into a unique pointer
            auto p = std::unique_ptr<T>(pMesh);

            // Add a visual helper mesh
            if (pModel->visualHelper_) {
                MeshCreateInfo meshCreateInfo = {};
                meshCreateInfo.isIndexed = false;
                meshCreateInfo.model = p->getData().model;
                std::unique_ptr<LineMesh> pVH = std::make_unique<VisualHelper>(&meshCreateInfo, p);

                auto& rp = pScene->moveMesh(inst_.settings_, sh_->context(), std::move(pVH));
                pModel->addOffset(rp);  // Separate visual helper offset???
            }

            // Add mesh to scene
            auto& rp = pScene->moveMesh(inst_.settings_, sh_->context(), std::move(p));
            // Store the offset of the mesh into the scene buffers after moving.
            pModel->addOffset(rp);
        }
    }

    std::vector<std::unique_ptr<Model>> pModels_;
    std::unordered_map<MODEL_INDEX, std::pair<std::future<std::vector<ColorMesh*>>, MODEL_CALLBACK>> ldgColorFutures_;
    std::unordered_map<MODEL_INDEX, std::pair<std::future<std::vector<TextureMesh*>>, MODEL_CALLBACK>> ldgTexFutures_;
};

#endif  // !MODEL_HANDLER_H
