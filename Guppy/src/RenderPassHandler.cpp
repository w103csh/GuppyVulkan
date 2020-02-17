/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "RenderPassHandler.h"

#include <algorithm>

#include "Plane.h"
#include "RenderPassCubeMap.h"
#include "RenderPassDeferred.h"
#ifdef USE_DEBUG_UI
#include "RenderPassImGui.h"
#endif
#include "RenderPassScreenSpace.h"
#include "RenderPassShadow.h"
#include "ScreenSpace.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "ComputeHandler.h"
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "MeshHandler.h"
#include "PipelineHandler.h"
#include "TextureHandler.h"

namespace {

// clang-format off
const std::vector<PASS> DEFAULT = {
#if VK_USE_PLATFORM_MACOS_MVK || false
#if DO_PROJECTOR
    PASS::SAMPLER_PROJECT,
#endif
    PASS::DEFAULT,
    //PASS::SAMPLER_DEFAULT,
    //PASS::SCREEN_SPACE,
#else
    PASS::DEFERRED,
#endif
// UI pass needs to always be last since it
// is optional
#ifdef USE_DEBUG_UI
    PASS::IMGUI,
#endif
};
// clang-format on

}  // namespace

RenderPass::Handler::Handler(Game* pGame)
    : Game::Handler(pGame), frameIndex_(0), swpchnRes_{}, submitResources_{}, screenQuadOffset_(Mesh::BAD_OFFSET) {
    for (const auto& type : RenderPass::ALL) {
        // clang-format off
        switch (type) {
            case PASS::DEFAULT:                 pPasses_.emplace_back(std::make_unique<RenderPass::Base>                (std::ref(*this), static_cast<index>(pPasses_.size()), &RenderPass::DEFAULT_CREATE_INFO)); break;
            case PASS::SAMPLER_PROJECT:         pPasses_.emplace_back(std::make_unique<RenderPass::Base>                (std::ref(*this), static_cast<index>(pPasses_.size()), &RenderPass::PROJECT_CREATE_INFO)); break;
            case PASS::SAMPLER_DEFAULT:         pPasses_.emplace_back(std::make_unique<RenderPass::Base>                (std::ref(*this), static_cast<index>(pPasses_.size()), &RenderPass::SAMPLER_DEFAULT_CREATE_INFO)); break;
            case PASS::SCREEN_SPACE:            pPasses_.emplace_back(std::make_unique<RenderPass::ScreenSpace::Base>   (std::ref(*this), static_cast<index>(pPasses_.size()), &RenderPass::ScreenSpace::CREATE_INFO)); break;
            case PASS::SCREEN_SPACE_HDR_LOG:    pPasses_.emplace_back(std::make_unique<RenderPass::ScreenSpace::HdrLog> (std::ref(*this), static_cast<index>(pPasses_.size()))); break;
            case PASS::SCREEN_SPACE_BRIGHT:     pPasses_.emplace_back(std::make_unique<RenderPass::ScreenSpace::Bright> (std::ref(*this), static_cast<index>(pPasses_.size()))); break;
            case PASS::SCREEN_SPACE_BLUR_A:     pPasses_.emplace_back(std::make_unique<RenderPass::ScreenSpace::BlurA>  (std::ref(*this), static_cast<index>(pPasses_.size()))); break;
            case PASS::SCREEN_SPACE_BLUR_B:     pPasses_.emplace_back(std::make_unique<RenderPass::ScreenSpace::BlurB>  (std::ref(*this), static_cast<index>(pPasses_.size()))); break;
            case PASS::DEFERRED:                pPasses_.emplace_back(std::make_unique<RenderPass::Deferred::Base>      (std::ref(*this), static_cast<index>(pPasses_.size()))); break;
            case PASS::SHADOW:                  pPasses_.emplace_back(std::make_unique<RenderPass::Shadow::Default>     (std::ref(*this), static_cast<index>(pPasses_.size()))); break;
            case PASS::SKYBOX_NIGHT:            pPasses_.emplace_back(std::make_unique<RenderPass::CubeMap::SkyboxNight>(std::ref(*this), static_cast<index>(pPasses_.size()))); break;
            case PASS::IMGUI:
#ifdef USE_DEBUG_UI
                                                pPasses_.emplace_back(std::make_unique<RenderPass::ImGui>(              std::ref(*this), static_cast<index>(pPasses_.size())));
#endif
                break;
            default: assert(false && "Unhandled pass type"); exit(EXIT_FAILURE);
        }
        // clang-format on
        assert(pPasses_.back()->TYPE == type);
        assert(RenderPass::ALL.count(pPasses_.back()->TYPE));
    }

    // Update subpass offsets
    for (auto& pPass : pPasses_) pPass->setSubpassOffsets(pPasses_);

    // Initialize offset structures
    for (const auto& type : DEFAULT) {
        bool found = false;
        for (const auto& pPass : pPasses_) {
            if (pPass->TYPE == type) {
                // Add the passes from DEFAULT to the main loop offsets
                mainLoopOffsets_.push_back(pPass->OFFSET);

                // Add the passes/subpasses from DEFAULT to the active pass/offset pairs
                activeTypeOffsetPairs_.insert({pPass->TYPE, pPass->OFFSET});
                for (const auto& subpassTypeOffsetPair : pPass->getDependentTypeOffsetPairs())
                    activeTypeOffsetPairs_.insert(subpassTypeOffsetPair);

                found = true;
                break;
            }
        }
        assert(found);
    }

    assert(activeTypeOffsetPairs_.size() <= RESOURCE_SIZE);
}

