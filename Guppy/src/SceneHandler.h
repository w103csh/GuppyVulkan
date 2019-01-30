#ifndef SCENE_HANDLER_H
#define SCENE_HANDLER_H

#include <memory>
#include <vector>

#include "Constants.h"
#include "Helpers.h"
#include "Scene.h"
#include "Game.h"

class ColorMesh;
class LineMesh;
class TextureMesh;
// clang-format off
namespace Selection { class Manager; }
// clang-format on

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
    inline void destroy() {
        reset();
        cleanup();
    }

    SCENE_INDEX_TYPE makeScene(bool setActive = false, bool makeFaceSelection = false);

    inline std::unique_ptr<Scene::Base>& getActiveScene() {
        assert(activeSceneIndex_ < pScenes_.size());
        return pScenes_[activeSceneIndex_];
    }

    inline std::unique_ptr<ColorMesh>& getColorMesh(size_t sceneOffset, size_t meshOffset) {
        return pScenes_[sceneOffset]->getColorMesh(meshOffset);
    }
    inline std::unique_ptr<LineMesh>& getLineMesh(size_t sceneOffset, size_t meshOffset) {
        return pScenes_[sceneOffset]->getLineMesh(meshOffset);
    }
    inline std::unique_ptr<TextureMesh>& getTextureMesh(size_t sceneOffset, size_t meshOffset) {
        return pScenes_[sceneOffset]->getTextureMesh(meshOffset);
    }

    // PIPELINE
    void updatePipelineReferences(const PIPELINE& type, const VkPipeline& pipeline);

    // SELECTION
    const std::unique_ptr<Face>& getFaceSelection();
    void select(const Ray& ray);

    void updateDescriptorSets(SCENE_INDEX_TYPE offset, bool isUpdate = true);
    void cleanup();

   private:
    void reset() override;

    SCENE_INDEX_TYPE activeSceneIndex_;
    std::vector<std::unique_ptr<Scene::Base>> pScenes_;

    // global storage for clean up
    std::vector<InvalidSceneResources> invalidRes_;

    // Selection
    std::unique_ptr<Selection::Manager> pSelectionManager_;
};

}  // namespace Scene

#endif  // !SCENE_HANDLER_H
