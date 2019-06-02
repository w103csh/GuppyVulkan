
#include "RenderPass.h"

#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "RenderPassHandler.h"
#include "SceneHandler.h"

RenderPass::Base::Base(RenderPass::Handler& handler, const uint32_t&& offset, const RenderPass::CreateInfo* pCreateInfo)
    : Handlee(handler),
      NAME(pCreateInfo->name),
      OFFSET(offset),
      SWAPCHAIN_DEPENDENT(pCreateInfo->swapchainDependent),
      TYPE(pCreateInfo->type),
      pass(VK_NULL_HANDLE),
      data{},
      scissor_{},
      viewport_{},
      beginInfo_{},
      includeDepth_(pCreateInfo->includeDepth),
      format_(VK_FORMAT_UNDEFINED),
      depthFormat_(VK_FORMAT_UNDEFINED),
      finalLayout_(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR),
      samples_(VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM),
      commandCount_(0),
      semaphoreCount_(0),
      extent_{0, 0},
      depth_{}  //
{
    for (const auto& pipelineType : pCreateInfo->pipelineTypes) {
        // If changed from set then a lot of work needs to be done, so I am
        // adding some validation here.
        assert(pipelineTypeReferenceMap_.count(pipelineType) == 0);
        pipelineTypeReferenceMap_.insert({pipelineType, {}});
    }
}

void RenderPass::Base::create() {
    createAttachmentsAndSubpasses();
    createDependencies();

    createPass();
    assert(pass != VK_NULL_HANDLE);

    createBeginInfo();

    if (handler().settings().enable_debug_markers) {
        std::string markerName = NAME + " render pass";
        ext::DebugMarkerSetObjectName(handler().shell().context().dev, (uint64_t)pass,
                                      VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, markerName.c_str());
    }

    // Not sure if this is the right place for validation since the
    // base doesn't use these.
    assert(format_ != VK_FORMAT_UNDEFINED);
    if (includeDepth_)  //
        assert(depthFormat_ != VK_FORMAT_UNDEFINED);
}

void RenderPass::Base::createTarget() {
    // FRAME
    createColorResources();
    createDepthResource();
    createAttachmentDebugMarkers();
    updateClearValues();
    createFramebuffers();

    // RENDER PASS
    createCommandBuffers();
    createViewport();
    updateBeginInfo();

    // SYNC
    createSemaphores();

    // Validate signal semaphore creation. If semaphores are created
    // the mask should be set.
    if (data.semaphores.empty())
        assert(data.signalSrcStageMask == VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM);
    else
        assert(data.signalSrcStageMask != VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM);

    assert(data.priCmds.size() + data.secCmds.size() <= RESOURCE_SIZE);
    assert(data.semaphores.size() <= RESOURCE_SIZE);
}

void RenderPass::Base::overridePipelineCreateInfo(const PIPELINE& type, Pipeline::CreateInfoResources& createInfoRes) {
    createInfoRes.depthStencilStateInfo.depthTestEnable = includeDepth_;
    createInfoRes.depthStencilStateInfo.depthWriteEnable = includeDepth_;
    assert(samples_ != VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM);
    createInfoRes.multisampleStateInfo.rasterizationSamples = samples_;
}

