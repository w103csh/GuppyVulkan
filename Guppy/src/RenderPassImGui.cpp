
#include "RenderPassImGui.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <utility>

#include "ConstantsAll.h"
#include "Helpers.h"
// HANDLERS
#include "CommandHandler.h"
#include "DescriptorHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"
#include "UIHandler.h"

namespace {
const RenderPass::CreateInfo CREATE_INFO = {PASS::IMGUI, "ImGui", {}, RenderPass::FLAG::SWAPCHAIN};
}  // namespace

RenderPass::ImGui::ImGui(RenderPass::Handler& handler, const uint32_t&& offset)
    : RenderPass::Base{handler, std::forward<const uint32_t>(offset), &CREATE_INFO} {}

void RenderPass::ImGui::postCreate() {
    // Let the ImGui code create its own pipeline...

    auto& ctx = handler().shell().context();

    // VULKAN INIT
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = ctx.instance;
    init_info.PhysicalDevice = ctx.physicalDev;
    init_info.Device = ctx.dev;
    init_info.QueueFamily = ctx.graphicsIndex;
    init_info.Queue = ctx.queues.at(ctx.graphicsIndex);
    init_info.PipelineCache = handler().pipelineHandler().getPipelineCache();
    init_info.DescriptorPool = handler().descriptorHandler().getPool();
    init_info.MinImageCount = handler().shell().context().imageCount;  // use the minImageCount?
    init_info.ImageCount = handler().shell().context().imageCount;
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
    vk::assert_success(vkQueueSubmit(ctx.queues.at(ctx.graphicsIndex), 1, &end_info, VK_NULL_HANDLE));

    vk::assert_success(vkDeviceWaitIdle(ctx.dev));

    // RESET GRAPHICS DEFAULT COMMAND
    vk::assert_success(vkResetCommandBuffer(handler().commandHandler().graphicsCmd(), 0));
    handler().commandHandler().beginCmd(handler().commandHandler().graphicsCmd());

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void RenderPass::ImGui::record(const uint8_t frameIndex) {
    beginInfo_.framebuffer = data.framebuffers[frameIndex];
    auto& priCmd = data.priCmds[frameIndex];

    // RESET BUFFERS
    vkResetCommandBuffer(priCmd, 0);

    // BEGIN BUFFERS
    VkCommandBufferBeginInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vk::assert_success(vkBeginCommandBuffer(priCmd, &bufferInfo));

    beginPass(priCmd, frameIndex);
    handler().uiHandler().draw(data.priCmds[frameIndex], frameIndex);
    endPass(priCmd);
    // vk::assert_success(vkEndCommandBuffer(data.priCmds[frameIndex]));
}

void RenderPass::ImGui::createAttachments() {
    /*
        THESE SETTINGS ARE COPIED DIRECTLY FROM THE IMGUI VULKAN SAMPLE.
        IF IMGUI IS UPDATED THIS SHOULD BE REPLACED WITH THE NEW SETTINGS, OR
        HOPEFULLY TURNED INTO A MORE MODULAR SETUP CALLING THEIR FUNCTIONS!

        The ImGui implementation currently creates a swapchain along with a
        render pass inside their setup. We only want the render pass & attachments,
        so unfortunately their code cannot be used as is.
    */

    bool isClear = handler().isClearTargetPass(getTargetId(), TYPE);

    VkAttachmentDescription attachment = {};
    attachment.format = getFormat();
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = isClear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = isClear ? VK_IMAGE_LAYOUT_UNDEFINED : getInitialLayout();
    attachment.finalLayout = getFinalLayout();

    resources_.attachments.push_back(attachment);

    resources_.colorAttachments.resize(1);
    resources_.colorAttachments.back().attachment = 0;
    resources_.colorAttachments.back().layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
}

void RenderPass::ImGui::createSubpassDescriptions() {
    VkSubpassDescription subpass = {};
    subpass.colorAttachmentCount = static_cast<uint32_t>(resources_.colorAttachments.size());
    subpass.pColorAttachments = resources_.colorAttachments.data();
    subpass.pResolveAttachments = resources_.resolveAttachments.data();
    subpass.pDepthStencilAttachment = pipelineData_.usesDepth ? &resources_.depthStencilAttachment : nullptr;
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
