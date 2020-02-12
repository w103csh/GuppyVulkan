/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "ConstantsAll.h"
#include "Handlee.h"
#include "Helpers.h"
#include "Texture.h"

// clang-format off
namespace Descriptor { class Base; }
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
    constexpr bool isIntialized() const { return isInitialized_; }

    // LIFECYCLE
    void create();
    virtual void postCreate() {}
    virtual void setSwapchainInfo();
    void createTarget();
    void overridePipelineCreateInfo(const PIPELINE &type, Pipeline::CreateInfoResources &createInfoRes);
    virtual void record(const uint8_t frameIndex);
    virtual void update(const std::vector<Descriptor::Base *> pDynamicItems = {});
    constexpr auto getStatus() const { return status_; }

    // PRIMARY
    virtual void beginPass(const vk::CommandBuffer &cmd, const uint8_t frameIndex,
                           vk::SubpassContents &&subpassContents = vk::SubpassContents::eInline) const;
    virtual void endPass(const vk::CommandBuffer &cmd) const;

    // SECONDARY
    virtual void beginSecondary(const uint8_t frameIndex);
    virtual void endSecondary(const uint8_t frameIndex, const vk::CommandBuffer &priCmd);

    virtual void updateSubmitResource(SubmitResource &resource, const uint8_t frameIndex) const;

    // SETTINGS
    constexpr const auto &getFormat() const { return format_; }
    constexpr const auto &getDepthFormat() const { return depthFormat_; }
    constexpr const auto &getFinalLayout() const { return finalLayout_; }
    constexpr const auto &getInitialLayout() const { return initialLayout_; }
    constexpr const auto &getSamples() const { return pipelineData_.samples; }
    constexpr bool usesSwapchain() const { return FLAGS & FLAG::SWAPCHAIN; }
    constexpr bool usesDepth() const { return FLAGS & FLAG::DEPTH; }
    constexpr bool usesDepthInputAttachment() const { return FLAGS & FLAG::DEPTH_INPUT_ATTACHMENT; }
    constexpr bool usesMultiSample() const {
        return FLAGS & FLAG::MULTISAMPLE && getSamples() != vk::SampleCountFlagBits::e1;
    }
    constexpr bool usesSecondaryCommands() const { return FLAGS & FLAG::SECONDARY_COMMANDS; }
    inline bool hasTargetSampler() const { return pTextures_.size(); }
    inline bool hasTargetSwapchain() const { return getTargetId() == SWAPCHAIN_TARGET_ID; }

    // SUBPASS
    uint32_t getSubpassId(const PIPELINE &type) const;
    void setSubpassOffsets(const std::vector<std::unique_ptr<Base>> &pPasses);

    constexpr const auto &getDependentTypeOffsetPairs() const { return dependentTypeOffsetPairs_; }

    // PIPELINE
    constexpr const auto &getPipelineBindDataList() const { return pipelineBindDataList_; }
    constexpr const auto &getPipelineData() const { return pipelineData_; }
    inline auto getPipelineCount() const { return pipelineBindDataList_.size(); }
    inline std::set<PIPELINE> getPipelineTypes() {
        std::set<PIPELINE> types;
        for (const auto &keyValue : pipelineBindDataList_.getKeyOffsetMap()) types.insert(keyValue.first);
        return types;
    }
    inline void addPipelineTypes(std::set<PIPELINE> &pipelineTypes) const {
        for (const auto &keyValue : pipelineBindDataList_.getKeyOffsetMap()) pipelineTypes.insert(keyValue.first);
    }
    constexpr bool comparePipelineData(const std::unique_ptr<Base> &pOther) const {
        return pipelineData_ == pOther->getPipelineData();
    }
    void setBindData(const PIPELINE &pipelineType, const std::shared_ptr<Pipeline::BindData> &pBindData);

    // DESCRIPTOR SET
    inline const auto &getDescSetBindDataMap(const PIPELINE pipelineType) const {
        return pipelineDescSetBindDataMap_.at(pipelineType);
    }

    // UNIFORM
    inline constexpr const auto &getDescriptorPipelineOffsets() { return descPipelineOffsets_; }

    const std::string &getTargetId() const { return textureIds_[0]; }

    virtual void destroy();  // calls destroyFrameData
    virtual void destroyTargetResources();

    // TODO: Scene as friend?
    vk::RenderPass pass;
    Data data;

   protected:
    FlagBits status_;

    // PIPELINE
    PipelineData pipelineData_;
    Pipeline::pipelineBindDataList pipelineBindDataList_;  // Is this still necessary?

    // DESCRIPTOR SET
    std::map<PIPELINE, Descriptor::Set::bindDataMap> pipelineDescSetBindDataMap_;

    // RENDER PASS
    virtual void createPass();
    virtual void createBeginInfo();
    virtual void updateBeginInfo();  // TODO: what should this be?
    virtual void createViewports();
    std::vector<vk::Rect2D> scissors_;
    std::vector<vk::Viewport> viewports_;
    vk::RenderPassBeginInfo beginInfo_;

    // SUBPASS
    virtual void createAttachments();
    virtual void createSubpassDescriptions();
    virtual void createDependencies();
    Resources resources_;
    std::vector<std::pair<PASS, index>> dependentTypeOffsetPairs_;

    // FRAME DATA
    virtual void createCommandBuffers();
    virtual void createImageResources();
    virtual void createDepthResource();
    virtual void updateClearValues();
    virtual void createFramebuffers();
    std::vector<vk::ClearValue> clearValues_;

    // SECONDARY
    vk::CommandBufferInheritanceInfo inheritInfo_;
    vk::CommandBufferBeginInfo secCmdBeginInfo_;
    bool secCmdFlag_;

    // ATTACHMENT
    std::vector<ImageResource> images_;
    ImageResource depth_;

    // UNIFORM
    descriptorPipelineOffsetsMap descPipelineOffsets_;

    // SAMPLER
    std::vector<std::string> textureIds_;  // TODO: const? Swapchain has an id now.
    std::vector<std::shared_ptr<::Texture::Base>> pTextures_;

    // BARRIER
    BarrierResource barrierResource_;

    vk::Extent2D extent_;

   private:
    /* It is important that this remains private and is set in Base::init. That way
     *  Base::create can validate that Base::init was called because Base::init
     *  governs the hanlder's clear flag map.
     */
    bool isInitialized_;

    // SETTINGS
    vk::Format format_;
    vk::Format depthFormat_;
    vk::ImageLayout initialLayout_;
    vk::ImageLayout finalLayout_;
    uint32_t commandCount_;
    uint32_t semaphoreCount_;

    void createSemaphores();
    void createAttachmentDebugMarkers();
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_H
