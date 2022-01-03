/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "RenderPassManager.h"

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
#if USE_VOLUMETRIC_LIGHTING
// ...
#endif
// HANDLERS
#include "CommandHandler.h"
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "MeshHandler.h"
#include "PassHandler.h"
#include "PipelineHandler.h"
#include "TextureHandler.h"

namespace {
// clang-format off
const std::vector<RENDER_PASS> DEFAULT = {
#if VK_USE_PLATFORM_MACOS_MVK || false
#if DO_PROJECTOR
    RENDER_PASS::SAMPLER_PROJECT,
#endif
    RENDER_PASS::DEFAULT,
    //RENDER_PASS::SAMPLER_DEFAULT,
    //RENDER_PASS::SCREEN_SPACE,
#else
    RENDER_PASS::DEFERRED,
#endif
// UI pass needs to always be last since it
// is optional
#ifdef USE_DEBUG_UI
    RENDER_PASS::IMGUI,
#endif
};
// clang-format on
}  // namespace

namespace RenderPass {

Manager::Manager(Pass::Handler& handler)
    : Pass::Manager(handler),  //
      frameIndex_(0),
      swpchnRes_{},
      submitResources_{},
      screenQuadOffset_(Mesh::BAD_OFFSET) {
    for (const auto& type : ALL) {
        // clang-format off
        switch (type) {
            case RENDER_PASS::DEFAULT:                  pPasses_.emplace_back(std::make_unique<Base>                       (handler, static_cast<index>(pPasses_.size()), &DEFAULT_CREATE_INFO)); break;
            case RENDER_PASS::SAMPLER_PROJECT:          pPasses_.emplace_back(std::make_unique<Base>                       (handler, static_cast<index>(pPasses_.size()), &PROJECT_CREATE_INFO)); break;
            case RENDER_PASS::SAMPLER_DEFAULT:          pPasses_.emplace_back(std::make_unique<Base>                       (handler, static_cast<index>(pPasses_.size()), &SAMPLER_DEFAULT_CREATE_INFO)); break;
            case RENDER_PASS::SCREEN_SPACE:             pPasses_.emplace_back(std::make_unique<ScreenSpace::Base>          (handler, static_cast<index>(pPasses_.size()), &ScreenSpace::CREATE_INFO)); break;
            case RENDER_PASS::SCREEN_SPACE_HDR_LOG:     pPasses_.emplace_back(std::make_unique<ScreenSpace::HdrLog>        (handler, static_cast<index>(pPasses_.size()))); break;
            case RENDER_PASS::SCREEN_SPACE_BRIGHT:      pPasses_.emplace_back(std::make_unique<ScreenSpace::Bright>        (handler, static_cast<index>(pPasses_.size()))); break;
            case RENDER_PASS::SCREEN_SPACE_BLUR_A:      pPasses_.emplace_back(std::make_unique<ScreenSpace::BlurA>         (handler, static_cast<index>(pPasses_.size()))); break;
            case RENDER_PASS::SCREEN_SPACE_BLUR_B:      pPasses_.emplace_back(std::make_unique<ScreenSpace::BlurB>         (handler, static_cast<index>(pPasses_.size()))); break;
            case RENDER_PASS::DEFERRED:                 pPasses_.emplace_back(std::make_unique<Deferred::Base>             (handler, static_cast<index>(pPasses_.size()))); break;
            case RENDER_PASS::SHADOW_DEFAULT:           pPasses_.emplace_back(std::make_unique<Shadow::Default>            (handler, static_cast<index>(pPasses_.size()))); break;
            case RENDER_PASS::SHADOW_CUBE:              pPasses_.emplace_back(std::make_unique<Shadow::Cube>               (handler, static_cast<index>(pPasses_.size()))); break;
            case RENDER_PASS::SKYBOX_NIGHT:             pPasses_.emplace_back(std::make_unique<CubeMap::SkyboxNight>       (handler, static_cast<index>(pPasses_.size()))); break;
#ifdef USE_VOLUMETRIC_LIGHTING
            // ...
#endif
            case RENDER_PASS::IMGUI:
#ifdef USE_DEBUG_UI
                                                    pPasses_.emplace_back(std::make_unique<ImGui>               (handler, static_cast<index>(pPasses_.size())));
#endif
                break;
            default: assert(false && "Unhandled RENDER_PASS type"); exit(EXIT_FAILURE);
        }
        // clang-format on
        assert(pPasses_.back()->TYPE == type);
        assert(ALL.count(pPasses_.back()->TYPE));
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

void Manager::init() {
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
        handler().meshHandler().makeTextureMesh<Mesh::Plane::Texture>(&planeInfo, &defMatInfo, &instObj3dInfo)->getOffset();
    assert(screenQuadOffset_ != Mesh::BAD_OFFSET);
}

void Manager::frame() {
    const auto& ctx = handler().shell().context();
    auto frameIndex = getFrameIndex();

    uint8_t passIndex = 0, resIndex = 0;
    for (; passIndex < mainLoopOffsets_.size(); passIndex++, resIndex++) {
        const auto& offset = mainLoopOffsets_[passIndex];
        SubmitResource* pResource = &submitResources_[resIndex];
        pResource->resetCount();

        if (passIndex == 0) {
            // Wait for back buffer
            pResource->waitSemaphores[pResource->waitSemaphoreCount] = ctx.acquiredBackBuffer.acquireSemaphore;
            pResource->waitDstStageMasks[pResource->waitSemaphoreCount] = ctx.waitDstStageMask;
            pResource->waitSemaphoreCount++;
        }

        // Record the pass and update the resources
        pPasses_[offset]->record(frameIndex);
        pPasses_[offset]->updateSubmitResource(*pResource, frameIndex);

        // Always add the render semaphore to the last pass
        if ((static_cast<size_t>(passIndex) + 1) == mainLoopOffsets_.size()) {
            // Signal back buffer
            pResource->signalSemaphores[pResource->signalSemaphoreCount] = ctx.acquiredBackBuffer.renderSemaphore;
            pResource->signalSemaphoreCount++;
        }
    }

    submit(resIndex);
}

Uniform::offsetsMap Manager::makeUniformOffsetsMap() {
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

void Manager::getActivePassTypes(const PIPELINE& pipelineTypeIn, std::set<PASS>& types) {
    /* TODO: I don't think this works as it should atm. I believe passing in GRAPHICS::ALL_ENUM is a shortcut for rendering
     * shadow maps, but I am pretty sure this code would add all the compute only passes to the set that is returned. This
     * might or might not be wanted/necessary. I would have to audit all the passes to find out.
     */

    // This is obviously too slow if ever used for anything meaningful.
    for (const auto& [passType, offset] : activeTypeOffsetPairs_) {
        if (passType == RENDER_PASS::IMGUI) continue;
        if (pipelineTypeIn == PIPELINE{COMPUTE::ALL_ENUM}) {
            // This could be made to work, but it doesn't currently...
            assert(false && "Pipeline type COMPUTE::ALL_ENUM not handled");
        } else if (pipelineTypeIn == PIPELINE{GRAPHICS::ALL_ENUM}) {
            types.insert(pPasses_[offset]->TYPE);
        } else {
            for (const auto& pipelineType : pPasses_[offset]->getPipelineTypes())
                if (pipelineType == pipelineTypeIn) types.insert(pPasses_[offset]->TYPE);
        }
    }
}

void Manager::addPipelinePassPairs(pipelinePassSet& set) {
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

void Manager::createCmds() {
    cmdList_.resize(10);
    for (auto& cmds : cmdList_) {
        cmds.resize(1);
        handler().commandHandler().createCmdBuffers(QUEUE::GRAPHICS, cmds.data(), vk::CommandBufferLevel::ePrimary,
                                                    cmds.size());
    }
}

void Manager::createFences(vk::FenceCreateFlags flags) {
    const auto& ctx = handler().shell().context();
    frameFences_.resize(ctx.imageCount);
    vk::FenceCreateInfo fenceInfo = {flags};
    for (auto& fence : frameFences_) fence = ctx.dev.createFence(fenceInfo, ctx.pAllocator);
}

const std::unique_ptr<Mesh::Texture>& Manager::getScreenQuad() {
    return handler().meshHandler().getTextureMesh(screenQuadOffset_);
}

void Manager::attachSwapchain() {
    // SWAPCHAIN
    createSwapchainResources();

    // LIFECYCLE
    for (const auto& [passType, offset] : activeTypeOffsetPairs_) {
        if (pPasses_[offset]->usesSwapchain()) pPasses_[offset]->createTarget();
    }
}

void Manager::detachSwapchain() {
    reset();
    destroySwapchainResources();
}

orderedPassTypeOffsetPairs Manager::getActivePassTypeOffsetPairs() {
    orderedPassTypeOffsetPairs pairs;
    for (const auto& offset : mainLoopOffsets_)
        for (const auto& pair : pPasses_[offset]->getDependentTypeOffsetPairs()) pairs.insert(pair);
    return pairs;
}

void Manager::createSwapchainResources() {
    const auto& ctx = handler().shell().context();

    // Get the image resources from the swapchain...
    swpchnRes_.images = ctx.dev.getSwapchainImagesKHR(ctx.swapchain);

    bool wasTexDestroyed = false;
    swpchnRes_.views.resize(swpchnRes_.images.size());
    for (uint32_t i = 0; i < ctx.imageCount; i++) {
        vk::ImageSubresourceRange range = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};

        helpers::createImageView(ctx.dev, swpchnRes_.images[i], ctx.surfaceFormat.format, vk::ImageViewType::e2D, range,
                                 swpchnRes_.views[i], ctx.pAllocator);

        // TEXTURE
        auto pTexture = handler().textureHandler().getTexture(SWAPCHAIN_TARGET_ID, i);
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
    if (wasTexDestroyed) handler().descriptorHandler().updateBindData({(std::string(SWAPCHAIN_TARGET_ID))});

    assert(ctx.imageCount == swpchnRes_.images.size());
    assert(swpchnRes_.images.size() == swpchnRes_.views.size());
}

void Manager::acquireBackBuffer() {
    const auto& ctx = handler().shell().context();
    fences_.clear();

    // TODO: Use the semaphores. This just waits for everything.
    handler().loadingHandler().getFences(fences_);
    fences_.push_back(frameFences_[frameIndex_]);

    // wait for the last submission since we reuse frame data.
    auto result = ctx.dev.waitForFences(fences_, VK_TRUE, UINT64_MAX);
    assert(result == vk::Result::eSuccess);
    result = ctx.dev.resetFences(1, &frameFences_[frameIndex_]);
    assert(result == vk::Result::eSuccess);
}

void Manager::submit(const uint8_t submitCount) {
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

    auto result =
        handler().commandHandler().graphicsQueue().submit(submitCount, submitInfos_.data(), frameFences_[frameIndex_]);
    assert(result == vk::Result::eSuccess);
}

void Manager::updateFrameIndex() {
    frameIndex_ = (frameIndex_ + 1) % static_cast<uint8_t>(handler().shell().context().imageCount);
}

bool Manager::isClearTargetPass(const std::string& targetId, const RENDER_PASS type) {
    if (clearTargetMap_.at(targetId) == type) return true;
    return false;
}

bool Manager::isFinalTargetPass(const std::string& targetId, const RENDER_PASS type) {
    if (finalTargetMap_.at(targetId) == type) return true;
    return false;
}

void Manager::destroySwapchainResources() {
    const auto& ctx = handler().shell().context();
    for (uint32_t i = 0; i < swpchnRes_.views.size(); i++) {
        ctx.dev.destroyImageView(swpchnRes_.views[i], ctx.pAllocator);
        // Update swapchain texture status.
        auto pTexture = handler().textureHandler().getTexture(SWAPCHAIN_TARGET_ID, i);
        assert(pTexture);
        pTexture->status = STATUS::DESTROYED;
    }
    swpchnRes_.views.clear();
    // Images are destroyed by destroySwapchainKHR
    swpchnRes_.images.clear();
}

void Manager::destroy() {
    const auto& ctx = handler().shell().context();
    // RENDER PASS
    for (auto& pPass : pPasses_) {
        if (!pPass->usesSwapchain() || std::find(activeTypeOffsetPairs_.begin(), activeTypeOffsetPairs_.end(),
                                                 std::pair{pPass->TYPE, pPass->OFFSET}) == activeTypeOffsetPairs_.end()) {
            pPass->destroyTargetResources();
        }
        pPass->destroy();
    }
    pPasses_.clear();
    // COMMAND
    for (auto& cmds : cmdList_) {
        helpers::destroyCommandBuffers(ctx.dev, handler().commandHandler().graphicsCmdPool(), cmds);
        cmds.clear();
    }
    cmdList_.clear();
    // FENCE
    for (auto& fence : frameFences_) ctx.dev.destroyFence(fence, ctx.pAllocator);
    frameFences_.clear();
}

void Manager::reset() {
    for (auto& pPass : pPasses_) {
        if (pPass->usesSwapchain() && std::find(activeTypeOffsetPairs_.begin(), activeTypeOffsetPairs_.end(),
                                                std::pair{pPass->TYPE, pPass->OFFSET}) != activeTypeOffsetPairs_.end()) {
            pPass->destroyTargetResources();
        }
    }
}

}  // namespace RenderPass