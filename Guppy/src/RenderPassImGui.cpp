/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "RenderPassImGui.h"

#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <utility>

#include <Common/Helpers.h>

#include "ConstantsAll.h"
#include "RenderPassManager.h"
// HANDLERS
#include "CommandHandler.h"
#include "DescriptorHandler.h"
#include "PipelineHandler.h"
#include "PassHandler.h"
#include "UIHandler.h"

namespace {
const RenderPass::CreateInfo CREATE_INFO = {RENDER_PASS::IMGUI, "ImGui", {}, RenderPass::FLAG::SWAPCHAIN};
}  // namespace

RenderPass::ImGui::ImGui(Pass::Handler& handler, const uint32_t&& offset)
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
    init_info.CheckVkResultFn = &helpers::checkVkResult;

    ImGui_ImplVulkan_Init(&init_info, pass);

    // FONTS
    ImGui_ImplVulkan_CreateFontsTexture(handler().commandHandler().graphicsCmd());
    handler().commandHandler().graphicsCmd().end();

    vk::SubmitInfo end_info = {};
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &handler().commandHandler().graphicsCmd();
    helpers::checkVkResult(ctx.queues.at(ctx.graphicsIndex).submit(1, &end_info, {}));

    ctx.dev.waitIdle();

    // RESET GRAPHICS DEFAULT COMMAND
    handler().commandHandler().graphicsCmd().reset({});
    handler().commandHandler().beginCmd(handler().commandHandler().graphicsCmd());

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void RenderPass::ImGui::record(const uint8_t frameIndex) {
    beginInfo_.framebuffer = data.framebuffers[frameIndex];
    auto& priCmd = data.priCmds[frameIndex];

    // RESET BUFFERS
    priCmd.reset({});

    // BEGIN BUFFERS
    vk::CommandBufferBeginInfo bufferInfo = {vk::CommandBufferUsageFlagBits::eSimultaneousUse};
    priCmd.begin(bufferInfo);

    beginPass(priCmd, frameIndex);
    handler().uiHandler().draw(data.priCmds[frameIndex], frameIndex);
    endPass(priCmd);
    // data.priCmds[frameIndex].end();
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

    bool isClear = handler().renderPassMgr().isClearTargetPass(getTargetId(), TYPE);

    vk::AttachmentDescription attachment = {};
    attachment.format = getFormat();
    attachment.samples = vk::SampleCountFlagBits::e1;
    attachment.loadOp = isClear ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
    attachment.storeOp = vk::AttachmentStoreOp::eDontCare;
    attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachment.initialLayout = isClear ? vk::ImageLayout::eUndefined : getInitialLayout();
    attachment.finalLayout = getFinalLayout();

    resources_.attachments.push_back(attachment);

    resources_.colorAttachments.resize(1);
    resources_.colorAttachments.back().attachment = 0;
    resources_.colorAttachments.back().layout = vk::ImageLayout::eColorAttachmentOptimal;
}

void RenderPass::ImGui::createSubpassDescriptions() {
    vk::SubpassDescription subpass = {};
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
    vk::SubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = {};
    dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

    resources_.dependencies.push_back(dependency);
}

void RenderPass::ImGui::updateClearValues() {
    clearValues_.clear();
    // SWAPCHAIN ATTACHMENT
    clearValues_.push_back({});
    clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
}