void RenderPass::Base::record(const uint32_t& frameIndex) {
    beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    // pPass->beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    // VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

    auto& priCmd = data.priCmds[frameIndex];
    auto& secCmd = data.secCmds[frameIndex];
    auto& pScene = handler().sceneHandler().getActiveScene();

    auto it = pipelineTypeReferenceMap_.begin();
    while (it != pipelineTypeReferenceMap_.end()) {
        // Record the scene
        pScene->record((*it).first, pipelineTypeReferenceMap_.at((*it).first), priCmd, secCmd, frameIndex);

        std::advance(it, 1);
        if (it != pipelineTypeReferenceMap_.end()) {
            vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
            // vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        }
    }

    endPass(frameIndex);
}

void RenderPass::Base::beginPass(const uint8_t& frameIndex, VkCommandBufferUsageFlags&& primaryCommandUsage,
                                 VkSubpassContents&& subpassContents) {
    // FRAME UPDATE
    beginInfo_.framebuffer = data.framebuffers[frameIndex];
    auto& priCmd = data.priCmds[frameIndex];

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

void RenderPass::Base::createPass() {
    VkRenderPassCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.pNext = nullptr;
    // ATTACHMENTS
    createInfo.attachmentCount = static_cast<uint32_t>(resources_.attachments.size());
    createInfo.pAttachments = resources_.attachments.data();
    // SUBPASSES
    createInfo.subpassCount = static_cast<uint32_t>(resources_.subpasses.size());
    createInfo.pSubpasses = resources_.subpasses.data();
    // DEPENDENCIES
    createInfo.dependencyCount = static_cast<uint32_t>(resources_.dependencies.size());
    createInfo.pDependencies = resources_.dependencies.data();

    vk::assert_success(vkCreateRenderPass(handler().shell().context().dev, &createInfo, nullptr, &pass));
}

void RenderPass::Base::createBeginInfo() {
    beginInfo_ = {};
    beginInfo_.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo_.renderPass = pass;
}

void RenderPass::Base::createCommandBuffers() {
    data.priCmds.resize(commandCount_);
    handler().commandHandler().createCmdBuffers(handler().commandHandler().graphicsCmdPool(), data.priCmds.data(),
                                                VK_COMMAND_BUFFER_LEVEL_PRIMARY, handler().shell().context().imageCount);
}

void RenderPass::Base::createViewport() {
    // VIEWPORT
    viewport_.x = 0.0f;
    viewport_.y = 0.0f;
    viewport_.width = static_cast<float>(extent_.width);
    viewport_.height = static_cast<float>(extent_.height);
    viewport_.minDepth = 0.0f;
    viewport_.maxDepth = 1.0f;
    // SCISSOR
    scissor_.offset = {0, 0};
    scissor_.extent = extent_;
}

void RenderPass::Base::updateBeginInfo() {
    beginInfo_.clearValueCount = static_cast<uint32_t>(clearValues_.size());
    beginInfo_.pClearValues = clearValues_.data();
    beginInfo_.renderArea.offset = {0, 0};
    beginInfo_.renderArea.extent = extent_;
}

void RenderPass::Base::destroyTargetResources() {
    auto& dev = handler().shell().context().dev;
    // COLOR
    for (auto& color : images_) helpers::destroyImageResource(dev, color);
    images_.clear();
    // DEPTH
    helpers::destroyImageResource(dev, depth_);
    // FRAMEBUFFER
    for (auto& framebuffer : data.framebuffers) vkDestroyFramebuffer(dev, framebuffer, nullptr);
    data.framebuffers.clear();
    // COMMAND
    helpers::destroyCommandBuffers(dev, handler().commandHandler().graphicsCmdPool(), data.priCmds);
    helpers::destroyCommandBuffers(dev, handler().commandHandler().graphicsCmdPool(), data.secCmds);
    // SEMAPHORE
    for (auto& semaphore : data.semaphores) vkDestroySemaphore(dev, semaphore, nullptr);
    data.semaphores.clear();
}

void RenderPass::Base::updateSubmitResource(SubmitResource& resource, const uint32_t& frameIndex) const {
    if (data.signalSrcStageMask != VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM) {
        std::memcpy(                                                    //
            &resource.signalSemaphores[resource.signalSemaphoreCount],  //
            &data.semaphores[frameIndex], sizeof(VkSemaphore)           //
        );
        std::memcpy(                                                       //
            &resource.signalSrcStageMasks[resource.signalSemaphoreCount],  //
            &data.signalSrcStageMask, sizeof(VkPipelineStageFlags)         //
        );
        resource.signalSemaphoreCount++;
    }
    std::memcpy(                                                //
        &resource.commandBuffers[resource.commandBufferCount],  //
        &data.priCmds[frameIndex], sizeof(VkCommandBuffer)      //
    );
    resource.commandBufferCount++;
}

uint32_t RenderPass::Base::getSubpassId(const PIPELINE& type) const {
    uint32_t id = 0;
    for (const auto keyValue : pipelineTypeReferenceMap_) {
        if (keyValue.first == type) break;
        id++;
    }
    return id;  // TODO: is returning 0 for no types okay? Try using std::optional maybe?
};

void RenderPass::Base::setPipelineReference(const PIPELINE& pipelineType, const Pipeline::Reference& reference) {
    pipelineTypeReferenceMap_.at(pipelineType) = reference;
}

void RenderPass::Base::destroy() {
    // render pass
    if (pass != VK_NULL_HANDLE) vkDestroyRenderPass(handler().shell().context().dev, pass, nullptr);
}

void RenderPass::Base::createSemaphores() {
    VkSemaphoreCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    data.semaphores.resize(semaphoreCount_);
    for (uint32_t i = 0; i < semaphoreCount_; i++) {
        vk::assert_success(vkCreateSemaphore(handler().shell().context().dev, &createInfo, nullptr, &data.semaphores[i]));
    }
}

void RenderPass::Base::createAttachmentDebugMarkers() {
    auto& ctx = handler().shell().context();
    if (handler().settings().enable_debug_markers) {
        std::string markerName;
        uint32_t count = 0;
        for (auto& color : images_) {
            if (color.image != VK_NULL_HANDLE) {
                markerName = NAME + " color framebuffer image (" + std::to_string(count++) + ")";
                ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)color.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                              markerName.c_str());
            }
        }
        if (depth_.image != VK_NULL_HANDLE) {
            markerName = NAME + " depth framebuffer image";
            ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)depth_.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                          markerName.c_str());
        }
    }
}
