#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <string>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "Shell.h"

namespace Pipeline {
class Base;
}

namespace RenderPass {

struct InitInfo {
    bool clearColor = false;
    bool clearDepth = false;
    VkFormat format = {};
    VkFormat depthFormat = {};
    VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    uint32_t commandCount = 0;
    uint32_t fenceCount = 0;
    uint32_t semaphoreCount = 0;
    VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
};

struct FrameInfo {
    VkExtent2D extent = {0, 0};
    uint32_t viewCount;
    VkImageView *pViews = nullptr;
};

struct Data {
    std::vector<VkFence> fences;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> priCmds;
    std::vector<VkCommandBuffer> secCmds;
    std::vector<VkSemaphore> semaphores;
    std::vector<int> tests;
};

struct SubpassResources {
    // storage
    std::vector<VkAttachmentReference> inputAttachments;
    std::vector<VkAttachmentReference> colorAttachments;
    std::vector<VkAttachmentReference> resolveAttachments;
    VkAttachmentReference depthStencilAttachment = {};
    std::vector<uint32_t> preserveAttachments;
    // render pass create info
    std::vector<VkSubpassDescription> subpasses;
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkSubpassDependency> dependencies;
};

// **********************
//      Base
// **********************

class Base {
    friend class Pipeline::Base;

   public:
    const std::unordered_set<PIPELINE> PIPELINE_TYPES;
    const std::string NAME;

    void init(const Shell::Context &ctx, const Game::Settings &settings, RenderPass::InitInfo *pInfo,
              SubpassResources *pSubpassResources = nullptr);

    virtual void createTarget(const Shell::Context &ctx, const Game::Settings &settings, RenderPass::FrameInfo *pInfo);

    virtual inline void beginPass(
        const uint8_t &frameIndex,
        VkCommandBufferUsageFlags &&primaryCommandUsage = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        VkSubpassContents &&subpassContents = VK_SUBPASS_CONTENTS_INLINE) {
        if (data.tests[frameIndex]) data.tests[frameIndex] = 0;

        // FRAME UPDATE
        beginInfo_.framebuffer = data.framebuffers[frameIndex];
        auto &priCmd = data.priCmds[frameIndex];

        // RESET BUFFERS
        vkResetCommandBuffer(priCmd, 0);

        // BEGIN BUFFERS
        VkCommandBufferBeginInfo bufferInfo;
        // PRIMARY
        bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bufferInfo.flags = primaryCommandUsage;
        vk::assert_success(vkBeginCommandBuffer(priCmd, &bufferInfo));

        // FRAME COMMANDS
        vkCmdSetScissor(priCmd, 0, 1, &scissor_);
        vkCmdSetViewport(priCmd, 0, 1, &viewport_);

        //// Start a new debug marker region
        // ext::DebugMarkerBegin(primaryCmd, "Render x scene", glm::vec4(0.2f, 0.3f, 0.4f, 1.0f));
        vkCmdBeginRenderPass(priCmd, &beginInfo_, subpassContents);
    }

    virtual inline void endPass(const uint8_t &frameIndex) {
        auto &primaryCmd = data.priCmds[frameIndex];
        vkCmdEndRenderPass(primaryCmd);
        //// End current debug marker region
        // ext::DebugMarkerEnd(priCmd);
        vk::assert_success(vkEndCommandBuffer(primaryCmd));
    }

    virtual void beginSecondary(const uint8_t &frameIndex) {}
    virtual void endSecondary(const uint8_t &frameIndex, const VkCommandBuffer &priCmd) {}

    virtual void getSubmitResource(const uint8_t &frameIndex, SubmitResource &resource) = 0;

    // SETTINGS
    InitInfo initInfo;
    FrameInfo frameInfo;

    // SUBPASS
    inline uint32_t getSubpassId(const PIPELINE &type) const {
        uint32_t id = 0;
        for (const auto &pipelineType : PIPELINE_TYPES) {
            if (pipelineType == type) break;
            id++;
        }
        return id;  // TODO: is returning 0 for no types okay???
    };

    virtual void destroy(const VkDevice &dev);  // calls destroyFrameData
    virtual void destroyTargetResources(const VkDevice &dev);

    // TODO: Scene as friend?
    VkRenderPass pass;
    Data data;

   protected:
    Base(std::string &&name, std::unordered_set<PIPELINE> &&types)
        : PIPELINE_TYPES(types), initInfo(), frameInfo(), pass(VK_NULL_HANDLE), NAME(name) {}

    // RENDER PASS
    virtual void createPass(const VkDevice &dev);
    virtual void createClearValues(const Shell::Context &ctx, const Game::Settings &settings) {
        // TODO: some default behavior when "clearColor" or "clearDefault" is set.
        assert(!initInfo.clearColor && !initInfo.clearDepth);
    }
    virtual void createBeginInfo(const Shell::Context &ctx, const Game::Settings &settings);
    virtual void updateBeginInfo(const Shell::Context &ctx, const Game::Settings &settings);  // TODO: what should this be?
    virtual void createViewport();
    std::vector<VkClearValue> clearValues_;
    VkRect2D scissor_;
    VkViewport viewport_;
    VkRenderPassBeginInfo beginInfo_;

    // SUBPASS
    virtual void createAttachmentsAndSubpasses(const Shell::Context &ctx, const Game::Settings &settings) = 0;
    virtual void createDependencies(const Shell::Context &ctx, const Game::Settings &settings) = 0;
    SubpassResources subpassResources_;

    // FRAME DATA
    virtual void createCommandBuffers(const Shell::Context &ctx);
    virtual void createColorResources(const Shell::Context &ctx, const Game::Settings &settings) {}
    virtual void createDepthResource(const Shell::Context &ctx, const Game::Settings &settings) {}
    virtual void createFramebuffers(const Shell::Context &ctx, const Game::Settings &settings) {}
    virtual void createFences(const Shell::Context &ctx, VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT);

    // ATTACHMENT
    std::vector<ImageResource> colors_;
    ImageResource depth_;

   private:
    void createSemaphores(const Shell::Context &ctx);
    void RenderPass::Base::createAttachmentDebugMarkers(const Shell::Context &ctx, const Game::Settings &settings);
};

// **********************
//      Default
// **********************

class Default : public Base {
   public:
    Default()
        : Base{"Default",
               {
                   // Order of the subpasses
                   PIPELINE::PBR_COLOR,
                   PIPELINE::TRI_LIST_COLOR,
                   PIPELINE::LINE,
                   PIPELINE::TRI_LIST_TEX,
               }},
          secCmdFlag_(false) {}

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

    void getSubmitResource(const uint8_t &frameIndex, SubmitResource &resource) override;

   private:
    void createClearValues(const Shell::Context &ctx, const Game::Settings &settings) override;
    void createBeginInfo(const Shell::Context &ctx, const Game::Settings &settings) override;
    VkCommandBufferInheritanceInfo inheritInfo_;
    VkCommandBufferBeginInfo secCmdBeginInfo_;
    bool secCmdFlag_;

    void createCommandBuffers(const Shell::Context &ctx) override;
    void createColorResources(const Shell::Context &ctx, const Game::Settings &settings) override;
    void createDepthResource(const Shell::Context &ctx, const Game::Settings &settings) override;
    void createAttachmentsAndSubpasses(const Shell::Context &ctx, const Game::Settings &settings) override;
    void createDependencies(const Shell::Context &ctx, const Game::Settings &settings) override;
    void createFramebuffers(const Shell::Context &ctx, const Game::Settings &settings) override;
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_H
