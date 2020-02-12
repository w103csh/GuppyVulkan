/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
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
    resources_.attachments.back().samples = usesMultiSample() ? vk::SampleCountFlagBits::e1 : getSamples();
    resources_.attachments.back().loadOp = vk::AttachmentLoadOp::eClear;
    resources_.attachments.back().storeOp = vk::AttachmentStoreOp::eStore;
    resources_.attachments.back().stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    resources_.attachments.back().stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    resources_.attachments.back().initialLayout = vk::ImageLayout::eUndefined;
    resources_.attachments.back().finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    resources_.attachments.back().flags = {};
    // REFERENCE
    if (usesMultiSample()) {
        // Set resolve to swapchain attachment
        resources_.resolveAttachments.push_back({
            static_cast<uint32_t>(resources_.attachments.size() - 1),
            vk::ImageLayout::eColorAttachmentOptimal,
        });
    } else {
        resources_.colorAttachments.push_back({
            static_cast<uint32_t>(resources_.attachments.size() - 1),
            vk::ImageLayout::eColorAttachmentOptimal,
        });
    }
}

void Base::createFramebuffers() {
    /* Views for framebuffer.
     *  - color
     */
    std::vector<std::vector<vk::ImageView>> attachmentViewsList(handler().shell().context().imageCount);
    data.framebuffers.resize(attachmentViewsList.size());

    vk::FramebufferCreateInfo createInfo = {};
    createInfo.renderPass = pass;
    createInfo.width = pTextures_[0]->samplers[0].imgCreateInfo.extent.width;
    createInfo.height = pTextures_[0]->samplers[0].imgCreateInfo.extent.height;
    createInfo.layers = pTextures_[0]->samplers[0].imgCreateInfo.arrayLayers;

    for (uint8_t frameIndex = 0; frameIndex < attachmentViewsList.size(); frameIndex++) {
        auto& attachmentViews = attachmentViewsList[frameIndex];

        assert(pTextures_[0]->samplers[0].layerResourceMap.size() == 1);
        attachmentViews.push_back(pTextures_[0]->samplers[0].layerResourceMap.begin()->second.view);
        assert(attachmentViews.back());

        createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        createInfo.pAttachments = attachmentViews.data();

        data.framebuffers[frameIndex] = handler().shell().context().dev.createFramebuffer(createInfo, ALLOC_PLACE_HOLDER);
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

void SkyboxNight::record(const uint8_t frameIndex, const vk::CommandBuffer& priCmd) {
    if (getStatus() != STATUS::READY) update();
    if (getStatus() == STATUS::READY) {
        beginPass(priCmd, frameIndex, vk::SubpassContents::eInline);

        auto& secCmd = data.secCmds[frameIndex];
        auto& pScene = handler().sceneHandler().getActiveScene();

        std::vector<PIPELINE>::iterator itSurrogate;

        // POINT
        if (true) {
            const auto& pStars = handler().meshHandler().getColorMesh(pScene->starsOffset);
            pStars->draw(TYPE, pipelineBindDataList_.getValue(pStars->PIPELINE_TYPE), priCmd, frameIndex);
        }

        priCmd.nextSubpass(vk::SubpassContents::eInline);

        // TEXTURE
        if (true) {
            const auto& pMoon = handler().meshHandler().getTextureMesh(pScene->moonOffset);
            pMoon->draw(TYPE, pipelineBindDataList_.getValue(pMoon->PIPELINE_TYPE), priCmd, frameIndex);
        }

        endPass(priCmd);
    }
}

}  // namespace CubeMap
}  // namespace RenderPass
