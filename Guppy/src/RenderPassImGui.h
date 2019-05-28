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
    ImGui(Handler& handler);

    void init() override;
    void postCreate() override;
    void setSwapchainInfo() override;
    void record() override;

    void getSubmitResource(const uint8_t& frameIndex, SubmitResource& resource) const override;

   private:
    void createClearValues() override;
    void createAttachmentsAndSubpasses() override;
    void createDependencies() override;
    void createFramebuffers() override;
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_IMGUI_H