void RenderPass::Handler::init() {
    // LIFECYCLE
    clearTargetMap_.clear();
    // Initialize the passes first to create the clear target offset map.
    for (const auto& offset : mainLoopOffsets_) pPasses_[offset]->init();
    // Finish creation with the clear map created.
    const auto activePassTypeOffsetPairs = getActivePassTypeOffsetPairs();  // Should I just set this??
    for (const auto& [passType, offset] : activePassTypeOffsetPairs.getList()) {
        pPasses_[offset]->create();
        pPasses_[offset]->postCreate();
        if (!pPasses_[offset]->usesSwapchain()) pPasses_[offset]->createTarget();
    }

    // SYNC
    createCmds();
    createFences();
    // TODO: should this just be an array too???? Ugh
    submitInfos_.assign(RESOURCE_SIZE, {});

    // SCREEN QUAD
    Mesh::Plane::CreateInfo planeInfo = {};
    // Indicate to the Mesh construction validation that this is a special
    // mesh because I don't want to do a bunch of work atm.
    {
        planeInfo.pipelineType = GRAPHICS::ALL_ENUM;
        planeInfo.passTypes.clear();
    }
    planeInfo.selectable = false;
    planeInfo.settings.geometryInfo.transform = helpers::affine(glm::vec3{2.0f});

    Instance::Obj3d::CreateInfo instObj3dInfo = {};
    Material::Default::CreateInfo defMatInfo = {};
    defMatInfo.flags = 0;

    // Create the default screen quad.
    screenQuadOffset_ =
        meshHandler().makeTextureMesh<Mesh::Plane::Texture>(&planeInfo, &defMatInfo, &instObj3dInfo)->getOffset();
    assert(screenQuadOffset_ != Mesh::BAD_OFFSET);
}

void RenderPass::Handler::frame() {
    // At one point something was here
}

Uniform::offsetsMap RenderPass::Handler::makeUniformOffsetsMap() {
    // TODO: validation? This is actually merging things.
    Uniform::offsetsMap map;
    for (const auto& pPass : pPasses_) {
        for (const auto& keyValue : pPass->getDescriptorPipelineOffsets()) {
            auto searchKey = map.find(keyValue.first);
            if (searchKey != map.end()) {
                auto searchValue = searchKey->second.find(keyValue.second);
                if (searchValue != searchKey->second.end()) {
                    searchValue->second.insert(pPass->TYPE);
                } else {
                    searchKey->second[keyValue.second] = {pPass->TYPE};
                }
            } else {
                map[keyValue.first] = {{keyValue.second, {pPass->TYPE}}};
            }
        }
    }
    return map;
}

