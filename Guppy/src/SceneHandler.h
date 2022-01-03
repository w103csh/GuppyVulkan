/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef SCENE_HANDLER_H
#define SCENE_HANDLER_H

#include <memory>
#include <vector>

#include <Common/Helpers.h>

#include "CdlodRenderer.h"
#include "ConstantsAll.h"
#include "Game.h"
#include "Mesh.h"
#include "OceanRenderer.h"
#include "Scene.h"

// clang-format off
namespace GraphicsWork { class Base; }
// clang-format on

namespace Scene {

class Handler : public Game::Handler {
    friend class Base;
    class SelectionManager;

   public:
    Handler(Game* pGame);
    ~Handler();

    void init() override;
    void destroy() override;
    void tick() override;
    void frame() override;

    std::unique_ptr<Scene::Base>& makeScene(bool setActive = false, bool makeFaceSelection = false);
    inline std::unique_ptr<Scene::Base>& getActiveScene() {
        assert(activeSceneIndex_ < pScenes_.size());
        return pScenes_[activeSceneIndex_];
    }

    std::unique_ptr<Mesh::Color>& getColorMesh(size_t sceneOffset, size_t meshOffset);
    std::unique_ptr<Mesh::Line>& getLineMesh(size_t sceneOffset, size_t meshOffset);
    std::unique_ptr<Mesh::Texture>& getTextureMesh(size_t sceneOffset, size_t meshOffset);

    void recordRenderer(const PASS passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                        const vk::CommandBuffer& cmd);

    void cleanup();

    // RENDERERS
    Cdlod::Renderer::Debug cdlodDbgRenderer;
    Ocean::Renderer ocnRenderer;
    // Just put this directly on the handler. I probably will never actually use a scene at this point.
    std::vector<std::unique_ptr<GraphicsWork::Base>> pGraphicsWork;

   private:
    void reset() override;
    void createGraphicsWork();

    index activeSceneIndex_;
    std::vector<std::unique_ptr<Scene::Base>> pScenes_;
};

}  // namespace Scene

#endif  // !SCENE_HANDLER_H
