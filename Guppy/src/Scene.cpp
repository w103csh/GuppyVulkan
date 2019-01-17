
#include "Scene.h"

#include "CmdBufHandler.h"
#include "PipelineHandler.h"
#include "RenderPass.h"
#include "SceneHandler.h"
#include "TextureHandler.h"
#include "UIHandler.h"

namespace {

struct UBOTag {
    const char name[17] = "ubo tag";
} uboTag;

}  // namespace

Scene::Scene(size_t offset) : offset_(offset) {}

std::unique_ptr<ColorMesh>& Scene::moveMesh(const Game::Settings& settings, const Shell::Context& ctx,
                                            std::unique_ptr<ColorMesh> pMesh) {
    assert(pMesh->getStatus() == STATUS::PENDING_BUFFERS);

    auto offset = colorMeshes_.size();
    pMesh->setSceneData(offset);

    colorMeshes_.push_back(std::move(pMesh));
    colorMeshes_.back()->prepare(ctx.dev, settings, true);

    return colorMeshes_.back();
}

std::unique_ptr<LineMesh>& Scene::moveMesh(const Game::Settings& settings, const Shell::Context& ctx,
                                           std::unique_ptr<LineMesh> pMesh) {
    assert(pMesh->getStatus() == STATUS::PENDING_BUFFERS);

    auto offset = lineMeshes_.size();
    pMesh->setSceneData(offset);

    lineMeshes_.push_back(std::move(pMesh));
    lineMeshes_.back()->prepare(ctx.dev, settings, true);

    return lineMeshes_.back();
}

std::unique_ptr<TextureMesh>& Scene::moveMesh(const Game::Settings& settings, const Shell::Context& ctx,
                                              std::unique_ptr<TextureMesh> pMesh) {
    assert(pMesh->getStatus() == STATUS::PENDING_BUFFERS);

    auto offset = texMeshes_.size();
    pMesh->setSceneData(offset);

    texMeshes_.push_back(std::move(pMesh));
    texMeshes_.back()->prepare(ctx.dev, settings, true);

    return texMeshes_.back();
}

void Scene::removeMesh(std::unique_ptr<Mesh>& pMesh) {
    // TODO...
    assert(false);
}

void Scene::record(const Shell::Context& ctx, const uint8_t& frameIndex, std::unique_ptr<RenderPass::Base>& pPass) {
    pPass->beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT | VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
                     VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    auto& priCmd = pPass->data.priCmds[frameIndex];
    auto& secCmd = pPass->data.secCmds[frameIndex];

    // **********************
    //  TRI_LIST_TEX PIPELINE
    // **********************

    for (auto& pMesh : texMeshes_) {
        if (pMesh->getStatus() == STATUS::READY) {
            pPass->beginSecondary(frameIndex);
            pMesh->draw(secCmd, frameIndex);
        }
    }
    pPass->endSecondary(frameIndex, priCmd);

    vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);

    // **********************
    //  LINE PIPELINE
    // **********************

    for (auto& pMesh : lineMeshes_) {
        if (pMesh->getStatus() == STATUS::READY) {
            pMesh->draw(priCmd, frameIndex);
        }
    }

    vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);

    // **********************
    //  TRI_LIST_COLOR PIPELINE
    // **********************

    for (auto& pMesh : colorMeshes_) {
        if (pMesh->getStatus() == STATUS::READY) {
            pMesh->draw(priCmd, frameIndex);
        }
    }

    UIHandler::draw(priCmd, frameIndex);

    pPass->endPass(frameIndex);
}

void Scene::update(const Game::Settings& settings, const Shell::Context& ctx) {
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
                pMesh->prepare(ctx.dev, settings, true);

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
            pMesh->prepare(ctx.dev, settings, false);
        }
    }
}

void Scene::destroy(const VkDevice& dev) {
    // mesh
    for (auto& pMesh : colorMeshes_) pMesh->destroy(dev);
    colorMeshes_.clear();
    for (auto& pMesh : lineMeshes_) pMesh->destroy(dev);
    lineMeshes_.clear();
    for (auto& pMesh : texMeshes_) pMesh->destroy(dev);
    texMeshes_.clear();
}