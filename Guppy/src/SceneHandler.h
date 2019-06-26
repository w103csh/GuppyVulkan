#ifndef SCENE_HANDLER_H
#define SCENE_HANDLER_H

#include <memory>
#include <vector>

#include "ConstantsAll.h"
#include "Helpers.h"
#include "Mesh.h"
#include "Scene.h"
#include "Game.h"

namespace Scene {

// TODO: is this still necessary???
struct InvalidSceneResources {
    VkDescriptorPool pool = VK_NULL_HANDLE;
    std::vector<std::vector<VkDescriptorSet>> texSets = {};
    std::shared_ptr<DescriptorBufferResources> pDynUBORes = nullptr;
};

class Handler : public Game::Handler {
    friend class Base;
    class SelectionManager;

   public:
    Handler(Game* pGame);
    ~Handler();

    void init() override;
    inline void destroy() override {
        reset();
        cleanup();
    }

    SCENE_INDEX_TYPE makeScene(bool setActive = false, bool makeFaceSelection = false);

    inline std::unique_ptr<Scene::Base>& getActiveScene() {
        assert(activeSceneIndex_ < pScenes_.size());
        return pScenes_[activeSceneIndex_];
    }

    std::unique_ptr<Mesh::Color>& getColorMesh(size_t sceneOffset, size_t meshOffset);
    std::unique_ptr<Mesh::Line>& getLineMesh(size_t sceneOffset, size_t meshOffset);
    std::unique_ptr<Mesh::Texture>& getTextureMesh(size_t sceneOffset, size_t meshOffset);

    void cleanup();

   private:
    void reset() override;

    SCENE_INDEX_TYPE activeSceneIndex_;
    std::vector<std::unique_ptr<Scene::Base>> pScenes_;

    // global storage for clean up
    std::vector<InvalidSceneResources> invalidRes_;
};

}  // namespace Scene

#endif  // !SCENE_HANDLER_H
