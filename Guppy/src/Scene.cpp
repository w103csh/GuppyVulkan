
#include "Scene.h"

#include <iterator>

#include "Face.h"
#include "RenderPass.h"
#include "Shell.h"
// HANDLERS
#include "MeshHandler.h"
#include "PipelineHandler.h"
#include "SceneHandler.h"
#include "SelectionManager.h"
#include "TextureHandler.h"

namespace {

struct UBOTag {
    const char name[17] = "ubo tag";
} uboTag;

}  // namespace

Scene::Base::Base(Scene::Handler& handler, size_t offset, bool makeFaceSelection)
    : Handlee(handler),
      offset_(offset),
      pSelectionManager_(std::make_unique<Selection::Manager>(handler, makeFaceSelection)) {}

Scene::Base::~Base() = default;

void Scene::Base::record(const RENDER_PASS& passType, const PIPELINE& pipelineType,
                         const std::shared_ptr<Pipeline::BindData>& pipelineBindData, const VkCommandBuffer& priCmd,
                         const VkCommandBuffer& secCmd, const uint8_t& frameIndex) {
    switch (pipelineType) {
        case PIPELINE::PBR_COLOR:
        case PIPELINE::CUBE:
        case PIPELINE::TRI_LIST_COLOR: {
            for (auto& pMesh : handler().meshHandler().getColorMeshes())
                if (pMesh->PIPELINE_TYPE == pipelineType && pMesh->getStatus() == STATUS::READY)
                    pMesh->draw(passType, pipelineBindData, priCmd, frameIndex);
        } break;
        case PIPELINE::PARALLAX_SIMPLE:
        case PIPELINE::PARALLAX_STEEP:
        case PIPELINE::PBR_TEX:
        case PIPELINE::BP_TEX_CULL_NONE:
        case PIPELINE::TRI_LIST_TEX: {
            for (auto& pMesh : handler().meshHandler().getTextureMeshes())
                if (pMesh->PIPELINE_TYPE == pipelineType && pMesh->getStatus() == STATUS::READY)
                    pMesh->draw(passType, pipelineBindData, priCmd, frameIndex);
        } break;
        case PIPELINE::LINE: {
            for (auto& pMesh : handler().meshHandler().getLineMeshes())
                if (pMesh->PIPELINE_TYPE == pipelineType && pMesh->getStatus() == STATUS::READY)
                    pMesh->draw(passType, pipelineBindData, priCmd, frameIndex);
        } break;
        default:;
    }
}

void Scene::Base::select(const Ray& ray) {
    if (!pSelectionManager_->isEnabled()) return;

    float tMin = T_MAX;  // This is relative to the distance between ray.e & ray.d
    Face face;

    // TODO: pass the scene in, or better, fix it...
    pSelectionManager_->selectFace(ray, tMin, handler().meshHandler().getColorMeshes(), face);
    pSelectionManager_->selectFace(ray, tMin, handler().meshHandler().getTextureMeshes(), face);

    pSelectionManager_->updateFace((tMin < T_MAX) ? std::make_unique<Face>(face) : nullptr);
}

void Scene::Base::destroy() {}