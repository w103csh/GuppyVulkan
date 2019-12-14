/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "RenderPassCubeMap.h"

// HANDLERS
#include "RenderPassHandler.h"
#include "MeshHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

namespace RenderPass {
namespace CubeMap {

Base::Base(RenderPass::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo)
    : RenderPass::Base(handler, std::forward<const index>(offset), pCreateInfo) {
    status_ = STATUS::PENDING_PIPELINE;
}

void Base::createAttachments() {
    // I am pretty sure you could get multi-sampling if you used the cube map image
    // as the resolve, but I'm not going to worry about it atm.
    assert(!usesMultiSample());

    // COLOR ATTACHMENT
    resources_.attachments.push_back({});
    resources_.attachments.back().format = pTextures_.at(0)->samplers.at(0).imgCreateInfo.format;
    resources_.attachments.back().samples = usesMultiSample() ? VK_SAMPLE_COUNT_1_BIT : getSamples();
    resources_.attachments.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    resources_.attachments.back().storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resources_.attachments.back().stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resources_.attachments.back().stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resources_.attachments.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resources_.attachments.back().finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    resources_.attachments.back().flags = 0;
    // REFERENCE
    if (usesMultiSample()) {
        // Set resolve to swapchain attachment
        resources_.resolveAttachments.push_back({
            static_cast<uint32_t>(resources_.attachments.size() - 1),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });
    } else {
        resources_.colorAttachments.push_back({
            static_cast<uint32_t>(resources_.attachments.size() - 1),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        });
    }
}

void Base::createFramebuffers() {
    /* Views for framebuffer.
     *  - color
     */
    std::vector<std::vector<VkImageView>> attachmentViewsList(handler().shell().context().imageCount);
    data.framebuffers.resize(attachmentViewsList.size());

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = pass;
    createInfo.width = pTextures_[0]->samplers[0].imgCreateInfo.extent.width;
    createInfo.height = pTextures_[0]->samplers[0].imgCreateInfo.extent.height;
    createInfo.layers = pTextures_[0]->samplers[0].imgCreateInfo.arrayLayers;

    for (uint8_t frameIndex = 0; frameIndex < attachmentViewsList.size(); frameIndex++) {
        auto& attachmentViews = attachmentViewsList[frameIndex];

        assert(pTextures_[0]->samplers[0].layerResourceMap.size() == 1);
        attachmentViews.push_back(pTextures_[0]->samplers[0].layerResourceMap.begin()->second.view);
        assert(attachmentViews.back() != VK_NULL_HANDLE);

        createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        createInfo.pAttachments = attachmentViews.data();

        vk::assert_success(
            vkCreateFramebuffer(handler().shell().context().dev, &createInfo, nullptr, &data.framebuffers[frameIndex]));
    }
}

// SKYBOX NIGHT
const CreateInfo SKYBOX_NIGHT_CREATE_INFO = {
    PASS::SKYBOX_NIGHT,
    "Skybox Night Render Pass",
    {
        GRAPHICS::CUBE_MAP_PT, GRAPHICS::CUBE_MAP_TEX,
        // GRAPHICS::CUBE_MAP_COLOR,
        // GRAPHICS::CUBE_MAP_LINE,
    },
    FLAG::NONE,
    {std::string(Texture::SKYBOX_NIGHT_ID)},
};
SkyboxNight::SkyboxNight(Handler& handler, const index&& offset)
    : Base(handler, std::forward<const index>(offset), &SKYBOX_NIGHT_CREATE_INFO) {}

void SkyboxNight::record(const uint8_t frameIndex, const VkCommandBuffer& priCmd) {
    if (getStatus() != STATUS::READY) update();
    if (getStatus() == STATUS::READY) {
        beginPass(priCmd, frameIndex, VK_SUBPASS_CONTENTS_INLINE);

        auto& secCmd = data.secCmds[frameIndex];
        auto& pScene = handler().sceneHandler().getActiveScene();

        std::vector<PIPELINE>::iterator itSurrogate;

        // POINT
        {
            const auto& pStars = handler().meshHandler().getColorMesh(pScene->starsOffset);
            pStars->draw(TYPE, pipelineBindDataList_.getValue(pStars->PIPELINE_TYPE), priCmd, frameIndex);
        }

        vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);

        // TEXTURE
        {
            const auto& pMoon = handler().meshHandler().getTextureMesh(pScene->moonOffset);
            pMoon->draw(TYPE, pipelineBindDataList_.getValue(pMoon->PIPELINE_TYPE), priCmd, frameIndex);
        }

        endPass(priCmd);
    }
}

}  // namespace CubeMap
}  // namespace RenderPass
