/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "RenderPass.h"

#include <Common/Helpers.h>

#include "Descriptor.h"
#include "RenderPassManager.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "DescriptorHandler.h"
#include "MaterialHandler.h"
#include "PipelineHandler.h"
#include "PassHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

RenderPass::Base::Base(Pass::Handler& handler, const uint32_t&& offset, const CreateInfo* pCreateInfo)
    : Handlee(handler),
      FLAGS(pCreateInfo->flags),
      NAME(pCreateInfo->name),
      OFFSET(offset),
      TYPE(pCreateInfo->type),
      pass(),
      data{},
      status_(STATUS::UNKNOWN),
      pipelineData_{usesDepth(), {}},
      beginInfo_{},
      // SECONDARY
      inheritInfo_{},
      secCmdBeginInfo_{},
      secCmdFlag_(false),
      depth_{},
      descPipelineOffsets_(pCreateInfo->descPipelineOffsets),
      textureIds_(pCreateInfo->textureIds),
      extent_(BAD_EXTENT_2D),
      isInitialized_(false),
      // SETTINGS
      format_(vk::Format::eUndefined),
      depthFormat_(vk::Format::eUndefined),
      initialLayout_(pCreateInfo->initialLayout),
      finalLayout_(pCreateInfo->finalLayout),
      commandCount_(0),
      semaphoreCount_(0) {
    // This is sloppy, but I don't really feel like changing a bunch of things.
    if (textureIds_.empty()) textureIds_.emplace_back(std::string(SWAPCHAIN_TARGET_ID));
    assert(textureIds_.size());

    // Create dependency list. Includes itself in order.
    for (const auto& passType : pCreateInfo->prePassTypes) dependentTypeOffsetPairs_.push_back({passType, BAD_OFFSET});
    dependentTypeOffsetPairs_.push_back({TYPE, BAD_OFFSET});
    for (const auto& passType : pCreateInfo->postPassTypes) dependentTypeOffsetPairs_.push_back({passType, BAD_OFFSET});

    for (const auto& pipelineType : pCreateInfo->pipelineTypes) {
        // If changed from unique list (set) then a lot of work needs to be done, so I am
        // adding some validation here.
        assert(pipelineBindDataList_.getOffset(pipelineType) == -1);
        pipelineBindDataList_.insert(pipelineType, nullptr);
    }
}

void RenderPass::Base::init() {
    assert(!isInitialized_);

    auto& ctx = handler().shell().context();

    // Find the texture targets if not the swapchain.
    if (getTargetId() != RenderPass::SWAPCHAIN_TARGET_ID) {
        for (const auto& textureId : textureIds_) {
            auto pTexture = handler().textureHandler().getTexture(textureId);
            if (pTexture != nullptr) {
                pTextures_.push_back(pTexture);
            } else {
                uint8_t frameIndex = 0;
                do {
                    pTexture = handler().textureHandler().getTexture(textureId, frameIndex++);
                    if (pTexture != nullptr) pTextures_.push_back(pTexture);
                } while (pTexture != nullptr);
            }
        }
        assert(pTextures_.size());
    }

    if (hasTargetSampler()) {
        assert(pTextures_.size());
        if (usesSwapchain()) {
            if (pTextures_[0]->samplers[0].imgCreateInfo.format == vk::Format::eUndefined)
                format_ = ctx.surfaceFormat.format;
            else
                format_ = pTextures_[0]->samplers[0].imgCreateInfo.format;
        } else {
            // Force only one sampler, for now, and just assume its the target.
            for (auto i = 0; i < pTextures_.size(); i++) {
                assert(pTextures_[i]->samplers.size() == 1);
                if (i == 0) {
                    extent_.width = pTextures_[i]->samplers[0].imgCreateInfo.extent.width;
                    extent_.height = pTextures_[i]->samplers[0].imgCreateInfo.extent.height;
                    format_ = pTextures_[i]->samplers[0].imgCreateInfo.format;
                } else {
                    assert(pTextures_[i]->samplers[0].imgCreateInfo.extent.depth == 1);
                    assert(extent_.width == pTextures_[i]->samplers[0].imgCreateInfo.extent.width);
                    assert(extent_.height == pTextures_[i]->samplers[0].imgCreateInfo.extent.height);
                    assert(format_ == pTextures_[i]->samplers[0].imgCreateInfo.format);
                }
            }
        }
    } else {
        format_ = ctx.surfaceFormat.format;
    }

    depthFormat_ = usesDepth() ? ctx.depthFormat : vk::Format::eUndefined;
    pipelineData_.samples = usesMultiSample() ? ctx.samples : vk::SampleCountFlagBits::e1;

    // Validate frame settings.
    assert(initialLayout_ != vk::ImageLayout::eUndefined);  // I think this is the case.
    assert(finalLayout_ != vk::ImageLayout::eUndefined);
    if (pipelineData_.usesDepth)
        assert(depthFormat_ != vk::Format::eUndefined);
    else
        assert(depthFormat_ == vk::Format::eUndefined);

    // SYNC
    commandCount_ = ctx.imageCount;

    // CLEAR
    if (handler().renderPassMgr().clearTargetMap_.count(getTargetId()) == 0)  //
        handler().renderPassMgr().clearTargetMap_[getTargetId()] = TYPE;
    // FINAL
    handler().renderPassMgr().finalTargetMap_[getTargetId()] = TYPE;

    isInitialized_ = true;
}

