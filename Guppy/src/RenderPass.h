#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Handlee.h"
#include "Helpers.h"

// clang-format off
namespace Pipeline { class Base; }
// clang-format on

namespace RenderPass {

class Handler;

class Base : public Handlee<RenderPass::Handler> {
    friend class Pipeline::Base;

   public:
    virtual ~Base() = default;

    const std::string NAME;
    const offset OFFSET;
    const bool SWAPCHAIN_DEPENDENT;
    const RENDER_PASS TYPE;

    // LIFECYCLE
    virtual void init() = 0;
    void create();
    virtual void postCreate() {}
    virtual void setSwapchainInfo() {}
    void createTarget();
    void overridePipelineCreateInfo(const PIPELINE &type, Pipeline::CreateInfoVkResources &createInfoRes);
    virtual void record(const uint32_t &frameIndex);

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

    virtual void updateSubmitResource(SubmitResource &resource, const uint32_t &frameIndex) const;

    // SETTINGS
    const VkFormat &getFormat() const { return format_; }
    const VkFormat &getDepthFormat() const { return depthFormat_; }
    const VkImageLayout &getFinalLayout() const { return finalLayout_; }
    const VkSampleCountFlagBits &getSamples() const { return pipelineData_.samples; }

    // SUBPASS
    uint32_t getSubpassId(const PIPELINE &type) const;

    // PIPELINE
    const auto &getPipelineBindDataMap() const { return pipelineTypeBindDataMap_; }
    void setBindData(const PIPELINE &pipelineType, const std::shared_ptr<Pipeline::BindData> &pBindData);
    inline auto getPipelineCount() const { return pipelineTypeBindDataMap_.size(); }
    inline std::set<PIPELINE> getPipelineTypes() {
        std::set<PIPELINE> types;
        for (const auto &keyValue : pipelineTypeBindDataMap_) types.insert(keyValue.first);
        return types;
    }
    inline void addPipelineTypes(std::set<PIPELINE> &pipelineTypes) {
        for (const auto &keyValue : pipelineTypeBindDataMap_) pipelineTypes.insert(keyValue.first);
    }
    inline const auto &getPipelineData() { return pipelineData_; }
    inline bool comparePipelineData(const std::unique_ptr<Base> &pOther) {
        return pipelineData_ == pOther->getPipelineData();
    }

    // UNIFORM
    const auto &getDescriptorPipelineOffsets() { return descPipelineOffsets_; }

    virtual void destroy();  // calls destroyFrameData
    virtual void destroyTargetResources();

    // TODO: Scene as friend?
    VkRenderPass pass;
    Data data;

   protected:
    Base(RenderPass::Handler &handler, const uint32_t &&offset, const RenderPass::CreateInfo *pCreateInfo);

    // PIPELINE
    PipelineData pipelineData_;
    std::unordered_map<PIPELINE, std::shared_ptr<Pipeline::BindData>> pipelineTypeBindDataMap_;  // Is this still necessary?

    // RENDER PASS
    virtual void createPass();
    virtual void createBeginInfo();
    virtual void updateBeginInfo();  // TODO: what should this be?
    virtual void createViewport();
    VkRect2D scissor_;
    VkViewport viewport_;
    VkRenderPassBeginInfo beginInfo_;

    // SUBPASS
    virtual void createAttachmentsAndSubpasses() = 0;
    virtual void createDependencies() = 0;
    Resources resources_;

    // FRAME DATA
    virtual void createCommandBuffers();
    virtual void createColorResources() {}
    virtual void createDepthResource() {}
    virtual void updateClearValues() = 0;
    virtual void createFramebuffers() {}
    std::vector<VkClearValue> clearValues_;

    // SETTINGS
    VkFormat format_;
    VkFormat depthFormat_;
    VkImageLayout finalLayout_;
    uint32_t commandCount_;
    uint32_t semaphoreCount_;
    VkExtent2D extent_;

    // ATTACHMENT
    std::vector<ImageResource> images_;
    ImageResource depth_;

    // UNIFORM
    descriptorPipelineOffsetsMap descPipelineOffsets_;

   private:
    void createSemaphores();
    void createAttachmentDebugMarkers();
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_H
