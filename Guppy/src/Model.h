/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef MODEL_H
#define MODEL_H

#include <functional>
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "ConstantsAll.h"
#include "Handlee.h"
#include "Helpers.h"
#include "Instance.h"
#include "Material.h"
#include "Mesh.h"
#include "Obj3dInst.h"

namespace Model {

class Handler;

class Base : public NonCopyable, public Handlee<Model::Handler>, public Obj3d::Instance {
   public:
    Base(Model::Handler &handler, const Model::index offset, const Model::CreateInfo *pCreateInfo,
         std::shared_ptr<::Instance::Obj3d::Base> &pInstanceData);
    virtual ~Base();

    const std::string MODEL_PATH;
    const PIPELINE PIPELINE_TYPE;

    constexpr auto getOffset() const { return offset_; }
    constexpr const auto &getSettings() const { return settings_; }

    void postLoad(Model::cback callback);

    const std::vector<Model::index> &getMeshOffsets(MESH type);

    // TODO: Private ? Friend ?
    void addMeshOffset(std::unique_ptr<Mesh::Color> &pMesh);
    void addMeshOffset(std::unique_ptr<Mesh::Line> &pMesh);
    void addMeshOffset(std::unique_ptr<Mesh::Texture> &pMesh);

    inline Model::CreateInfo getMeshCreateInfo() {
        Model::CreateInfo createInfo = {};
        createInfo.pipelineType = PIPELINE_TYPE;
        createInfo.settings = settings_;
        return createInfo;
    }

    void allMeshAction(std::function<void(Mesh::Base *)> action);

    // STATUS status;

   private:
    void updateAggregateBoundingBox(Mesh::Base *pMesh);

    Model::index offset_;
    Mesh::Settings settings_;

    std::vector<Model::index> colorOffsets_;
    std::vector<Model::index> lineOffsets_;
    std::vector<Model::index> texOffsets_;
};

}  // namespace Model

#endif  // !MODEL_H