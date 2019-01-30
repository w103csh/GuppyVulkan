#ifndef IMGUI_RENDER_PASS_H
#define IMGUI_RENDER_PASS_H

#include <vulkan/vulkan.h>

#include "RenderPass.h"
#include "Shell.h"

class ImGuiRenderPass : public RenderPass::Base {
   public:
    ImGuiRenderPass() : RenderPass::Base{"ImGui", {}} {}

    void getSubmitResource(const uint8_t& frameIndex, SubmitResource& resource) const override;

   private:
    void createClearValues(const Shell::Context& ctx, const Game::Settings& settings) override;
    void createAttachmentsAndSubpasses(const Shell::Context& ctx, const Game::Settings& settings) override;
    void createDependencies(const Shell::Context& ctx, const Game::Settings& settings) override;
    void createFramebuffers(const Shell::Context& ctx, const Game::Settings& settings) override;
};

#endif  // !IMGUI_RENDER_PASS_H
