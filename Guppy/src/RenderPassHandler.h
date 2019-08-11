#ifndef RENDER_PASS_HANDLER_H
#define RENDER_PASS_HANDLER_H

#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "ConstantsAll.h"
#include "Game.h"
#include "Mesh.h"
#include "RenderPass.h"

namespace RenderPass {

class Handler : public Game::Handler {
    friend RenderPass::Base;

   public:
    Handler(Game* pGame);

    Uniform::offsetsMap makeUniformOffsetsMap();

    constexpr const auto& getPasses() { return pPasses_; }
    // NOTE: this is not in order!!!
    void getActivePassTypes(std::set<PASS>& types, const PIPELINE& pipelineTypeIn = PIPELINE::ALL_ENUM);

    void init() override;
    void destroy() override;

    void acquireBackBuffer();
    void recordPasses();
    void update();
    void updateFrameIndex();

    inline const auto& getPass(const index& offset) { return pPasses_.at(offset); }
    inline const auto& getPass(const PASS& type) {
        for (const auto& pPass : pPasses_)
            if (pPass->TYPE == type) return pPass;
        assert(false);
        exit(EXIT_FAILURE);
    }
    inline uint8_t getFrameIndex() const { return frameIndex_; }

    void attachSwapchain();
    void detachSwapchain();

    inline const auto& getCurrentFramebufferImage() const { return swpchnRes_.images[frameIndex_]; }
    inline const auto* getSwapchainImages() const { return swpchnRes_.images.data(); }
    inline const auto* getSwapchainViews() const { return swpchnRes_.views.data(); }

    // TARGET
    bool isClearTargetPass(const std::string& targetId, const PASS type);
    bool isFinalTargetPass(const std::string& targetId, const PASS type);

    // PIPELINE
    void addPipelinePassPairs(pipelinePassSet& set);
    void updateBindData(const pipelinePassSet& set);

    inline const auto& getFrameFence(const uint8_t frameIndex) const { return frameFences_[frameIndex]; }

    const std::unique_ptr<Mesh::Texture>& getScreenQuad();

   private:
    void reset() override;

    uint8_t frameIndex_;

    // COMMANDS
    void createCmds();
    std::vector<std::vector<VkCommandBuffer>> cmdList_;

    // FENCES
    void createFences(VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT);
    std::vector<VkFence> fences_;
    std::vector<VkFence> frameFences_;

    // This should be a unique list of active passes in order of first use.
    orderedPassTypeOffsetPairs getActivePassTypeOffsetPairs();

    // SWAPCHAIN
    void createSwapchainResources();
    void destroySwapchainResources();
    SwapchainResources swpchnRes_;

    // CLEAR
    std::map<std::string, PASS> clearTargetMap_;
    std::map<std::string, PASS> finalTargetMap_;

    // BARRIER
    BarrierResource barrierResource_;

    // SUBMIT
    void submit(const uint8_t submitCount);
    SubmitResources submitResources_;
    std::vector<VkSubmitInfo> submitInfos_;

    std::vector<std::unique_ptr<RenderPass::Base>> pPasses_;
    std::set<std::pair<PASS, index>> activeTypeOffsetPairs_;
    std::vector<index> mainLoopOffsets_;

    Mesh::index screenQuadOffset_;
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_HANDLER_H