void RenderPass::Base::create() {
    assert(isInitialized_);

    // Update some settings based on the clear map.
    if (hasTargetSwapchain() && handler().renderPassMgr().isFinalTargetPass(getTargetId(), TYPE)) {
        finalLayout_ = vk::ImageLayout::ePresentSrcKHR;
    }
    // Not sure if this makes sense anymore
    if (finalLayout_ != vk::ImageLayout::ePresentSrcKHR) {
        semaphoreCount_ = handler().shell().context().imageCount;
        data.signalSrcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    }

    createAttachments();
    createSubpassDescriptions();
    createDependencies();

    createPass();

    createBeginInfo();
}

void RenderPass::Base::setSwapchainInfo() {
    assert(helpers::compExtent2D(extent_, BAD_EXTENT_2D));
    extent_ = handler().shell().context().extent;
}

void RenderPass::Base::createTarget() {
    if (usesSwapchain()) setSwapchainInfo();

    // FRAME
    createImageResources();
    if (usesDepth()) createDepthResource();
    createAttachmentDebugMarkers();
    updateClearValues();
    createFramebuffers();

    // RENDER PASS
    createCommandBuffers();
    createViewports();
    updateBeginInfo();

    // SYNC
    createSemaphores();

    // Validate signal semaphore creation. If semaphores are created
    // the mask should be set.
    if (data.semaphores.empty())
        assert(data.signalSrcStageMask == vk::PipelineStageFlagBits{});
    else
        assert(data.signalSrcStageMask != vk::PipelineStageFlagBits{});

    assert(data.priCmds.size() + data.secCmds.size() <= RESOURCE_SIZE);
    assert(data.semaphores.size() <= RESOURCE_SIZE);
}

void RenderPass::Base::overridePipelineCreateInfo(const PIPELINE& type, Pipeline::CreateInfoResources& createInfoRes) {
    assert(pipelineData_.samples != vk::SampleCountFlagBits{});
    createInfoRes.depthStencilStateInfo.depthTestEnable = pipelineData_.usesDepth;
    createInfoRes.depthStencilStateInfo.depthWriteEnable = pipelineData_.usesDepth;
    createInfoRes.multisampleStateInfo.rasterizationSamples = pipelineData_.samples;
    if (pipelineData_.samples == vk::SampleCountFlagBits::e1) {
        createInfoRes.multisampleStateInfo.sampleShadingEnable = VK_FALSE;
    }
}

