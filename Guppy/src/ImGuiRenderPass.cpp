
#include "ImGuiRenderPass.h"

#include "CmdBufHandler.h"
#include "Helpers.h"

// void ImGuiRenderPass::createAttachments(const Shell::Context& ctx, const Game::Settings& settings, bool clear,
//                                        VkImageLayout finalLayout) {
//    /*
//        THESE SETTINGS ARE COPIED DIRECTLY FROM THE IMGUI VULKAN SAMPLE.
//        IF IMGUI IS UPDATED THIS SHOULD BE REPLACED WITH THE NEW SETTINGS, OR
//        HOPEFULLY TURNED INTO A MORE MODULAR SETUP CALLING THEIR FUNCTIONS!
//
//        The ImGui implementation currently creates a swapchain along with a
//        render pass inside their setup. We only want the render pass & attachments,
//        so unfortunately their code cannot be used as is.
//    */
//    VkAttachmentDescription attachment = {};
//    attachment.format = ctx.surface_format.format;
//    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
//    attachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//
//    VkAttachmentReference color_attachment = {};
//    color_attachment.attachment = 0;
//    color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//    colorAttachments.push_back(color_attachment);
//}

void ImGuiRenderPass::createDependencies(const Shell::Context& ctx, const Game::Settings& settings) {
    /*
        SEE COMMENT IN createAttachments !!!!!
    */
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    // dependencies_.push_back(dependency);
}

void ImGuiRenderPass::getSubpassDescriptions(std::vector<VkSubpassDescription>& subpasses) const {
    /*
        SEE COMMENT IN createAttachments !!!!!
    */
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // subpass.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
    // subpass.pColorAttachments = colorAttachments.data();
}

void ImGuiRenderPass::createImageViews(const Shell::Context& ctx) {
    /*
        SEE COMMENT IN createAttachments !!!!!
    */
    VkImageViewCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    info.format = ctx.surfaceFormat.format;
    info.components.r = VK_COMPONENT_SWIZZLE_R;
    info.components.g = VK_COMPONENT_SWIZZLE_G;
    info.components.b = VK_COMPONENT_SWIZZLE_B;
    info.components.a = VK_COMPONENT_SWIZZLE_A;
    VkImageSubresourceRange image_range = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    info.subresourceRange = image_range;
    // for (auto& data : frameData_) {
    //    info.image = data.colorResource.image;
    //    vk::assert_success(vkCreateImageView(ctx.dev, &info, nullptr, &data.colorResource.view));
    //}
}

// void ImGuiRenderPass::createFramebuffers(const Shell::Context& ctx) {
//    /*
//        SEE COMMENT IN createAttachments !!!!!
//    */
//    VkFramebufferCreateInfo info = {};
//    info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//    info.renderPass = pass;
//    info.attachmentCount = 1;
//    info.width = ctx.extent.width;
//    info.height = ctx.extent.height;
//    info.layers = 1;
//    // for (uint32_t i = 0; i < ctx.image_count; i++) {
//    //    info.pAttachments = &frameData_[i].colorResource.view;
//    //    vk::assert_success(vkCreateFramebuffer(ctx.dev, &info, nullptr, &frameData_[i].framebuffer));
//    //}
//}
