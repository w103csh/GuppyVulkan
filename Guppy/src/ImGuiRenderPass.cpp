
#include "ImGuiRenderPass.h"

#include "CmdBufHandler.h"
#include "Helpers.h"

void ImGuiRenderPass::getSubmitResource(const uint8_t& frameIndex, SubmitResource& resource) {
    resource.waitDstStageMasks.push_back({VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT});
    resource.commandBuffers.push_back(data.priCmds[frameIndex]);
}

void ImGuiRenderPass::createAttachmentsAndSubpasses(const Shell::Context& ctx, const Game::Settings& settings) {
    auto& resources = subpassResources_;
    /*
        THESE SETTINGS ARE COPIED DIRECTLY FROM THE IMGUI VULKAN SAMPLE.
        IF IMGUI IS UPDATED THIS SHOULD BE REPLACED WITH THE NEW SETTINGS, OR
        HOPEFULLY TURNED INTO A MORE MODULAR SETUP CALLING THEIR FUNCTIONS!

        The ImGui implementation currently creates a swapchain along with a
        render pass inside their setup. We only want the render pass & attachments,
        so unfortunately their code cannot be used as is.
    */
    VkAttachmentDescription attachment = {};
    attachment.format = initInfo.format;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = initInfo.clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = initInfo.finalLayout;

    subpassResources_.attachments.push_back(attachment);

    subpassResources_.colorAttachments.resize(1);
    subpassResources_.colorAttachments.back().attachment = 0;
    subpassResources_.colorAttachments.back().layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = static_cast<uint32_t>(subpassResources_.colorAttachments.size());
    subpass.pColorAttachments = &subpassResources_.colorAttachments.back();

    resources.subpasses.push_back(subpass);
}

void ImGuiRenderPass::createDependencies(const Shell::Context& ctx, const Game::Settings& settings) {
    auto& resources = subpassResources_;
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

    resources.dependencies.push_back(dependency);
}

void ImGuiRenderPass::createFramebuffers(const Shell::Context& ctx, const Game::Settings& settings) {
    /*  These are the views for framebuffer.

        view[0] is swapchain

        The incoming "views" param are the swapchain views.
    */
    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = pass;
    createInfo.attachmentCount = 1;
    createInfo.width = frameInfo.extent.width;
    createInfo.height = frameInfo.extent.height;
    createInfo.layers = 1;

    data.framebuffers.resize(frameInfo.viewCount);
    for (uint8_t i = 0; i < frameInfo.viewCount; i++) {
        createInfo.pAttachments = &frameInfo.pViews[i];
        vk::assert_success(vkCreateFramebuffer(ctx.dev, &createInfo, nullptr, &data.framebuffers[i]));
        data.tests.push_back(1);
    }
}