void RenderPass::Base::record(const uint8_t frameIndex) {
    // FRAME UPDATE
    beginInfo_.framebuffer = data.framebuffers[frameIndex];
    auto& priCmd = data.priCmds[frameIndex];

    // RESET BUFFERS
    priCmd.reset({});

    // BEGIN BUFFERS
    vk::CommandBufferBeginInfo bufferInfo = {vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
    priCmd.begin(bufferInfo);

    beginPass(priCmd, frameIndex, vk::SubpassContents::eInline);
    // pPass->beginPass(frameIndex, vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
    // vk::SubpassContents::eSecondaryCommandBuffers);

    auto& secCmd = data.secCmds[frameIndex];
    auto& pScene = handler().sceneHandler().getActiveScene();

    auto it = pipelineBindDataList_.getValues().begin();
    while (it != pipelineBindDataList_.getValues().end()) {
        pScene->record(TYPE, (*it)->type, (*it), priCmd, secCmd, frameIndex);

        ++it;

        if (it != pipelineBindDataList_.getValues().end()) {
            priCmd.nextSubpass(vk::SubpassContents::eInline);
            // priCmd.nextSubpass(vk::SubpassContents::eSecondaryCommandBuffers);
        }
    }

    endPass(priCmd);

    // priCmd.end();
}

void RenderPass::Base::update(const std::vector<Descriptor::Base*> pDynamicItems) {
    assert(status_ & STATUS::PENDING_PIPELINE);

    bool isReady = true;
    for (const auto& [pipelineType, value] : pipelineBindDataList_.getKeyOffsetMap()) {
        if (Pipeline::MESHLESS.count(pipelineType) == 0) continue;

        // Get pipeline bind data, and check the status
        auto& pPipeline = handler().pipelineHandler().getPipeline(pipelineType);

        // If the pipeline is not ready try to update once.
        if (pPipeline->getStatus() != STATUS::READY) pPipeline->updateStatus();

        if (pPipeline->getStatus() == STATUS::READY) {
            if (pipelineDescSetBindDataMap_.count(pipelineType) == 0) {
                auto it = pipelineDescSetBindDataMap_.insert({pipelineType, {}});
                assert(it.second);

                // Get or make descriptor bind data.
                handler().descriptorHandler().getBindData(pipelineType, it.first->second, pDynamicItems);
                assert(it.first->second.size());
            }
        }

        isReady &= pPipeline->getStatus() == STATUS::READY;
    }

    if (isReady) {
        status_ ^= STATUS::PENDING_PIPELINE;
        assert(status_ == STATUS::READY);
    }
}

void RenderPass::Base::beginPass(const vk::CommandBuffer& cmd, const uint8_t frameIndex,
                                 vk::SubpassContents&& subpassContents) const {
    // TODO: remove the data member.
    const_cast<vk::RenderPassBeginInfo*>(&beginInfo_)->framebuffer = data.framebuffers[frameIndex];
    //// Start a new debug marker region
    // priCmd.debugMarkerBeginEXT("Render x scene", {0.2f, 0.3f, 0.4f, 1.0f});
    cmd.beginRenderPass(beginInfo_, subpassContents);
    // Frame commands
    cmd.setScissor(0, scissors_);
    cmd.setViewport(0, viewports_);
}

void RenderPass::Base::endPass(const vk::CommandBuffer& cmd) const {
    cmd.endRenderPass();
    //// End current debug marker region
    // cmd.debugMarkerEndEXT();
    // cmd.end();
}

void RenderPass::Base::createPass() {
    vk::RenderPassCreateInfo2 createInfo = {};
    // ATTACHMENTS
    createInfo.attachmentCount = static_cast<uint32_t>(resources_.attachments.size());
    createInfo.pAttachments = resources_.attachments.data();
    // SUBPASSES
    createInfo.subpassCount = static_cast<uint32_t>(resources_.subpasses.size());
    createInfo.pSubpasses = resources_.subpasses.data();
    // DEPENDENCIES
    createInfo.dependencyCount = static_cast<uint32_t>(resources_.dependencies.size());
    createInfo.pDependencies = resources_.dependencies.data();

    // handler().shell().log(Shell::LogPriority::LOG_INFO, ("Creating render pass: " + NAME).c_str());
    pass = handler().shell().context().dev.createRenderPass2(createInfo, handler().shell().context().pAllocator);
    assert(pass);
    // handler().shell().context().dbg.setMarkerName(pass, NAME.c_str());
}

void RenderPass::Base::createBeginInfo() {
    beginInfo_ = vk::RenderPassBeginInfo{};
    beginInfo_.renderPass = pass;

    if (usesSecondaryCommands()) {
        // INHERITANCE
        inheritInfo_ = vk::CommandBufferInheritanceInfo{};
        inheritInfo_.renderPass = pass;
        inheritInfo_.subpass = getSubpassId(GRAPHICS::TRI_LIST_TEX);
        // Validation layer: Cannot set inherited occlusionQueryEnable in begin() when device does not
        // support inheritedQueries.
        inheritInfo_.occlusionQueryEnable = VK_FALSE;  // TODO: not sure
        inheritInfo_.queryFlags = {};                  // TODO: not sure
        inheritInfo_.pipelineStatistics = {};          // TODO: not sure

        // BUFFER
        secCmdBeginInfo_ = {vk::CommandBufferUsageFlagBits::eSimultaneousUse |
                            vk::CommandBufferUsageFlagBits::eRenderPassContinue};
        secCmdBeginInfo_.pInheritanceInfo = &inheritInfo_;
    }
}

void RenderPass::Base::createAttachments() {
    // DEPTH ATTACHMENT
    if (pipelineData_.usesDepth) {
        resources_.attachments.push_back({});
        resources_.attachments.back().format = depthFormat_;
        resources_.attachments.back().samples = getSamples();
        resources_.attachments.back().loadOp = vk::AttachmentLoadOp::eClear;
        resources_.attachments.back().storeOp = vk::AttachmentStoreOp::eDontCare;
        resources_.attachments.back().stencilLoadOp = vk::AttachmentLoadOp::eClear;
        resources_.attachments.back().stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        resources_.attachments.back().initialLayout = vk::ImageLayout::eUndefined;
        resources_.attachments.back().finalLayout = helpers::getDepthStencilAttachmentLayout(depthFormat_);
        resources_.attachments.back().flags = {};
        // REFERENCE
        resources_.depthStencilAttachment = {
            static_cast<uint32_t>(resources_.attachments.size() - 1),
            helpers::getDepthStencilAttachmentLayout(depthFormat_),
            helpers::getDepthStencilAspectMask(depthFormat_),
        };
        // INPUT ATTACHMENT
        if (usesDepthInputAttachment()) {
            resources_.inputAttachments.push_back({
                resources_.depthStencilAttachment.attachment,
                helpers::getDepthStencilReadOnlyLayout(depthFormat_),
                helpers::getDepthStencilAspectMask(depthFormat_),
            });
        }
    }

    // COLOR ATTACHMENT (MULTI-SAMPLE)
    if (usesMultiSample()) {
        resources_.attachments.push_back({});
        resources_.attachments.back().format = format_;
        resources_.attachments.back().samples = getSamples();
        resources_.attachments.back().loadOp = vk::AttachmentLoadOp::eClear;
        resources_.attachments.back().storeOp = vk::AttachmentStoreOp::eStore;
        // TODO: Are these stencil ops right?
        resources_.attachments.back().stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        resources_.attachments.back().stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        resources_.attachments.back().initialLayout = vk::ImageLayout::eUndefined;
        resources_.attachments.back().finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
        resources_.attachments.back().flags = {};
        // REFERENCE
        resources_.colorAttachments.push_back({
            static_cast<uint32_t>(resources_.attachments.size() - 1),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageAspectFlagBits::eColor,
        });
    }

    bool isClear = handler().renderPassMgr().isClearTargetPass(getTargetId(), TYPE);

    // COLOR ATTACHMENT
    resources_.attachments.push_back({});
    resources_.attachments.back().format = format_;
    resources_.attachments.back().samples = usesMultiSample() ? vk::SampleCountFlagBits::e1 : getSamples();
    resources_.attachments.back().loadOp = isClear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
    resources_.attachments.back().storeOp = vk::AttachmentStoreOp::eStore;
    resources_.attachments.back().stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    resources_.attachments.back().stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    resources_.attachments.back().initialLayout = isClear ? vk::ImageLayout::eUndefined : initialLayout_;
    resources_.attachments.back().finalLayout = finalLayout_;
    resources_.attachments.back().flags = {};
    // REFERENCE
    if (usesMultiSample()) {
        // Set resolve to swapchain attachment
        resources_.resolveAttachments.push_back({
            static_cast<uint32_t>(resources_.attachments.size() - 1),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageAspectFlagBits::eColor,
        });
    } else {
        resources_.colorAttachments.push_back({
            static_cast<uint32_t>(resources_.attachments.size() - 1),
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageAspectFlagBits::eColor,
        });
    }
}

void RenderPass::Base::createSubpassDescriptions() {
    // TODO: below comes from pipeline ???

    // SUBPASS DESCRIPTION
    vk::SubpassDescription2 subpass = {};
    subpass.colorAttachmentCount = static_cast<uint32_t>(resources_.colorAttachments.size());
    subpass.pColorAttachments = resources_.colorAttachments.data();
    subpass.pResolveAttachments = resources_.resolveAttachments.data();
    subpass.pDepthStencilAttachment = pipelineData_.usesDepth ? &resources_.depthStencilAttachment : nullptr;

    resources_.subpasses.assign(getPipelineCount(), subpass);
}

void RenderPass::Base::createDependencies() {
    // 7.1. Render Pass Creation has examples of what this should be.
    vk::SubpassDependency2 dependency;
    for (uint32_t i = 0; i < pipelineBindDataList_.size() - 1; i++) {
        dependency = vk::SubpassDependency2{};
        dependency.srcSubpass = i;
        dependency.dstSubpass = i + 1;
        dependency.dependencyFlags = {};
        dependency.srcStageMask = vk::PipelineStageFlagBits::eAllGraphics;
        dependency.dstStageMask = vk::PipelineStageFlagBits::eAllGraphics;
        dependency.dstAccessMask =
            vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
        dependency.srcAccessMask =
            vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
        resources_.dependencies.push_back(dependency);
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        resources_.dependencies.push_back(dependency);
    }

    //// Desparate attempt to understand the pipline ordering issue below...
    // vk::SubpassDependency2 dependency = {};

    // dependency = {};
    // dependency.srcSubpass = 0;
    // dependency.dstSubpass = 2;
    // dependency.dependencyFlags = 0;
    // dependency.srcStageMask = vk::PipelineStageFlagBits::eAllGraphics;
    // dependency.dstStageMask = vk::PipelineStageFlagBits::eAllGraphics;
    // dependency.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite |
    // vk::AccessFlagBits::eDepthStencilAttachmentRead; dependency.srcAccessMask =
    // vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
    // resources.dependencies.push_back(dependency);

    // dependency = {};
    // dependency.srcSubpass = 1;
    // dependency.dstSubpass = 2;
    // dependency.dependencyFlags = 0;
    // dependency.srcStageMask = vk::PipelineStageFlagBits::eAllGraphics;
    // dependency.dstStageMask = vk::PipelineStageFlagBits::eAllGraphics;
    // dependency.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite |
    // vk::AccessFlagBits::eDepthStencilAttachmentRead; dependency.srcAccessMask =
    // vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
    // resources.dependencies.push_back(dependency);
}

void RenderPass::Base::createCommandBuffers() {
    // PRIMARY
    data.priCmds.resize(commandCount_);
    handler().commandHandler().createCmdBuffers(QUEUE::GRAPHICS, data.priCmds.data(), vk::CommandBufferLevel::ePrimary,
                                                commandCount_);
    if (usesSecondaryCommands() || true) {  // TODO: these things are never used
        // SECONDARY
        data.secCmds.resize(commandCount_);
        handler().commandHandler().createCmdBuffers(QUEUE::GRAPHICS, data.secCmds.data(), vk::CommandBufferLevel::eSecondary,
                                                    commandCount_);
    }
}

void RenderPass::Base::createImageResources() {
    auto& ctx = handler().shell().context();

    if (hasTargetSampler()) {
        for (auto& pTexture : pTextures_) {
            // Force only one sampler, for now, and just assume its the target.
            assert(pTexture->samplers.size() == 1);
            auto& sampler = pTexture->samplers.back();
            if (!usesSwapchain()) {
                // I did not test or think this through. This might require postponing
                // texture creation to here like the swapchain dependent samplers above.
                assert(!usesMultiSample());
            }
            assert(pTexture->status == STATUS::READY);
            assert((sampler.imgCreateInfo.usage & vk::ImageUsageFlagBits::eColorAttachment));
        }
    } else {
        assert(pTextures_.empty());
    }

    if (usesMultiSample()) {
        assert(resources_.resolveAttachments.size() == 1 && "Make sure multi-sampling attachment reference was added");

        images_.push_back({});

        helpers::createImage(ctx.dev, ctx.memProps,
                             handler().commandHandler().getUniqueQueueFamilies(true, false, true, false),
                             pipelineData_.samples, format_, vk::ImageTiling::eOptimal,
                             vk::ImageUsageFlagBits::eTransientAttachment | vk::ImageUsageFlagBits::eColorAttachment,
                             vk::MemoryPropertyFlagBits::eDeviceLocal, extent_.width, extent_.height, 1, 1,
                             images_.back().image, images_.back().memory, ctx.pAllocator);

        vk::ImageSubresourceRange range = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1};
        helpers::createImageView(ctx.dev, images_.back().image, format_, vk::ImageViewType::e2D, range, images_.back().view,
                                 ctx.pAllocator);
    } else {
        assert(resources_.resolveAttachments.size() == 0 && "Make sure multi-sampling attachment reference was not added");
    }
}

