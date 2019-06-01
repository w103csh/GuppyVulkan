#ifndef RENDER_PASS_IMGUI_H
#define RENDER_PASS_IMGUI_H

#include <vulkan/vulkan.h>

#include "Constants.h"
#include "RenderPass.h"

struct SubmitResource;

namespace RenderPass {

class Handler;

class ImGui : public Base {
   public:
    ImGui(Handler& handler, const uint32_t&& offset);

    void init() override;
    void postCreate() override;
    void setSwapchainInfo() override;
    void record(const uint32_t& frameIndex) override;

   private:
    void createAttachmentsAndSubpasses() override;
    void createDependencies() override;
    void updateClearValues() override;
    void createFramebuffers() override;
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_IMGUI_H
