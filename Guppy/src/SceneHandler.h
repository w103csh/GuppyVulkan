#ifndef SCENE_HANDLER_H
#define SCENE_HANDLER_H

#include <memory>
#include <vector>

#include "Helpers.h"
#include "Singleton.h"
#include "Scene.h"

class MyShell;

class SceneHandler : Singleton {
   public:
    static void init(const MyShell::Context& ctx, const Game::Settings& settings);
    static inline void destroy() {
        inst_.reset();
        inst_.cleanupOldResources();
    }

    static uint8_t makeScene(const UniformBufferResources& uboResource, bool setActive = false);

    static std::unique_ptr<Scene>& getActiveScene() {
        assert(inst_.activeSceneIndex_ < inst_.pScenes_.size());
        return inst_.pScenes_[inst_.activeSceneIndex_];
    }

    static std::unique_ptr<ColorMesh>& getColorMesh(size_t sceneOffset, size_t meshOffset) {
        return inst_.pScenes_[sceneOffset]->getColorMesh(meshOffset);
    }
    static std::unique_ptr<ColorMesh>& getLineMesh(size_t sceneOffset, size_t meshOffset) {
        return inst_.pScenes_[sceneOffset]->getLineMesh(meshOffset);
    }
    static std::unique_ptr<TextureMesh>& getTextureMesh(size_t sceneOffset, size_t meshOffset) {
        return inst_.pScenes_[sceneOffset]->getTextureMesh(meshOffset);
    }

    static void updateDescriptorResources(uint8_t offset, const UniformBufferResources* pUBORes = nullptr);

    static void destroyDescriptorResources(std::unique_ptr<DescriptorResources>& pRes);
    static void cleanupOldResources();

   private:
    SceneHandler();     // Prevent construction
    ~SceneHandler(){};  // Prevent construction
    static SceneHandler inst_;
    void reset() override;

    MyShell::Context ctx_;     // TODO: shared_ptr
    Game::Settings settings_;  // TODO: shared_ptr

    uint8_t activeSceneIndex_;
    std::vector<std::unique_ptr<Scene>> pScenes_;

    // global storage for clean up
    std::vector<std::unique_ptr<DescriptorResources>> oldDescRes_;
};

#endif  // !SCENE_HANDLER_H
