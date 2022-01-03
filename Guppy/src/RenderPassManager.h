/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef RENDER_PASS_HANDLER_H
#define RENDER_PASS_HANDLER_H

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "Compute.h"
#include "ConstantsAll.h"
#include "Mesh.h"
#include "PassHandler.h"
#include "RenderPass.h"

namespace RenderPass {

class Manager : public Pass::Manager {
    friend Base;

   public:
    Manager(Pass::Handler& handler);

    Uniform::offsetsMap makeUniformOffsetsMap();

    constexpr const auto& getPasses() { return pPasses_; }
    // NOTE: this is not in order!!!
    void getActivePassTypes(const PIPELINE& pipelineTypeIn, std::set<PASS>& types);

    void init() override;
    void frame() override;
    void destroy() override;
    void reset() override;

    void acquireBackBuffer();
    void updateFrameIndex();

    inline auto& getPass(const index& offset) { return pPasses_.at(offset); }
    inline auto& getPass(const RENDER_PASS& type) {
        for (const auto& pPass : pPasses_)
            if (pPass->TYPE == type) return pPass;
        assert(false);
        exit(EXIT_FAILURE);
    }
    constexpr auto getFrameIndex() const { return frameIndex_; }
    inline const auto& getFrameFence() const { return frameFences_[frameIndex_]; }

    void attachSwapchain();
    void detachSwapchain();

    inline const auto& getCurrentFramebufferImage() const { return swpchnRes_.images[frameIndex_]; }
    inline const auto* getSwapchainImages() const { return swpchnRes_.images.data(); }
    inline const auto* getSwapchainViews() const { return swpchnRes_.views.data(); }

    // TARGET
    bool isClearTargetPass(const std::string& targetId, const RENDER_PASS type);
    bool isFinalTargetPass(const std::string& targetId, const RENDER_PASS type);

    // PIPELINE
    void addPipelinePassPairs(pipelinePassSet& set);

    inline const auto& getFrameFence(const uint8_t frameIndex) const { return frameFences_[frameIndex]; }

    const std::unique_ptr<Mesh::Texture>& getScreenQuad();

   private:
    uint8_t frameIndex_;

    // COMMANDS
    void createCmds();
    std::vector<std::vector<vk::CommandBuffer>> cmdList_;

    // FENCES
    void createFences(vk::FenceCreateFlags flags = vk::FenceCreateFlagBits::eSignaled);
    std::vector<vk::Fence> fences_;
    std::vector<vk::Fence> frameFences_;

    // This should be a unique list of active passes in order of first use.
    orderedPassTypeOffsetPairs getActivePassTypeOffsetPairs();

    // SWAPCHAIN
    void createSwapchainResources();
    void destroySwapchainResources();
    SwapchainResources swpchnRes_;

    // CLEAR
    std::map<std::string, RENDER_PASS> clearTargetMap_;
    std::map<std::string, RENDER_PASS> finalTargetMap_;

    // BARRIER
    BarrierResource barrierResource_;

    // SUBMIT
    void submit(const uint8_t submitCount);
    SubmitResources submitResources_;
    std::vector<vk::SubmitInfo> submitInfos_;

    std::vector<std::unique_ptr<Base>> pPasses_;
    std::set<std::pair<RENDER_PASS, index>> activeTypeOffsetPairs_;
    std::vector<index> mainLoopOffsets_;

    Mesh::index screenQuadOffset_;
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_HANDLER_H