void RenderPass::Handler::getActivePassTypes(std::set<PASS>& types, const PIPELINE& pipelineTypeIn) {
    // This is obviously to slow if ever used for anything meaningful.
    for (const auto& [passType, offset] : activeTypeOffsetPairs_) {
        if (passType == PASS::IMGUI) continue;
        if (pipelineTypeIn == PIPELINE{GRAPHICS::ALL_ENUM}) {
            types.insert(pPasses_[offset]->TYPE);
        } else {
            for (const auto& pipelineType : pPasses_[offset]->getPipelineTypes())
                if (pipelineType == pipelineTypeIn) types.insert(pPasses_[offset]->TYPE);
        }
    }
}

void RenderPass::Handler::addPipelinePassPairs(pipelinePassSet& set) {
    /* Create pipeline to pass offset map. This way pipeline cache stuff
     *  can potentially be used to speed up creation or caching???? I am
     *  not going to worry about it other than this atm though.
     */
    for (const auto& [passType, offset] : activeTypeOffsetPairs_) {
        for (const auto [pipelineType, bindDataOffset] : pPasses_[offset]->getPipelineBindDataList().getKeyOffsetMap()) {
            set.insert({pipelineType, pPasses_[offset]->TYPE});
        }
    }
}

void RenderPass::Handler::updateBindData(const pipelinePassSet& set) {
    const auto& bindDataMap = pipelineHandler().getPipelineBindDataMap();
    // Update pipeline bind data for all render passes.
    for (const auto& pair : set) {
        if (RenderPass::ALL.count(pair.second))  //
            getPass(pair.second)->setBindData(pair.first, bindDataMap.at(pair));
    }
}

void RenderPass::Handler::createCmds() {
    cmdList_.resize(10);
    for (auto& cmds : cmdList_) {
        cmds.resize(1);
        commandHandler().createCmdBuffers(QUEUE::GRAPHICS, cmds.data(), vk::CommandBufferLevel::ePrimary, cmds.size());
    }
}

void RenderPass::Handler::createFences(vk::FenceCreateFlags flags) {
    frameFences_.resize(shell().context().imageCount);
    vk::FenceCreateInfo fenceInfo = {flags};
    for (auto& fence : frameFences_) fence = shell().context().dev.createFence(fenceInfo, shell().context().pAllocator);
}

const std::unique_ptr<Mesh::Texture>& RenderPass::Handler::getScreenQuad() {
    return meshHandler().getTextureMesh(screenQuadOffset_);
}

void RenderPass::Handler::attachSwapchain() {
    // SWAPCHAIN
    createSwapchainResources();

    // LIFECYCLE
    for (const auto& [passType, offset] : activeTypeOffsetPairs_) {
        if (pPasses_[offset]->usesSwapchain()) pPasses_[offset]->createTarget();
    }
}

void RenderPass::Handler::detachSwapchain() {
    reset();
    destroySwapchainResources();
}

RenderPass::orderedPassTypeOffsetPairs RenderPass::Handler::getActivePassTypeOffsetPairs() {
    orderedPassTypeOffsetPairs pairs;
    for (const auto& offset : mainLoopOffsets_)
        for (const auto& pair : pPasses_[offset]->getDependentTypeOffsetPairs()) pairs.insert(pair);
    return pairs;
}

