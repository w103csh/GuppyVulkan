#ifndef RENDER_PASS_HANDLER_H
#define RENDER_PASS_HANDLER_H

#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "ConstantsAll.h"
#include "DescriptorSet.h"
#include "Game.h"
#include "RenderPass.h"

namespace Descriptor {
class Handler;
namespace Set {
namespace RenderPass {
class SwapchainImage : public Descriptor::Set::Base {
   public:
    SwapchainImage(Handler& handler);
};
}  // namespace RenderPass
}  // namespace Set
}  // namespace Descriptor

namespace RenderPass {

using index = uint32_t;
constexpr index BAD_OFFSET = UINT32_MAX;

class Handler : public Game::Handler {
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

    inline const auto& getPass(const offset& offset) {
        assert(offset >= 0 && offset < pPasses_.size());
        return pPasses_[offset];
    }
    inline const auto& getPass(const PASS& type) {
        for (const auto& pPass : pPasses_)
            if (pPass->TYPE == type) return pPass;
        assert(false);
        throw std::runtime_error("Couldn't find pass");
    }
    inline uint8_t getFrameIndex() const { return frameIndex_; }

    void attachSwapchain();
    void detachSwapchain();

    inline const auto& getCurrentFramebufferImage() const { return swpchnRes_.images[frameIndex_]; }
    inline const auto* getSwapchainImages() const { return swpchnRes_.images.data(); }
    inline const auto* getSwapchainViews() const { return swpchnRes_.views.data(); }

    // TARGET
    bool shouldClearTarget(const std::string& targetId);
    std::set<std::string> targetClearFlags_;

    // PIPELINE
    void addPipelinePassPairs(pipelinePassSet& set);
    void updateBindData(const pipelinePassSet& set);

    inline const auto& getFrameFence(const uint8_t frameIndex) const { return frameFences_[frameIndex]; }

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

   private:
    // SWAPCHAIN
    void createSwapchainResources();
    void destroySwapchainResources();
    SwapchainResources swpchnRes_;

    // BARRIER
    BarrierResource barrierResource_;

    // SUBMIT
    void submit(const uint8_t submitCount);
    SubmitResources submitResources_;
    std::vector<VkSubmitInfo> submitInfos_;

    std::vector<std::unique_ptr<RenderPass::Base>> pPasses_;
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_HANDLER_H