void RenderPass::Base::createDepthResource() {
    auto& ctx = handler().shell().context();
    if (pipelineData_.usesDepth) {
        vk::ImageTiling tiling = helpers::getDepthStencilImageTiling(ctx.physicalDev, depthFormat_);
        helpers::createImage(ctx.dev, ctx.memProps,
                             handler().commandHandler().getUniqueQueueFamilies(true, false, true, false),
                             pipelineData_.samples, depthFormat_, tiling, vk::ImageUsageFlagBits::eDepthStencilAttachment,
                             vk::MemoryPropertyFlagBits::eDeviceLocal, extent_.width, extent_.height, 1, 1, depth_.image,
                             depth_.memory, ctx.pAllocator);

        vk::ImageSubresourceRange range = {helpers::getDepthStencilAspectMask(depthFormat_), 0, 1, 0, 1};
        helpers::createImageView(ctx.dev, depth_.image, depthFormat_, vk::ImageViewType::e2D, range, depth_.view,
                                 ctx.pAllocator);
    }
}

void RenderPass::Base::updateClearValues() {
    /* NO LONGER RELEVANT! (keeping because this was confusing when first encountered)
     *  Need to pad this here because:
     *      In beginRenderPass() the vk::RenderPassBeginInfo struct has a clearValueCount
     *      of 2 but there must be at least 3 entries in pClearValues array to account for the
     *      highest index attachment in renderPass 0x2f that uses vk::AttachmentLoadOp::eClear
     *      is 3. Note that the pClearValues array is indexed by attachment number so even if
     *      some pClearValues entries between 0 and 2 correspond to attachments that aren't
     *      cleared they will be ignored. The spec valid usage text states 'clearValueCount
     *      must be greater than the largest attachment index in renderPass that specifies a
     *      loadOp (or stencilLoadOp, if the attachment has a depth/stencil format) of
     *      vk::AttachmentLoadOp::eClear'
     *      (https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#VUID-VkRenderPassBeginInfo-clearValueCount-00902)
     */
    clearValues_.clear();
    // DEPTH ATTACHMENT
    if (pipelineData_.usesDepth) {
        clearValues_.push_back({});
        clearValues_.back().depthStencil = DEFAULT_CLEAR_DEPTH_STENCIL_VALUE;
    }
    // MULTI-SAMPLE ATTACHMENT
    if (usesMultiSample()) {
        clearValues_.push_back({});
        clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
    }
    // TARGET ATTACHMENT
    clearValues_.push_back({});
    clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
}