void RenderPass::Handler::createSwapchainResources() {
    const auto& ctx = shell().context();

    // Get the image resources from the swapchain...
    swpchnRes_.images = ctx.dev.getSwapchainImagesKHR(ctx.swapchain);

    bool wasTexDestroyed = false;
    swpchnRes_.views.resize(swpchnRes_.images.size());
    for (uint32_t i = 0; i < ctx.imageCount; i++) {
        vk::ImageSubresourceRange range = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

        helpers::createImageView(ctx.dev, swpchnRes_.images[i], ctx.surfaceFormat.format, vk::ImageViewType::e2D, range,
                                 swpchnRes_.views[i], shell().context().pAllocator);

        // TEXTURE
        auto pTexture = textureHandler().getTexture(SWAPCHAIN_TARGET_ID, i);
        assert(pTexture && pTexture->status != STATUS::READY);
        // Check if the swapchain was destroy before being recreated.
        wasTexDestroyed = pTexture->status == STATUS::DESTROYED;
        // Update swapchain texture descriptor info.
        auto& sampler = pTexture->samplers[0];
        sampler.image = swpchnRes_.images[i];
        auto& layerResource = sampler.layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL);
        layerResource.view = swpchnRes_.views[i];
        Texture::Handler::createDescInfo(pTexture, Sampler::IMAGE_ARRAY_LAYERS_ALL, layerResource, sampler);

        pTexture->status = STATUS::READY;
    }
    if (wasTexDestroyed) descriptorHandler().updateBindData({(std::string(SWAPCHAIN_TARGET_ID))});

    assert(shell().context().imageCount == swpchnRes_.images.size());
    assert(swpchnRes_.images.size() == swpchnRes_.views.size());
}

void RenderPass::Handler::acquireBackBuffer() {
    const auto& ctx = shell().context();
    fences_.clear();

    // TODO: Use the semaphores. This just waits for everything.
    loadingHandler().getFences(fences_);
    fences_.push_back(frameFences_[frameIndex_]);

    // wait for the last submission since we reuse frame data.
    ctx.dev.waitForFences(fences_, VK_TRUE, UINT64_MAX);
    ctx.dev.resetFences(1, &frameFences_[frameIndex_]);
}

void RenderPass::Handler::recordPasses() {
    const auto& ctx = shell().context();
    auto frameIndex = getFrameIndex();

    // auto& cmd = cmdList_[frameIndex][0];
    // cmd.reset({});
    // vk::CommandBufferBeginInfo bufferInfo = {};
    //// bufferInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    // bufferInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;
    // cmd.begin(bufferInfo);

    uint8_t passIndex = 0, resIndex = 0;
    for (; passIndex < mainLoopOffsets_.size(); passIndex++, resIndex++) {
        const auto& offset = mainLoopOffsets_[passIndex];
        SubmitResource* pResource = &submitResources_[resIndex];
        pResource->resetCount();

        if (passIndex == 0) {
            // wait for back buffer...
            pResource->waitSemaphores[pResource->waitSemaphoreCount] = ctx.acquiredBackBuffer.acquireSemaphore;
            // back buffer flags...
            pResource->waitDstStageMasks[pResource->waitSemaphoreCount] = ctx.waitDstStageMask;
            pResource->waitSemaphoreCount++;
        } else if (submitResources_[resIndex - 1].signalSemaphoreCount) {
            // Take the signals from the previous pass
            std::memcpy(                                                                     //
                &pResource->waitSemaphores[pResource->waitSemaphoreCount],                   //
                submitResources_[resIndex - 1].signalSemaphores.data(),                      //
                sizeof(vk::Semaphore) * submitResources_[resIndex - 1].signalSemaphoreCount  //
            );
            std::memcpy(                                                                              //
                &pResource->waitDstStageMasks[pResource->waitSemaphoreCount],                         //
                submitResources_[resIndex - 1].signalSrcStageMasks.data(),                            //
                sizeof(vk::PipelineStageFlags) * submitResources_[resIndex - 1].signalSemaphoreCount  //
            );
            pResource->waitSemaphoreCount += submitResources_[resIndex - 1].signalSemaphoreCount;
        }

        // Record the pass and update the resources
        pPasses_[offset]->record(frameIndex);
        pPasses_[offset]->updateSubmitResource(*pResource, frameIndex);

        // Check if a compute pass is needed.
        // computeHandler().recordPasses(pPasses_[offset]->TYPE, *pResource);

        // Always add the render semaphore to the last pass
        if ((static_cast<size_t>(passIndex) + 1) == mainLoopOffsets_.size()) {
            // signal back buffer...
            pResource->signalSemaphores[pResource->signalSemaphoreCount] = ctx.acquiredBackBuffer.renderSemaphore;
            pResource->signalSemaphoreCount++;
        }
    }

    submit(resIndex);
}

