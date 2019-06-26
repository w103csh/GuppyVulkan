#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

#include "ConstantsAll.h"
#include "Handlee.h"
#include "Helpers.h"
#include "Texture.h"

// clang-format off
namespace Pipeline { class Base; }
// clang-format on

namespace RenderPass {

class Handler;

class Base : public Handlee<RenderPass::Handler> {
    friend class Pipeline::Base;

   public:
    Base(RenderPass::Handler &handler, const uint32_t &&offset, const RenderPass::CreateInfo *pCreateInfo);
    virtual ~Base() = default;

    const FlagBits FLAGS;
    const std::string NAME;
    const offset OFFSET;
    const RENDER_PASS TYPE;

    virtual void init(bool isFinal = false);

    // LIFECYCLE
    void create();
    virtual void postCreate() {}
    virtual void setSwapchainInfo();
    void createTarget();
    void overridePipelineCreateInfo(const PIPELINE &type, Pipeline::CreateInfoVkResources &createInfoRes);
    virtual void record(const uint32_t &frameIndex);

    // PRIMARY
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

    // SECONDARY
    virtual void beginSecondary(const uint8_t &frameIndex);
    virtual void endSecondary(const uint8_t &frameIndex, const VkCommandBuffer &priCmd);

    virtual void updateSubmitResource(SubmitResource &resource, const uint32_t &frameIndex) const;

    // SETTINGS
    constexpr const auto &getFormat() const { return format_; }
    constexpr const auto &getDepthFormat() const { return depthFormat_; }
    constexpr const auto &getFinalLayout() const { return finalLayout_; }
    constexpr const auto &getSamples() const { return pipelineData_.samples; }
    constexpr bool usesSwapchain() const { return FLAGS & FLAG::SWAPCHAIN; }
    constexpr bool usesDepth() const { return FLAGS & FLAG::DEPTH; }
    constexpr bool usesMultiSample() const { return FLAGS & FLAG::MULTISAMPLE; }
    constexpr bool usesSecondaryCommands() const { return FLAGS & FLAG::SECONDARY_COMMANDS; }
    inline bool hasTargetSampler() const { return pTexture_ != nullptr; }
    inline bool hasTargetSwapchain() const { return pTexture_ == nullptr; }

    // SUBPASS
    uint32_t getSubpassId(const PIPELINE &type) const;

    // PIPELINE
    constexpr const auto &getPipelineBindDataMap() const { return pipelineTypeBindDataMap_; }
    constexpr const auto &getPipelineData() const { return pipelineData_; }
    inline auto getPipelineCount() const { return pipelineTypeBindDataMap_.size(); }
    inline std::set<PIPELINE> getPipelineTypes() {
        std::set<PIPELINE> types;
        for (const auto &keyValue : pipelineTypeBindDataMap_) types.insert(keyValue.first);
        return types;
    }
    inline void addPipelineTypes(std::set<PIPELINE> &pipelineTypes) const {
        for (const auto &keyValue : pipelineTypeBindDataMap_) pipelineTypes.insert(keyValue.first);
    }
    constexpr bool comparePipelineData(const std::unique_ptr<Base> &pOther) const {
        return pipelineData_ == pOther->getPipelineData();
    }
    void setBindData(const PIPELINE &pipelineType, const std::shared_ptr<Pipeline::BindData> &pBindData);

    // UNIFORM
    inline constexpr const auto &getDescriptorPipelineOffsets() { return descPipelineOffsets_; }

    const auto &getTargetId() { return textureIds_[0]; }

    virtual void destroy();  // calls destroyFrameData
    virtual void destroyTargetResources();

    // TODO: Scene as friend?
    VkRenderPass pass;
    Data data;

   protected:
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
    virtual void createAttachmentsAndSubpasses();
    virtual void createDependencies();
    Resources resources_;

    // FRAME DATA
    virtual void createCommandBuffers();
    virtual void createImageResources();
    virtual void createDepthResource();
    virtual void updateClearValues();
    virtual void createFramebuffers();
    std::vector<VkClearValue> clearValues_;

    // SECONDARY
    VkCommandBufferInheritanceInfo inheritInfo_;
    VkCommandBufferBeginInfo secCmdBeginInfo_;
    bool secCmdFlag_;

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

    // SAMPLER
    std::vector<std::string> textureIds_;  // oh well, and it should be const
    std::shared_ptr<Texture::Base> pTexture_;

   private:
    void createSemaphores();
    void createAttachmentDebugMarkers();
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_H
