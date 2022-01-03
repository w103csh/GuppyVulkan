/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef SCENE_H
#define SCENE_H

#include <set>
#include <vulkan/vulkan.hpp>

#include "Handlee.h"
#include "Mesh.h"
#include "Model.h"
#include "SelectionManager.h"

namespace Scene {

using index = uint8_t;

class Handler;

class Base : public Handlee<Scene::Handler> {
    friend class Handler;

   public:
    Base(Scene::Handler& handler, const index offset, bool makeFaceSelection);
    virtual ~Base();

    const index OFFSET;

    void addMeshIndex(const MESH type, const Mesh::index offset);
    void addModelIndex(const Model::index offset);

    void record(const RENDER_PASS& passType, const PIPELINE& pipelineType,
                const std::shared_ptr<Pipeline::BindData>& pipelineBindData, const vk::CommandBuffer& priCmd,
                const vk::CommandBuffer& secCmd, const uint8_t frameIndex,
                const Descriptor::Set::BindData* pDescSetBindData = nullptr);

    // SELECTION
    inline const std::unique_ptr<Face>& getFaceSelection() { return pSelectionManager_->getFace(); }
    void select(const Ray& ray);

    void destroy();

    // TODO: move these
    uint32_t moonOffset;
    uint32_t starsOffset;
    uint32_t posLgtCubeShdwOffset;
    uint32_t debugFrustumOffset;
    uint32_t volLgtBoxesOffset;

   private:
    std::set<Mesh::index> colorOffsets_;
    std::set<Mesh::index> lineOffsets_;
    std::set<Mesh::index> texOffsets_;
    std::set<Model::index> modelOffsets_;

    // Selection
    std::unique_ptr<Selection::Manager> pSelectionManager_;
};

}  // namespace Scene

#endif  // !SCENE_H