void RenderPass::Handler::submit(const uint8_t submitCount) {
    const SubmitResource* pResource;
    vk::SubmitInfo* pInfo;
    for (uint8_t i = 0; i < submitCount; i++) {
        pResource = &submitResources_[i];
        pInfo = &submitInfos_[i];
        pInfo->waitSemaphoreCount = pResource->waitSemaphoreCount;
        pInfo->pWaitSemaphores = pResource->waitSemaphores.data();
        pInfo->pWaitDstStageMask = pResource->waitDstStageMasks.data();
        for (uint32_t j = 0; j < pResource->commandBufferCount; j++)  //
            pResource->commandBuffers[j].end();
        pInfo->commandBufferCount = pResource->commandBufferCount;
        pInfo->pCommandBuffers = pResource->commandBuffers.data();
        pInfo->signalSemaphoreCount = pResource->signalSemaphoreCount;
        pInfo->pSignalSemaphores = pResource->signalSemaphores.data();
    }

    helpers::checkVkResult(
        commandHandler().graphicsQueue().submit(submitCount, submitInfos_.data(), frameFences_[frameIndex_]));
}

void RenderPass::Handler::updateFrameIndex() {
    frameIndex_ = (frameIndex_ + 1) % static_cast<uint8_t>(shell().context().imageCount);
}

bool RenderPass::Handler::isClearTargetPass(const std::string& targetId, const PASS type) {
    if (clearTargetMap_.at(targetId) == type) return true;
    return false;
}

bool RenderPass::Handler::isFinalTargetPass(const std::string& targetId, const PASS type) {
    if (finalTargetMap_.at(targetId) == type) return true;
    return false;
}

void RenderPass::Handler::destroySwapchainResources() {
    for (uint32_t i = 0; i < swpchnRes_.views.size(); i++) {
        shell().context().dev.destroyImageView(swpchnRes_.views[i], shell().context().pAllocator);
        // Update swapchain texture status.
        auto pTexture = textureHandler().getTexture(SWAPCHAIN_TARGET_ID, i);
        assert(pTexture);
        pTexture->status = STATUS::DESTROYED;
    }
    swpchnRes_.views.clear();
    // Images are destroyed by destroySwapchainKHR
    swpchnRes_.images.clear();
}

void RenderPass::Handler::destroy() {
    // PASS
    for (auto& pPass : pPasses_) {
        if (!pPass->usesSwapchain() || std::find(activeTypeOffsetPairs_.begin(), activeTypeOffsetPairs_.end(),
                                                 std::pair{pPass->TYPE, pPass->OFFSET}) == activeTypeOffsetPairs_.end()) {
            pPass->destroyTargetResources();
        }
        pPass->destroy();
    }
    // COMMAND
    for (auto& cmds : cmdList_) {
        helpers::destroyCommandBuffers(shell().context().dev, commandHandler().graphicsCmdPool(), cmds);
        cmds.clear();
    }
    cmdList_.clear();
    // FENCE
    for (auto& fence : frameFences_) shell().context().dev.destroyFence(fence, shell().context().pAllocator);
    frameFences_.clear();
}

void RenderPass::Handler::reset() {
    for (auto& pPass : pPasses_) {
        if (pPass->usesSwapchain() && std::find(activeTypeOffsetPairs_.begin(), activeTypeOffsetPairs_.end(),
                                                std::pair{pPass->TYPE, pPass->OFFSET}) != activeTypeOffsetPairs_.end()) {
            pPass->destroyTargetResources();
        }
    }
}
