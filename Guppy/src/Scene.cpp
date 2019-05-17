
#include <iterator>

#include "Scene.h"

#include "Face.h"
#include "MeshHandler.h"
#include "PipelineHandler.h"
#include "RenderPass.h"
#include "SceneHandler.h"
#include "SelectionManager.h"
#include "Shell.h"
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

void Scene::Base::record(const uint8_t& frameIndex, std::unique_ptr<RenderPass::Base>& pPass) {
    // if (pPass->data.tests[frameIndex] == 0) {
    //    return;
    //}
    pPass->beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    // pPass->beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    auto& priCmd = pPass->data.priCmds[frameIndex];
    auto& secCmd = pPass->data.secCmds[frameIndex];

    auto it = pPass->PIPELINE_TYPES.begin();
    while (it != pPass->PIPELINE_TYPES.end()) {
        const auto& pipelineType = (*it);

        switch (pipelineType) {
            case PIPELINE::PBR_COLOR:
            case PIPELINE::TRI_LIST_COLOR: {
                for (auto& pMesh : handler().meshHandler().getColorMeshes())
                    if (pMesh->PIPELINE_TYPE == pipelineType && pMesh->getStatus() == STATUS::READY)
                        pMesh->draw(priCmd, frameIndex);
            } break;
            case PIPELINE::PARALLAX_SIMPLE:
            case PIPELINE::PARALLAX_STEEP:
            case PIPELINE::PBR_TEX:
            case PIPELINE::BP_TEX_CULL_NONE:
            case PIPELINE::TRI_LIST_TEX: {
                for (auto& pMesh : handler().meshHandler().getTextureMeshes())
                    if (pMesh->PIPELINE_TYPE == pipelineType && pMesh->getStatus() == STATUS::READY)
                        pMesh->draw(priCmd, frameIndex);
            } break;
            case PIPELINE::LINE: {
                for (auto& pMesh : handler().meshHandler().getLineMeshes())
                    if (pMesh->PIPELINE_TYPE == pipelineType && pMesh->getStatus() == STATUS::READY)
                        pMesh->draw(priCmd, frameIndex);
            } break;
            default:;
        }

        std::advance(it, 1);
        if (it != pPass->PIPELINE_TYPES.end()) {
            vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
            // vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        }
    }

    pPass->endPass(frameIndex);
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