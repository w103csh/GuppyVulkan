#ifndef MODEL_HANDLER_H
#define MODEL_HANDLER_H

#include <future>
#include <unordered_map>
#include <vector>

#include "Model.h"
#include "Singleton.h"
#include "Vertex.h"

class ColorMesh;
class MyShell;
class TextureMesh;

class ModelHandler : Singleton {
   public:
    static void init(MyShell* sh);
    static inline void destroy() { inst_.reset(); }

    // color
    static void makeModel(std::unique_ptr<Scene>& pScene, std::string modelPath, Material material, glm::mat4 model,
                          bool async = true, Model::CALLBK callback = nullptr);
    // texture
    static void makeModel(std::unique_ptr<Scene>& pScene, std::string modelPath, Material material, glm::mat4 model,
                          std::shared_ptr<Texture::Data> pTexture = nullptr, bool async = true, Model::CALLBK callback = nullptr);

    static std::unique_ptr<Model>& getModel(Model::IDX offset) {
        assert(offset < inst_.pModels_.size());
        return inst_.pModels_[offset];
    }

    static void update(std::unique_ptr<Scene>& pScene);

   private:
    ModelHandler() : sh_(nullptr){};  // Prevent construction
    ~ModelHandler(){};                // Prevent construction
    static ModelHandler inst_;
    void reset() override{};

    MyShell* sh_;  // TODO: shared_ptr

    // Mesh futures
    template <typename T>
    void checkFutures(std::unique_ptr<Scene>& pScene, std::unordered_map<Model::IDX, T>& futuresMap) {
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
            // Add mesh to scene
            auto& rp = pScene->moveMesh(sh_->context(), std::unique_ptr<T>(pMesh));
            // Store the offset of the mesh into the scene buffers on the model after moving.
            pModel->addOffset(rp);
        }
    }

    std::vector<std::unique_ptr<Model>> pModels_;
    std::unordered_map<Model::IDX, std::pair<std::future<std::vector<ColorMesh*>>, Model::CALLBK>> ldgColorFutures_;
    std::unordered_map<Model::IDX, std::pair<std::future<std::vector<TextureMesh*>>, Model::CALLBK>> ldgTexFutures_;
};

#endif  // !MODEL_HANDLER_H
