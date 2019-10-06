
#include "Model.h"

#include "Mesh.h"
// HANDLERS
#include "MeshHandler.h"
#include "ModelHandler.h"

Model::Base::Base(Model::Handler &handler, const Model::index offset, const Model::CreateInfo *pCreateInfo,
                  std::shared_ptr<::Instance::Obj3d::Base> &pInstanceData)
    : Handlee(handler),
      Obj3d::Instance(pInstanceData),
      MODEL_PATH(pCreateInfo->modelPath),
      PIPELINE_TYPE(pCreateInfo->pipelineType),
      offset_(offset),
      settings_(pCreateInfo->settings) {}

Model::Base::~Base() = default;

void Model::Base::postLoad(Model::cback callback) {
    allMeshAction([this](Mesh::Base *pMesh) { updateAggregateBoundingBox(pMesh); });
    // Invoke callback for say... model transformations. This is a pain in the ass
    // and slow.
    if (callback) callback(this);
    allMeshAction([this](Mesh::Base *pMesh) {  //
        handler().meshHandler().updateInstanceData(pMesh->getInstanceDataInfo());
    });
}

void Model::Base::addMeshOffset(std::unique_ptr<Mesh::Color> &pMesh) { colorOffsets_.push_back(pMesh->getOffset()); }
void Model::Base::addMeshOffset(std::unique_ptr<Mesh::Line> &pMesh) { lineOffsets_.push_back(pMesh->getOffset()); }
void Model::Base::addMeshOffset(std::unique_ptr<Mesh::Texture> &pMesh) { texOffsets_.push_back(pMesh->getOffset()); }

void Model::Base::allMeshAction(std::function<void(Mesh::Base *)> action) {
    for (auto &offset : colorOffsets_) {
        auto &pMesh = handler().meshHandler().getColorMesh(offset);
        action(pMesh.get());
        handler().meshHandler().updateMesh(pMesh);
    }
    for (auto &offset : lineOffsets_) {
        auto &pMesh = handler().meshHandler().getLineMesh(offset);
        action(pMesh.get());
        handler().meshHandler().updateMesh(pMesh);
    }
    for (auto &offset : texOffsets_) {
        auto &pMesh = handler().meshHandler().getTextureMesh(offset);
        action(pMesh.get());
        handler().meshHandler().updateMesh(pMesh);
    }
}

const std::vector<Model::index> &Model::Base::getMeshOffsets(MESH type) {
    switch (type) {
        case MESH::COLOR:
            return colorOffsets_;
        case MESH::LINE:
            return lineOffsets_;
        case MESH::TEXTURE:
            return texOffsets_;
        default:
            assert(false);
            exit(EXIT_FAILURE);
    }
}

void Model::Base::updateAggregateBoundingBox(Mesh::Base *pMesh) {
    pInstObj3d_->updateBoundingBox(pMesh->getBoundingBoxMinMax(false));
}
