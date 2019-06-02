#ifndef RENDER_PASS_HANDLER_H
#define RENDER_PASS_HANDLER_H

#include <array>
#include <memory>
#include <utility>
#include <vector>

#include "Constants.h"
#include "Game.h"
#include "RenderPass.h"

namespace RenderPass {

using index = uint32_t;
constexpr index BAD_OFFSET = UINT32_MAX;

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame);

    void init() override;
    void destroy() override;

    void acquireBackBuffer();
    void recordPasses();
    void update();

    inline const std::unique_ptr<RenderPass::Base>& getPass(const offset& offset) {
        assert(offset >= 0 && offset < pPasses_.size());
        return pPasses_[offset];
    }
    inline uint8_t getFrameIndex() const { return frameIndex_; }

    void attachSwapchain();
    void detachSwapchain();

    inline bool getSwapchainCleared() {
        bool cleared = swpchnRes_.cleared;
        if (!cleared) swpchnRes_.cleared = true;
        return cleared;
    }
    inline auto getSwapchainImageCount() const { return swpchnRes_.images.size(); }
    inline const VkImage* getSwapchainImages() const { return swpchnRes_.images.data(); }
    inline const VkImageView* getSwapchainViews() const { return swpchnRes_.views.data(); }

    // PIPELINE
    void updatePipelineReferences(const std::multiset<std::pair<PIPELINE, RenderPass::offset>>& set,
                                  const std::vector<Pipeline::Reference>& refs);

   private:
    void reset() override;

    uint8_t frameIndex_;

    // PIPELINE
    void createPipelines();

    // FENCES
    void createFences(VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT);
    std::vector<VkFence> fences_;

    // SWAPCHAIN
    void createSwapchainResources();
    void destroySwapchainResources();
    SwapchainResources swpchnRes_;

    // SUBMIT
    void submit();
    std::array<SubmitResource, RESOURCE_SIZE> submitResources_;
    std::vector<VkSubmitInfo> submitInfos_;

    std::vector<std::unique_ptr<RenderPass::Base>> pPasses_;
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_HANDLER_H