void RenderPass::Base::createFramebuffers() {
    /* These are the potential views for framebuffer.
     *  - depth
     *  - mulit-sample
     *  - target
     *      - swapchain
     *      - sampler
     *
     * TODO: Should it always be imageCount based?
     */
    std::vector<std::vector<vk::ImageView>> attachmentViewsList(handler().shell().context().imageCount);
    data.framebuffers.resize(attachmentViewsList.size());

    vk::FramebufferCreateInfo createInfo = {};
    createInfo.renderPass = pass;
    // createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
    // createInfo.pAttachments = attachmentViews.data();
    createInfo.width = extent_.width;
    createInfo.height = extent_.height;
    createInfo.layers = 1;

    for (uint8_t frameIndex = 0; frameIndex < attachmentViewsList.size(); frameIndex++) {
        auto& attachmentViews = attachmentViewsList[frameIndex];
        // DEPTH
        if (pipelineData_.usesDepth) {
            assert(depth_.view);
            attachmentViews.push_back(depth_.view);  // TODO: should there be one per swapchain image????????????????
        }
        // MULTI-SAMPLE
        if (usesMultiSample()) {
            assert(images_.size() == 1);
            assert(images_[0].view);
            attachmentViews.push_back(images_[0].view);  // TODO: should there be one per swapchain image????????????????
        }
        // TARGET
        if (hasTargetSwapchain()) {
            // SWAPCHAIN
            attachmentViews.push_back(handler().renderPassMgr().getSwapchainViews()[frameIndex]);
        } else {
            // SAMPLER
            auto texIndex = (std::min)(static_cast<uint8_t>(pTextures_.size() - 1), frameIndex);
            const auto& imageView =
                pTextures_[texIndex]->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view;
            assert(imageView);
            attachmentViews.push_back(imageView);
        }

        createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        createInfo.pAttachments = attachmentViews.data();

        data.framebuffers[frameIndex] =
            handler().shell().context().dev.createFramebuffer(createInfo, handler().shell().context().pAllocator);
    }
}

