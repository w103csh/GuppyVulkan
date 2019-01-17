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
    // MULTI-SAMPLE (TODO: attachment vectors?)
    bool hasColor = false;
    VkClearColorValue colorClearColorValue = {};
    // DEPTH (TODO: attachment vectors?)
    bool hasDepth = false;
    VkClearDepthStencilValue depthClearValue = {};
    VkFormat depthFormat = {};

    VkFormat format = {};
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
};

struct FrameInfo {
    VkExtent2D extent = {0, 0};
    uint32_t viewCount;
    VkImageView *pViews = nullptr;
};

struct FrameData {
    // MULTI-SAMPLE
    ImageResource colorResource = {};
    // DEPTH
    ImageResource depthResource = {};

    VkViewport viewport;
    VkRect2D scissor;

    // Signaled when this struct is ready for reuse.
    std::vector<VkFence> fences;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> priCmds;
    std::vector<VkCommandBuffer> secCmds;
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
    void init(const Shell::Context &ctx, const Game::Settings &settings, RenderPass::InitInfo *pInfo);
    virtual void createTarget(const Shell::Context &ctx, const Game::Settings &settings, RenderPass::FrameInfo *pInfo);

    virtual inline void beginPass(
        const uint8_t &frameIndex,
        VkCommandBufferUsageFlags &&primaryCommandUsage = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        VkSubpassContents &&subpassContents = VK_SUBPASS_CONTENTS_INLINE) {
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
        vkCmdSetViewport(priCmd, 0, 1, &data.viewport);
        vkCmdSetScissor(priCmd, 0, 1, &data.scissor);

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

    virtual void beginSecondary(const uint8_t &frameIndex) = 0;
    virtual void endSecondary(const uint8_t &frameIndex, const VkCommandBuffer &priCmd) = 0;

    virtual void getSubmitCommands(const uint8_t &frameIndex, std::vector<VkCommandBuffer> &cmds) = 0;

    std::unordered_set<PIPELINE_TYPE> PIPELINE_TYPES;

    // SETTINGS
    InitInfo initInfo;
    FrameInfo frameInfo;

    // SUBPASS
    inline uint32_t getSubpassId(const PIPELINE_TYPE &type) const {
        uint32_t id = 0;
        for (const auto &pipelineType : PIPELINE_TYPES) {
            if (pipelineType == type) break;
            id++;
        }
        return id;  // TODO: is returning 0 for no types okay???
    };

    virtual void destroy(const VkDevice &dev);  // calls destroyFrameData
    virtual void destroyTarget(const VkDevice &dev);

    // TODO: Scene as friend?
    VkRenderPass pass;
    FrameData data;

   protected:
    Base(std::string &&name, std::unordered_set<PIPELINE_TYPE> &&types)
        : PIPELINE_TYPES(types), initInfo(), frameInfo(), pass(VK_NULL_HANDLE), attachmentCount_(0), name_(name){};

    // RENDER PASS
    virtual void createPass(const VkDevice &dev);
    virtual void createClearValues(const Shell::Context &ctx, const Game::Settings &settings) {}
    std::vector<VkClearValue> clearValues_;
    virtual void createBeginInfo(const Shell::Context &ctx, const Game::Settings &settings);
    VkRenderPassBeginInfo beginInfo_;

    // SUBPASS
    void createSubpassResources(const Shell::Context &ctx, const Game::Settings &settings);
    virtual void createSubpassesAndAttachments(const Shell::Context &ctx, const Game::Settings &settings) = 0;
    virtual void createDependencies(const Shell::Context &ctx, const Game::Settings &settings) = 0;
    SubpassResources subpassResources_;

    // FRAME DATA
    void createFrameData(const Shell::Context &ctx, const Game::Settings &settings);
    virtual void createCommandBuffers(const Shell::Context &ctx);
    virtual void createImages(const Shell::Context &ctx, const Game::Settings &settings);  // TODO: bad name
    virtual void createColorResources(const Shell::Context &ctx);                          // TODO: bad name
    virtual void createDepthResources(const Shell::Context &ctx);                          // TODO: bad name
    virtual void createViewport();
    virtual void createImageViews(const Shell::Context &ctx) = 0;
    virtual void createFramebuffers(const Shell::Context &ctx) = 0;  // TODO: Pure virtual or empty?
    virtual void createFences(const Shell::Context &ctx,
                              VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT) = 0;  // TODO: Pure virtual or empty?
    virtual void updateBeginInfo(const Shell::Context &ctx, const Game::Settings &settings);

    // ATTACHMENTS
    uint32_t attachmentCount_;

   private:
    std::string name_;
};

// **********************
//      Default
// **********************

class Default : public Base {
   public:
    Default()
        : Base("Default",
               {
                   // Order of the subpasses
                   PIPELINE_TYPE::TRI_LIST_TEX,
                   PIPELINE_TYPE::LINE,
                   PIPELINE_TYPE::TRI_LIST_COLOR,
               }),
          secCmdFlag_(false) {}

    inline void beginSecondary(const uint8_t &frameIndex) override {
        // FRAME UPDATE
        auto &secCmd = data.secCmds[frameIndex];
        inheritInfo_.framebuffer = data.framebuffers[frameIndex];
        // COMMAND
        vkResetCommandBuffer(secCmd, 0);
        vk::assert_success(vkBeginCommandBuffer(secCmd, &secCmdBeginInfo_));
        // FRAME COMMANDS
        vkCmdSetViewport(secCmd, 0, 1, &data.viewport);
        vkCmdSetScissor(secCmd, 0, 1, &data.scissor);

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

    void getSubmitCommands(const uint8_t &frameIndex, std::vector<VkCommandBuffer> &cmds) override {
        cmds.push_back(data.priCmds[frameIndex]);
        // if (secCmdFlag_) cmds.push_back(data.secCmds[frameIndex]);
    };

   private:
    void createClearValues(const Shell::Context &ctx, const Game::Settings &settings) override;
    void createBeginInfo(const Shell::Context &ctx, const Game::Settings &settings) override;
    VkCommandBufferInheritanceInfo inheritInfo_;
    VkCommandBufferBeginInfo secCmdBeginInfo_;
    bool secCmdFlag_;

    void createCommandBuffers(const Shell::Context &ctx) override;
    void createSubpassesAndAttachments(const Shell::Context &ctx, const Game::Settings &settings) override;
    void createDependencies(const Shell::Context &ctx, const Game::Settings &settings) override;
    void createFences(const Shell::Context &ctx, VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT) override;
    void createImageViews(const Shell::Context &ctx) override{};
    void createFramebuffers(const Shell::Context &ctx) override;
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_H
