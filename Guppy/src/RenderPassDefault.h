#ifndef RENDER_PASS_DEFAULT_H
#define RENDER_PASS_DEFAULT_H

#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Helpers.h"
#include "RenderPass.h"

namespace RenderPass {

class Handler;

class Default : public Base {
   public:
    Default(Handler &handler, const uint32_t &&offset, const RenderPass::CreateInfo *pCreateInfo);

    void init() override;
    void setSwapchainInfo() override;

    inline void beginSecondary(const uint8_t &frameIndex) override {
        if (secCmdFlag_) return;
        // FRAME UPDATE
        auto &secCmd = data.secCmds[frameIndex];
        inheritInfo_.framebuffer = data.framebuffers[frameIndex];
        // COMMAND
        vkResetCommandBuffer(secCmd, 0);
        vk::assert_success(vkBeginCommandBuffer(secCmd, &secCmdBeginInfo_));
        // FRAME COMMANDS
        vkCmdSetScissor(secCmd, 0, 1, &scissor_);
        vkCmdSetViewport(secCmd, 0, 1, &viewport_);

        secCmdFlag_ = true;
    }

    inline void endSecondary(const uint8_t &frameIndex, const VkCommandBuffer &priCmd) override {
        if (!secCmdFlag_) return;
        // EXECUTE SECONDARY
        auto &secCmd = data.secCmds[frameIndex];
        vk::assert_success(vkEndCommandBuffer(secCmd));
        vkCmdExecuteCommands(priCmd, 1, &secCmd);

        secCmdFlag_ = false;
    }

   protected:
    bool includeMultiSampleAttachment_;

   private:
    void createBeginInfo() override;
    VkCommandBufferInheritanceInfo inheritInfo_;
    VkCommandBufferBeginInfo secCmdBeginInfo_;
    bool secCmdFlag_;

    void createCommandBuffers() override;
    void createColorResources() override;
    void createDepthResource() override;
    void createAttachmentsAndSubpasses() override;
    void createDependencies() override;
    void updateClearValues() override;
    void createFramebuffers() override;
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_DEFAULT_H