void RenderPass::Base::createViewports() {
    // VIEWPORT
    viewports_.clear();
    viewports_.emplace_back();
    viewports_.back().x = 0.0f;
    viewports_.back().y = 0.0f;
    viewports_.back().width = static_cast<float>(extent_.width);
    viewports_.back().height = static_cast<float>(extent_.height);
    viewports_.back().minDepth = 0.0f;
    viewports_.back().maxDepth = 1.0f;
    // SCISSOR
    scissors_.clear();
    scissors_.emplace_back();
    scissors_.back().offset = vk::Offset2D{0, 0};
    scissors_.back().extent = extent_;
}

void RenderPass::Base::updateBeginInfo() {
    beginInfo_.clearValueCount = static_cast<uint32_t>(clearValues_.size());
    beginInfo_.pClearValues = clearValues_.data();
    beginInfo_.renderArea.offset = vk::Offset2D{0, 0};
    beginInfo_.renderArea.extent = extent_;
}

void RenderPass::Base::destroyTargetResources() {
    auto& ctx = handler().shell().context();

    // COLOR
    for (auto& color : images_) helpers::destroyImageResource(ctx.dev, color, ctx.pAllocator);
    images_.clear();

    // DEPTH
    helpers::destroyImageResource(ctx.dev, depth_, ctx.pAllocator);

    // FRAMEBUFFER
    for (auto& framebuffer : data.framebuffers) ctx.dev.destroyFramebuffer(framebuffer, ctx.pAllocator);
    data.framebuffers.clear();

    // EXTENT
    extent_ = BAD_EXTENT_2D;

    // COMMAND
    helpers::destroyCommandBuffers(ctx.dev, handler().commandHandler().graphicsCmdPool(), data.priCmds);
    helpers::destroyCommandBuffers(ctx.dev, handler().commandHandler().graphicsCmdPool(), data.secCmds);

    // SEMAPHORE
    for (auto& semaphore : data.semaphores) ctx.dev.destroySemaphore(semaphore, ctx.pAllocator);
    data.semaphores.clear();
}

