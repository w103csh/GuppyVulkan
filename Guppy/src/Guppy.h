
#ifndef GUPPY_H
#define GUPPY_H

#include <vector>

#include "Camera.h"
#include "Game.h"
#include "Helpers.h"
#include "RenderPass.h"

class Model;
struct MouseInput;
class Shell;

class Guppy : public Game {
   public:
    Guppy(const std::vector<std::string>& args);
    ~Guppy();

    // SHELL
    void attachShell(Shell& sh) override;
    void detachShell() override;

    // SWAPCHAIN
    void attachSwapchain() override;
    void detachSwapchain() override;

    // GAME
    void onTick() override;
    void onFrame(float framePred) override;

    // INPUT
    void onKey(GAME_KEY key) override;
    void onMouse(const MouseInput& input) override;

   private:
    // TODO: check Hologram for a good starting point for these...
    bool multithread_;
    bool use_push_constants_;
    bool sim_paused_;
    bool sim_fade_;
    // Simulation sim_;

    // SWAPCHAIN
    void createSwapchainResources(const Shell::Context& ctx);
    void destroySwapchainResources(const Shell::Context& ctx);
    struct SwapchainResources {
        std::vector<VkImage> images;
        std::vector<VkImageView> views;
    } swapchainResources_;

    // FRAME
    uint8_t frameIndex_;
    std::vector<VkFramebuffer> framebuffers_;

    // RENDER PASS
    void initRenderPasses();
    void submitRenderPasses(const std::vector<SubmitResource>& resources, VkFence fence = VK_NULL_HANDLE);
    std::unique_ptr<RenderPass::Base> pDefaultRenderPass_;
    std::unique_ptr<RenderPass::Base> pUIRenderPass_;

    // SCENE
    void createScenes();

    Camera camera_;
};

#endif  // !GUPPY_H