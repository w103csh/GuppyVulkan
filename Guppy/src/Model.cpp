
#include "Model.h"

#include "Mesh.h"
#include "MeshHandler.h"
#include "ModelHandler.h"

Model::Base::Base(Model::Handler &handler, Model::CreateInfo *pCreateInfo)
    : Object3d(pCreateInfo->model),
      Handlee(handler),
      PIPELINE_TYPE(pCreateInfo->pipelineType),
      offset_(pCreateInfo->handlerOffset),
      modelPath_(pCreateInfo->modelPath),
      smoothNormals_(pCreateInfo->smoothNormals),
      visualHelper_(pCreateInfo->visualHelper),
      visualHelperLineSize_(pCreateInfo->visualHelperLineSize) {}

Model::Base::~Base() = default;

void Model::Base::postLoad(Model::CBACK callback) {
    allMeshAction([this](Mesh::Base *pMesh) { updateAggregateBoundingBox(pMesh); });
    // Invoke callback for say... model transformations. This is a pain in the ass.
    if (callback) callback(this);
}

void Model::Base::addOffset(std::unique_ptr<Mesh::Color> &pMesh) { colorOffsets_.push_back(pMesh->getOffset()); }
void Model::Base::addOffset(std::unique_ptr<Mesh::Line> &pMesh) { lineOffsets_.push_back(pMesh->getOffset()); }
void Model::Base::addOffset(std::unique_ptr<Mesh::Texture> &pMesh) { texOffsets_.push_back(pMesh->getOffset()); }

void Model::Base::allMeshAction(std::function<void(Mesh::Base *)> action) {
    for (auto &offset : colorOffsets_) {
        auto &pMesh = handler().meshHandler().getColorMesh(offset);
        action(pMesh.get());
    }
    for (auto &offset : lineOffsets_) {
        auto &pMesh = handler().meshHandler().getLineMesh(offset);
        action(pMesh.get());
    }
    for (auto &offset : texOffsets_) {
        auto &pMesh = handler().meshHandler().getTextureMesh(offset);
        action(pMesh.get());
    }
}

void Model::Base::transform(const glm::mat4 t) {
    Object3d::transform(t);
    allMeshAction([&t](Mesh::Base *pMesh) { pMesh->transform(t); });
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

void Model::Base::updateAggregateBoundingBox(Mesh::Base *pMesh) { updateBoundingBox(pMesh->getBoundingBoxMinMax(false)); }

Model::ColorMesh::ColorMesh(Mesh::Handler &handler, Model::CreateInfo *pCreateInfo,
                            std::shared_ptr<Material::Base> &pMaterial)
    : Mesh::Color(handler, (std::string)(pCreateInfo->modelPath + "Color Mesh"), pCreateInfo, pMaterial) {
    status_ = STATUS::PENDING_VERTICES;
}

Model::TextureMesh::TextureMesh(Mesh::Handler &handler, Model::CreateInfo *pCreateInfo,
                                std::shared_ptr<Material::Base> &pMaterial)
    : Mesh::Texture(handler, (std::string)(pCreateInfo->modelPath + "Texture Mesh"), pCreateInfo, pMaterial) {
    status_ = STATUS::PENDING_VERTICES;
}