void RenderPass::Base::beginSecondary(const uint8_t frameIndex) {
    if (secCmdFlag_) return;
    // FRAME UPDATE
    auto& secCmd = data.secCmds[frameIndex];
    inheritInfo_.framebuffer = data.framebuffers[frameIndex];
    // COMMAND
    secCmd.reset({});
    secCmd.begin(secCmdBeginInfo_);
    // FRAME COMMANDS
    secCmd.setScissor(0, scissors_);
    secCmd.setViewport(0, viewports_);

    secCmdFlag_ = true;
}

void RenderPass::Base::endSecondary(const uint8_t frameIndex, const vk::CommandBuffer& priCmd) {
    if (!secCmdFlag_) return;
    // EXECUTE SECONDARY
    auto& secCmd = data.secCmds[frameIndex];
    secCmd.end();
    priCmd.executeCommands({secCmd});

    secCmdFlag_ = false;
}

void RenderPass::Base::updateSubmitResource(SubmitResource& resource, const uint8_t frameIndex) const {
    std::memcpy(                                                //
        &resource.commandBuffers[resource.commandBufferCount],  //
        &data.priCmds[frameIndex], sizeof(vk::CommandBuffer)    //
    );
    resource.commandBufferCount++;
}

uint32_t RenderPass::Base::getSubpassId(const PIPELINE& type) const { return pipelineBindDataList_.getOffset(type); }

