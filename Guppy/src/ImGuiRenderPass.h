#ifndef IMGUI_RENDER_PASS_H
#define IMGUI_RENDER_PASS_H

#include <vulkan/vulkan.h>

#include "RenderPass.h"
#include "Shell.h"

class ImGuiRenderPass : public RenderPass::Base {
   public:
    ImGuiRenderPass() : RenderPass::Base("ImGui", {}) {}

   private:
    void createSubpassesAndAttachments(const Shell::Context& ctx, const Game::Settings& settings) override{};
    void createDependencies(const Shell::Context& ctx, const Game::Settings& settings) override;
    void getSubpassDescriptions(std::vector<VkSubpassDescription>& subpasses) const;
    // void createImages(const Shell::Context& ctx) override;
    void createImageViews(const Shell::Context& ctx) override;
    // void createFramebuffers(const Shell::Context& ctx) override;
};

#endif  // !IMGUI_RENDER_PASS_H
