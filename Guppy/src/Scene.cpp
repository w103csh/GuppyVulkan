
#include "Scene.h"

#include "PipelineHandler.h"
#include "RenderPass.h"
#include "SceneHandler.h"
#include "Shell.h"
#include "TextureHandler.h"

namespace {

struct UBOTag {
    const char name[17] = "ubo tag";
} uboTag;

}  // namespace

Scene::Base::Base(const Scene::Handler& handler, size_t offset) : Handlee(handler), offset_(offset) {}

Scene::Base::~Base() = default;

std::unique_ptr<ColorMesh>& Scene::Base::moveMesh(std::unique_ptr<ColorMesh> pMesh) {
    assert(pMesh->getStatus() == STATUS::PENDING_BUFFERS);

    auto offset = colorMeshes_.size();
    pMesh->setSceneData(offset);

    colorMeshes_.push_back(std::move(pMesh));
    colorMeshes_.back()->prepare(handler_, true);

    return colorMeshes_.back();
}

std::unique_ptr<LineMesh>& Scene::Base::moveMesh(std::unique_ptr<LineMesh> pMesh) {
    assert(pMesh->getStatus() == STATUS::PENDING_BUFFERS);

    auto offset = lineMeshes_.size();
    pMesh->setSceneData(offset);

    lineMeshes_.push_back(std::move(pMesh));
    lineMeshes_.back()->prepare(handler_, true);

    return lineMeshes_.back();
}

std::unique_ptr<TextureMesh>& Scene::Base::moveMesh(std::unique_ptr<TextureMesh> pMesh) {
    assert(pMesh->getStatus() == STATUS::PENDING_BUFFERS);

    auto offset = texMeshes_.size();
    pMesh->setSceneData(offset);

    texMeshes_.push_back(std::move(pMesh));
    texMeshes_.back()->prepare(handler_, true);

    return texMeshes_.back();
}

void Scene::Base::removeMesh(std::unique_ptr<Mesh>& pMesh) {
    // TODO...
    assert(false);
}

void Scene::Base::record(const uint8_t& frameIndex, std::unique_ptr<RenderPass::Base>& pPass) {
    // if (pPass->data.tests[frameIndex] == 0) {
    //    return;
    //}

    pPass->beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    // pPass->beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    auto& priCmd = pPass->data.priCmds[frameIndex];
    auto& secCmd = pPass->data.secCmds[frameIndex];

    auto penultimate = std::prev(pPass->PIPELINE_TYPES.end());
    for (auto it = pPass->PIPELINE_TYPES.begin(); it != pPass->PIPELINE_TYPES.end(); std::advance(it, 1)) {
        const auto& pipelineType = (*it);

        switch (pipelineType) {
            case PIPELINE::LINE: {
                for (auto& pMesh : lineMeshes_)
                    if (pMesh->PIPELINE_TYPE == pipelineType && pMesh->getStatus() == STATUS::READY)
                        pMesh->draw(priCmd, frameIndex);
            } break;
            case PIPELINE::TRI_LIST_COLOR:
            case PIPELINE::PBR_COLOR: {
                for (auto& pMesh : colorMeshes_)
                    if (pMesh->PIPELINE_TYPE == pipelineType && pMesh->getStatus() == STATUS::READY)
                        pMesh->draw(priCmd, frameIndex);
            } break;
            case PIPELINE::TRI_LIST_TEX: {
                for (auto& pMesh : texMeshes_)
                    if (pMesh->PIPELINE_TYPE == pipelineType && pMesh->getStatus() == STATUS::READY)
                        pMesh->draw(priCmd, frameIndex);
            } break;
        }

        if (it != penultimate) {
            vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
            // vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        }
    }

    pPass->endPass(frameIndex);
}

void Scene::Base::update() {
    // Check loading futures
    if (!ldgFutures_.empty()) {
        auto itFut = ldgFutures_.begin();

        while (itFut != ldgFutures_.end()) {
            auto& fut = (*itFut);

            // Check the status but don't wait...
            auto status = fut.wait_for(std::chrono::seconds(0));

            if (status == std::future_status::ready) {
                auto pMesh = fut.get();
                // Future is ready so prepare the mesh...
                pMesh->prepare(handler_, true);

                // Remove the future from the list if all goes well.
                itFut = ldgFutures_.erase(itFut);

            } else {
                ++itFut;
            }
        }
    }

    // TODO: fix all this ready stuff
    for (auto& pMesh : texMeshes_) {
        if (pMesh->getStatus() == STATUS::PENDING_TEXTURE) {
            pMesh->prepare(handler_, false);
        }
    }
}

void Scene::Base::destroy() {
    const auto& dev = handler_.shell().context().dev;
    // mesh
    for (auto& pMesh : colorMeshes_) pMesh->destroy(dev);
    colorMeshes_.clear();
    for (auto& pMesh : lineMeshes_) pMesh->destroy(dev);
    lineMeshes_.clear();
    for (auto& pMesh : texMeshes_) pMesh->destroy(dev);
    texMeshes_.clear();
}