void RenderPass::Base::setSubpassOffsets(const std::vector<std::unique_ptr<Base>>& pPasses) {
    for (auto& [passType, offset] : dependentTypeOffsetPairs_) {
        bool found = false;
        for (const auto& pPass : pPasses) {
            if (pPass->TYPE == passType) {
                offset = pPass->OFFSET;
                found = true;
            }
        }
        assert(found);
    }
}

void RenderPass::Base::setBindData(const PIPELINE& pipelineType, const std::shared_ptr<Pipeline::BindData>& pBindData) {
    pipelineBindDataList_.insert(pipelineType, pBindData);
}

void RenderPass::Base::destroy() {
    // render pass
    if (pass) handler().shell().context().dev.destroyRenderPass(pass, handler().shell().context().pAllocator);
}

void RenderPass::Base::createSemaphores() {
    vk::SemaphoreCreateInfo createInfo = {};
    data.semaphores.resize(semaphoreCount_);
    for (uint32_t i = 0; i < semaphoreCount_; i++) {
        data.semaphores[i] =
            handler().shell().context().dev.createSemaphore(createInfo, handler().shell().context().pAllocator);
    }
}

void RenderPass::Base::createAttachmentDebugMarkers() {
    // auto& ctx = handler().shell().context();
    if (handler().shell().context().debugMarkersEnabled) {
        std::string markerName;
        uint32_t count = 0;
        for (auto& color : images_) {
            if (color.image) {
                markerName = NAME + " color framebuffer image (" + std::to_string(count++) + ")";
                // ctx.dbg.setMarkerName(color.image, markerName.c_str());
            }
        }
        if (depth_.image) {
            markerName = NAME + " depth framebuffer image";
            // ctx.dbg.setMarkerName(depth_.image, markerName.c_str());
        }
    }
}
