
//#include "FileLoader.h"
//#include "TextureHandler.h"
//#include "Scene.h"
//#include "Material.h"

#include "Mesh.h"
#include "ModelHandler.h"
#include "Model.h"
#include "Object3d.h"
#include "SceneHandler.h"

Model::Base::Base(const Model::Handler &handler, Model::CreateInfo *pCreateInfo, Model::INDEX sceneOffset)
    : Object3d(pCreateInfo->model),
      Handlee(handler),
      PIPELINE_TYPE(pCreateInfo->pipelineType),
      handlerOffset_(pCreateInfo->handlerOffset),
      modelPath_(pCreateInfo->modelPath),
      sceneOffset_(sceneOffset),
      smoothNormals_(pCreateInfo->smoothNormals),
      visualHelper_(pCreateInfo->visualHelper) {
    assert(sceneOffset_ < Model::INDEX_MAX);
}

Model::Base::~Base() = default;

void Model::Base::postLoad(Model::CBACK callback) {
    allMeshAction([this](Mesh *pMesh) { updateAggregateBoundingBox(pMesh); });
    // Invoke callback for say... model transformations. This is a pain in the ass.
    if (callback) callback(this);
}

void Model::Base::addOffset(std::unique_ptr<ColorMesh> &pMesh) { colorOffsets_.push_back(pMesh->getOffset()); }
void Model::Base::addOffset(std::unique_ptr<LineMesh> &pMesh) { lineOffsets_.push_back(pMesh->getOffset()); }
void Model::Base::addOffset(std::unique_ptr<TextureMesh> &pMesh) { texOffsets_.push_back(pMesh->getOffset()); }

void Model::Base::allMeshAction(std::function<void(Mesh *)> action) {
    for (auto &offset : colorOffsets_) {
        auto &pMesh = handler_.sceneHandler().getColorMesh(sceneOffset_, offset);
        action(pMesh.get());
    }
    for (auto &offset : lineOffsets_) {
        auto &pMesh = handler_.sceneHandler().getLineMesh(sceneOffset_, offset);
        action(pMesh.get());
    }
    for (auto &offset : texOffsets_) {
        auto &pMesh = handler_.sceneHandler().getTextureMesh(sceneOffset_, offset);
        action(pMesh.get());
    }
}

void Model::Base::transform(const glm::mat4 t) {
    Object3d::transform(t);
    allMeshAction([&t](Mesh *pMesh) { pMesh->transform(t); });
}

Model::INDEX Model::Base::getMeshOffset(MESH type, uint8_t offset) {
    switch (type) {
        case MESH::COLOR:
            assert(offset < colorOffsets_.size());
            return colorOffsets_[offset];
        case MESH::LINE:
            assert(offset < lineOffsets_.size());
            return lineOffsets_[offset];
        case MESH::TEXTURE:
            assert(offset < texOffsets_.size());
            return texOffsets_[offset];
        default:
            throw std::runtime_error("Unhandled mesh type.");
    }
}

void Model::Base::updateAggregateBoundingBox(Mesh *pMesh) { updateBoundingBox(pMesh->getBoundingBoxMinMax(false)); }
