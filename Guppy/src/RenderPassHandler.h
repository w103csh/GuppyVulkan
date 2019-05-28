#ifndef RENDER_PASS_HANDLER_H
#define RENDER_PASS_HANDLER_H

#include <memory>
#include <vector>

#include "Game.h"
#include "RenderPass.h"

namespace RenderPass {

typedef uint32_t INDEX;
constexpr INDEX BAD_OFFSET = UINT32_MAX;

struct SwapchainResources {
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
};

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame);

    void init() override;
    void destroy() override;

    void acquireBackBuffer();
    void recordPasses();
    void submit(const std::vector<SubmitResource>& resources);
    void update();

    inline const std::unique_ptr<RenderPass::Base>& getMainPass() const { return pPasses_[defaultOffset_]; }
    inline const std::unique_ptr<RenderPass::Base>& getUIPass() const { return pPasses_[uiOffset_]; }
    inline const uint8_t getFrameIndex() const { return frameIndex_; }

    void attachSwapchain();
    void detachSwapchain();
    inline const SwapchainResources& getSwapchainResources() const { return swpchnRes_; }

   private:
    void reset() override;

    uint8_t frameIndex_;

    // SWAPCHAIN
    void createSwapchainResources();
    void destroySwapchainResources();
    SwapchainResources swpchnRes_;

    INDEX defaultOffset_;
    INDEX uiOffset_;

    std::vector<std::unique_ptr<RenderPass::Base>> pPasses_;
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_HANDLER_H
