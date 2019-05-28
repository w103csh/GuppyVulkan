#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <string>
#include <unordered_set>
#include <vector>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Constants.h"
#include "Handlee.h"
#include "Helpers.h"

// clang-format off
namespace Pipeline { class Base; }
// clang-format on

namespace RenderPass {

class Handler;

// **********************
//      Base
// **********************

class Base : public Handlee<RenderPass::Handler> {
    friend class Pipeline::Base;

   public:
    virtual ~Base() = default;

    const std::string NAME;
    const std::unordered_set<PIPELINE> PIPELINE_TYPES;

    const bool CLEAR_COLOR;
    const bool CLEAR_DEPTH;

    // LIFECYCLE
    virtual void init() = 0;
    void create();
    virtual void postCreate() {}
    virtual void setSwapchainInfo() {}
    void createTarget();
    virtual void record() = 0;

    virtual void beginPass(const uint8_t &frameIndex,
                           VkCommandBufferUsageFlags &&primaryCommandUsage = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
                           VkSubpassContents &&subpassContents = VK_SUBPASS_CONTENTS_INLINE);

    virtual inline void endPass(const uint8_t &frameIndex) {
        auto &primaryCmd = data.priCmds[frameIndex];
        vkCmdEndRenderPass(primaryCmd);
        //// End current debug marker region
        // ext::DebugMarkerEnd(priCmd);
        vk::assert_success(vkEndCommandBuffer(primaryCmd));
    }

    virtual void beginSecondary(const uint8_t &frameIndex) {}
    virtual void endSecondary(const uint8_t &frameIndex, const VkCommandBuffer &priCmd) {}

    virtual void getSubmitResource(const uint8_t &frameIndex, SubmitResource &resource) const = 0;

    // SUBPASS
    inline uint32_t getSubpassId(const PIPELINE &type) const {
        uint32_t id = 0;
        for (const auto &pipelineType : PIPELINE_TYPES) {
            if (pipelineType == type) break;
            id++;
        }
        return id;  // TODO: is returning 0 for no types okay? Try using std::optional maybe?
    };

    virtual void destroy();  // calls destroyFrameData
    virtual void destroyTargetResources();

    // TODO: Scene as friend?
    VkRenderPass pass;
    Data data;

   protected:
    Base(RenderPass::Handler &handler, const std::string &&name, const RenderPass::CreateInfo createInfo = {});

    // RENDER PASS
    virtual void createPass();
    virtual void createClearValues() {
        // TODO: some default behavior when "clearColor" or "clearDefault" is set.
        assert(!CLEAR_COLOR && !CLEAR_DEPTH);
    }
    virtual void createBeginInfo();
    virtual void updateBeginInfo();  // TODO: what should this be?
    virtual void createViewport();
    std::vector<VkClearValue> clearValues_;
    VkRect2D scissor_;
    VkViewport viewport_;
    VkRenderPassBeginInfo beginInfo_;

    // SUBPASS
    virtual void createAttachmentsAndSubpasses() = 0;
    virtual void createDependencies() = 0;
    SubpassResources subpassResources_;

    // FRAME DATA
    virtual void createCommandBuffers();
    virtual void createColorResources() {}
    virtual void createDepthResource() {}
    virtual void createFramebuffers() {}
    virtual void createFences(VkFenceCreateFlags flags = VK_FENCE_CREATE_SIGNALED_BIT);

    // SETTINGS
    VkFormat format_;
    VkFormat depthFormat_;
    VkImageLayout finalLayout_;
    VkSampleCountFlagBits samples_;
    uint32_t commandCount_;
    uint32_t fenceCount_;
    uint32_t semaphoreCount_;
    VkPipelineStageFlags waitDstStageMask_;
    VkExtent2D extent_;
    uint32_t viewCount_;
    const VkImageView *pViews_;

    // ATTACHMENT
    std::vector<ImageResource> colors_;
    ImageResource depth_;

   private:
    void createSemaphores();
    void createAttachmentDebugMarkers();
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_H
