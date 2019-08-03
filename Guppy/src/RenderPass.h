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
namespace Pipeline { class Graphics; }
// clang-format on

namespace RenderPass {

class Handler;

class Base : public Handlee<RenderPass::Handler> {
    friend class Pipeline::Graphics;

   public:
    Base(RenderPass::Handler &handler, const index &&offset, const RenderPass::CreateInfo *pCreateInfo);
    virtual ~Base() = default;

    const FlagBits FLAGS;
    const std::string NAME;
    const index OFFSET;
    const PASS TYPE;

    virtual void init();
    constexpr const bool isIntialized() const { return isInitialized_; }

    // LIFECYCLE
    void create();
    virtual void postCreate() {}
    virtual void setSwapchainInfo();
    void createTarget();
    void overridePipelineCreateInfo(const PIPELINE &type, Pipeline::CreateInfoResources &createInfoRes);
    virtual void record(const uint8_t frameIndex);
    virtual void postDraw(const VkCommandBuffer &cmd, const uint8_t frameIndex);
    virtual void update() {}
    constexpr const auto getStatus() const { return status_; }

    // PRIMARY
    virtual void beginPass(const VkCommandBuffer &cmd, const uint8_t frameIndex,
                           VkSubpassContents &&subpassContents = VK_SUBPASS_CONTENTS_INLINE) const;
    virtual void endPass(const VkCommandBuffer &cmd) const;

    // SECONDARY
    virtual void beginSecondary(const uint8_t frameIndex);
    virtual void endSecondary(const uint8_t frameIndex, const VkCommandBuffer &priCmd);

    virtual void updateSubmitResource(SubmitResource &resource, const uint8_t frameIndex) const;

    // SETTINGS
    constexpr const auto &getFormat() const { return format_; }
    constexpr const auto &getDepthFormat() const { return depthFormat_; }
    constexpr const auto &getFinalLayout() const { return finalLayout_; }
    constexpr const auto &getSamples() const { return pipelineData_.samples; }
    constexpr bool usesSwapchain() const { return FLAGS & FLAG::SWAPCHAIN; }
    constexpr bool usesDepth() const { return FLAGS & FLAG::DEPTH; }
    constexpr bool usesMultiSample() const { return FLAGS & FLAG::MULTISAMPLE; }
    constexpr bool usesSecondaryCommands() const { return FLAGS & FLAG::SECONDARY_COMMANDS; }
    inline bool hasTargetSampler() const { return pTextures_.size(); }
    inline bool hasTargetSwapchain() const { return getTargetId() == SWAPCHAIN_TARGET_ID; }

    // SUBPASS
    uint32_t getSubpassId(const PIPELINE &type) const;
    void setSubpassOffsets(const std::vector<std::unique_ptr<Base>> &pPasses);

    constexpr const auto &getDependentTypeOffsetPairs() const { return dependentTypeOffsetPairs_; }

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

    // DESCRIPTOR SET
    constexpr const auto &getDescSetBindDataMap() const { return descSetBindDataMap_; }

    // UNIFORM
    inline constexpr const auto &getDescriptorPipelineOffsets() { return descPipelineOffsets_; }

    const std::string &getTargetId() const { return textureIds_[0]; }

    virtual void destroy();  // calls destroyFrameData
    virtual void destroyTargetResources();

    // TODO: Scene as friend?
    VkRenderPass pass;
    Data data;

   protected:
    FlagBits status_;

    // PIPELINE
    PipelineData pipelineData_;
    std::unordered_map<PIPELINE, std::shared_ptr<Pipeline::BindData>> pipelineTypeBindDataMap_;  // Is this still necessary?

    // DESCRIPTOR SET
    Descriptor::Set::bindDataMap descSetBindDataMap_;

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
    std::vector<std::pair<PASS, index>> dependentTypeOffsetPairs_;

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
    std::vector<std::shared_ptr<Texture::Base>> pTextures_;

    // BARRIER
    BarrierResource barrierResource_;

   private:
    /* It is important that this remains private and is set in Base::init. That way
     *  Base::create can validate that Base::init was called because Base::init
     *  governs the hanlder's clear flag map.
     */
    bool isInitialized_;

    void createSemaphores();
    void createAttachmentDebugMarkers();
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_H
