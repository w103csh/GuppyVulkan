
#include "RenderPassImGui.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <utility>

#include "Constants.h"
#include "Helpers.h"
// HANDLERS
#include "CommandHandler.h"
#include "DescriptorHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"
#include "UIHandler.h"

namespace {
const RenderPass::CreateInfo CREATE_INFO = {
    RENDER_PASS::IMGUI, "ImGui", {}, true, false,
};
}

RenderPass::ImGui::ImGui(RenderPass::Handler& handler, const uint32_t&& offset)
    : RenderPass::Base{handler, std::forward<const uint32_t>(offset), &CREATE_INFO} {}

void RenderPass::ImGui::init() {
    auto& ctx = handler().shell().context();
    // FRAME DATA
    format_ = ctx.surfaceFormat.format;
    // SYNC
    commandCount_ = ctx.imageCount;
}

void RenderPass::ImGui::setSwapchainInfo() {
    extent_ = handler().shell().context().extent;  //
}

void RenderPass::ImGui::postCreate() {
    // Let the ImGui code create its own pipeline...

    auto& ctx = handler().shell().context();

    // VULKAN INIT
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = ctx.instance;
    init_info.PhysicalDevice = ctx.physical_dev;
    init_info.Device = ctx.dev;
    init_info.QueueFamily = ctx.graphics_index;
    init_info.Queue = ctx.queues.at(ctx.graphics_index);
    init_info.PipelineCache = handler().pipelineHandler().getPipelineCache();
    init_info.DescriptorPool = handler().descriptorHandler().getPool();
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = (void (*)(VkResult))vk::assert_success;

    ImGui_ImplVulkan_Init(&init_info, pass);

    // FONTS
    ImGui_ImplVulkan_CreateFontsTexture(handler().commandHandler().graphicsCmd());
    vkEndCommandBuffer(handler().commandHandler().graphicsCmd());

    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &handler().commandHandler().graphicsCmd();
    vk::assert_success(vkQueueSubmit(ctx.queues.at(ctx.graphics_index), 1, &end_info, VK_NULL_HANDLE));

    vk::assert_success(vkDeviceWaitIdle(ctx.dev));

    // RESET GRAPHICS DEFAULT COMMAND
    vk::assert_success(vkResetCommandBuffer(handler().commandHandler().graphicsCmd(), 0));
    handler().commandHandler().beginCmd(handler().commandHandler().graphicsCmd());

    ImGui_ImplVulkan_InvalidateFontUploadObjects();
}

void RenderPass::ImGui::record(const uint32_t& frameIndex) {
    beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);
    handler().uiHandler().draw(data.priCmds[frameIndex], frameIndex);
    endPass(frameIndex);
}

void RenderPass::ImGui::createAttachmentsAndSubpasses() {
    bool clearAttachment = !handler().getSwapchainCleared();
    /*
        THESE SETTINGS ARE COPIED DIRECTLY FROM THE IMGUI VULKAN SAMPLE.
        IF IMGUI IS UPDATED THIS SHOULD BE REPLACED WITH THE NEW SETTINGS, OR
        HOPEFULLY TURNED INTO A MORE MODULAR SETUP CALLING THEIR FUNCTIONS!

        The ImGui implementation currently creates a swapchain along with a
        render pass inside their setup. We only want the render pass & attachments,
        so unfortunately their code cannot be used as is.
    */

    VkAttachmentDescription attachment = {};
    attachment.format = format_;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = clearAttachment ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = finalLayout_;

    resources_.attachments.push_back(attachment);

    resources_.colorAttachments.resize(1);
    resources_.colorAttachments.back().attachment = 0;
    resources_.colorAttachments.back().layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(resources_.colorAttachments.size());
    subpass.pColorAttachments = &resources_.colorAttachments.back();

    resources_.subpasses.push_back(subpass);
}

void RenderPass::ImGui::createDependencies() {
    /*
        SEE COMMENT IN createSubpassesAndAttachments !!!!!
    */
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    resources_.dependencies.push_back(dependency);
}

void RenderPass::ImGui::updateClearValues() {
    clearValues_.clear();
    // SWAPCHAIN ATTACHMENT
    clearValues_.push_back({});
    clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
}

void RenderPass::ImGui::createFramebuffers() {
    /* These are the potential views for framebuffer.
     *  - swapchain
     */
    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = pass;
    createInfo.attachmentCount = 1;
    createInfo.width = extent_.width;
    createInfo.height = extent_.height;
    createInfo.layers = 1;

    data.framebuffers.resize(handler().getSwapchainImageCount());
    for (auto i = 0; i < data.framebuffers.size(); i++) {
        createInfo.pAttachments = &handler().getSwapchainViews()[i];
        vk::assert_success(
            vkCreateFramebuffer(handler().shell().context().dev, &createInfo, nullptr, &data.framebuffers[i]));
    }
}