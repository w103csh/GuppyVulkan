
#include "RenderPass.h"

#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "RenderPassHandler.h"

RenderPass::Base::Base(RenderPass::Handler& handler, const std::string&& name, const RenderPass::CreateInfo createInfo)
    : Handlee(handler),
      NAME(name),
      PIPELINE_TYPES(createInfo.types),
      CLEAR_COLOR(createInfo.clearColor),
      CLEAR_DEPTH(createInfo.clearDepth),
      pass(VK_NULL_HANDLE),
      data{},
      scissor_{},
      viewport_{},
      beginInfo_{},
      depthFormat_{},
      finalLayout_(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR),
      samples_(VK_SAMPLE_COUNT_1_BIT),
      commandCount_(0),
      fenceCount_(0),
      semaphoreCount_(0),
      waitDstStageMask_(VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT),
      extent_{0, 0},
      viewCount_(0),
      pViews_(nullptr),
      depth_{}  //
{}

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
}

void RenderPass::Base::createTarget() {
    // FRAME
    createColorResources();
    createDepthResource();
    createAttachmentDebugMarkers();
    createClearValues();
    createFramebuffers();

    // RENDER PASS
    createCommandBuffers();
    createViewport();
    updateBeginInfo();

    // SYNC
    createFences();
    createSemaphores();
}

void RenderPass::Base::beginPass(const uint8_t& frameIndex, VkCommandBufferUsageFlags&& primaryCommandUsage,
                                 VkSubpassContents&& subpassContents) {
    if (data.tests[frameIndex]) data.tests[frameIndex] = 0;

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
    createInfo.attachmentCount = static_cast<uint32_t>(subpassResources_.attachments.size());
    createInfo.pAttachments = subpassResources_.attachments.data();
    // SUBPASSES
    createInfo.subpassCount = static_cast<uint32_t>(subpassResources_.subpasses.size());
    createInfo.pSubpasses = subpassResources_.subpasses.data();
    // DEPENDENCIES
    createInfo.dependencyCount = static_cast<uint32_t>(subpassResources_.dependencies.size());
    createInfo.pDependencies = subpassResources_.dependencies.data();

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

void RenderPass::Base::createFences(VkFenceCreateFlags flags) {
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = flags;
    data.fences.resize(fenceCount_);
    for (auto& fence : data.fences)
        vk::assert_success(vkCreateFence(handler().shell().context().dev, &fenceInfo, nullptr, &fence));
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
    for (auto& color : colors_) helpers::destroyImageResource(dev, color);
    colors_.clear();
    // DEPTH
    helpers::destroyImageResource(dev, depth_);
    // FRAMEBUFFER
    for (auto& framebuffer : data.framebuffers) vkDestroyFramebuffer(dev, framebuffer, nullptr);
    data.framebuffers.clear();
    // COMMAND
    helpers::destroyCommandBuffers(dev, handler().commandHandler().graphicsCmdPool(), data.priCmds);
    helpers::destroyCommandBuffers(dev, handler().commandHandler().graphicsCmdPool(), data.secCmds);
    // FENCE
    for (auto& fence : data.fences) vkDestroyFence(dev, fence, nullptr);
    data.fences.clear();
    // SEMAPHORE
    for (auto& semaphore : data.semaphores) vkDestroySemaphore(dev, semaphore, nullptr);
    data.semaphores.clear();
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
        for (auto& color : colors_) {
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
