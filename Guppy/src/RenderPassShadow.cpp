/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "RenderPassShadow.h"

#include <array>

#include "Camera.h"
#include "ConstantsAll.h"
#include "Shadow.h"
// HANDLERS
#include "PipelineHandler.h"
#include "DescriptorHandler.h"
#include "ParticleHandler.h"
#include "PassHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"
#include "UniformHandler.h"

namespace RenderPass {
namespace Shadow {

Base::Base(Pass::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo)
    : RenderPass::Base{handler, std::forward<const index>(offset), pCreateInfo} {
    status_ = STATUS::PENDING_PIPELINE;
}

void Base::createAttachments() {
    resources_.depthStencilAttachment = {static_cast<uint32_t>(resources_.attachments.size()),
                                         vk::ImageLayout::eDepthStencilAttachmentOptimal};

    resources_.attachments.push_back({});
    resources_.attachments.back().format = pTextures_[0]->samplers[0].imgCreateInfo.format;
    resources_.attachments.back().samples = vk::SampleCountFlagBits::e1;
    resources_.attachments.back().loadOp = vk::AttachmentLoadOp::eClear;
    resources_.attachments.back().storeOp = vk::AttachmentStoreOp::eStore;
    resources_.attachments.back().stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    resources_.attachments.back().stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    resources_.attachments.back().initialLayout = vk::ImageLayout::eUndefined;
    resources_.attachments.back().finalLayout = vk::ImageLayout::eDepthStencilReadOnlyOptimal;
    resources_.attachments.back().flags = {};
}

void Base::createDependencies() {
    ::RenderPass::Base::createDependencies();
    if (pipelineBindDataList_.hasKey(GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER)) {
        resources_.dependencies.push_back({
            VK_SUBPASS_EXTERNAL,
            pipelineBindDataList_.getOffset(GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER),
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eVertexInput,
            vk::AccessFlagBits::eShaderWrite,
            vk::AccessFlagBits::eVertexAttributeRead,
        });
    }
}

void Base::createFramebuffers() {
    /* Views for framebuffer.
     *  - depth
     */
    std::vector<std::vector<vk::ImageView>> attachmentViewsList(handler().shell().context().imageCount);
    data.framebuffers.resize(attachmentViewsList.size());

    vk::FramebufferCreateInfo createInfo = {};
    createInfo.renderPass = pass;
    createInfo.width = extent_.width;
    createInfo.height = extent_.height;
    createInfo.layers = pTextures_[0]->samplers[0].imgCreateInfo.arrayLayers;

    for (uint8_t frameIndex = 0; frameIndex < attachmentViewsList.size(); frameIndex++) {
        auto& attachmentViews = attachmentViewsList[frameIndex];

        assert(pTextures_[0]->samplers[0].layerResourceMap.size() == 1);
        attachmentViews.push_back(pTextures_[0]->samplers[0].layerResourceMap.begin()->second.view);
        assert(attachmentViews.back());

        createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        createInfo.pAttachments = attachmentViews.data();

        data.framebuffers[frameIndex] =
            handler().shell().context().dev.createFramebuffer(createInfo, handler().shell().context().pAllocator);
    }
}

namespace {
std::array<PIPELINE, 5> COLOR_LIST = {
    GRAPHICS::DEFERRED_MRT_COLOR,                //
    GRAPHICS::DEFERRED_MRT_WF_COLOR,             //
    GRAPHICS::TESS_PHONG_TRI_COLOR_DEFERRED,     //
    GRAPHICS::TESS_PHONG_TRI_COLOR_WF_DEFERRED,  //
    GRAPHICS::GEOMETRY_SILHOUETTE_DEFERRED,      //
    // GRAPHICS::PRTCL_WAVE_DEFERRED,               //
};
std::array<PIPELINE, 2> TEX_LIST = {
    GRAPHICS::DEFERRED_MRT_TEX,
    /**
     * There are two reasons I can think of why cloth below won't work. First, the "mesh" for cloth is not stored in the
     * scene, and I am not sure if its compatible but I would imagine it is. Secondly, there would need to be a triangle
     * strip shadow pipeline.
     */
    // GRAPHICS::PRTCL_CLOTH_DEFERRED,
};
}  // namespace

void Base::record(const uint8_t frameIndex, const RENDER_PASS& surrogatePassType,
                  std::vector<PIPELINE>& surrogatePipelineTypes, const vk::CommandBuffer& priCmd) {
    if (getStatus() != STATUS::READY) update();
    if (getStatus() == STATUS::READY) {
        beginPass(priCmd, frameIndex, vk::SubpassContents::eInline);

        auto& secCmd = data.secCmds[frameIndex];
        auto& pScene = handler().sceneHandler().getActiveScene();

        std::vector<PIPELINE>::iterator itSurrogate;

        // COLOR
        for (const auto& pipelineType : COLOR_LIST) {
            itSurrogate = std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(), pipelineType);
            if (itSurrogate != surrogatePipelineTypes.end()) {
                pScene->record(surrogatePassType, *itSurrogate, pipelineBindDataList_.getValue(GRAPHICS::SHADOW_COLOR),
                               priCmd, secCmd, frameIndex, &getDescSetBindDataMap(GRAPHICS::SHADOW_COLOR).begin()->second);
                surrogatePipelineTypes.erase(itSurrogate);
            }
        }

        priCmd.nextSubpass(vk::SubpassContents::eInline);

        // PRTCL_FOUNTAIN_EULER_DEFERRED
        itSurrogate = std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(),
                                PIPELINE{GRAPHICS::PRTCL_FOUNTAIN_EULER_DEFERRED});
        if (itSurrogate != std::end(surrogatePipelineTypes) &&
            pipelineBindDataList_.hasKey(GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER)) {
            handler().particleHandler().recordDraw(TYPE, pipelineBindDataList_.getValue(GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER),
                                                   priCmd, frameIndex);

            surrogatePipelineTypes.erase(itSurrogate);
            priCmd.nextSubpass(vk::SubpassContents::eInline);
        }

        // TEXTURE
        for (const auto& pipelineType : TEX_LIST) {
            itSurrogate = std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(), pipelineType);
            if (itSurrogate != surrogatePipelineTypes.end()) {
                pScene->record(surrogatePassType, *itSurrogate, pipelineBindDataList_.getValue(GRAPHICS::SHADOW_TEX), priCmd,
                               secCmd, frameIndex, &getDescSetBindDataMap(GRAPHICS::SHADOW_TEX).begin()->second);
                surrogatePipelineTypes.erase(itSurrogate);
            }
        }

        endPass(priCmd);
    }
}

// DEFAULT
const CreateInfo DEFAULT_CREATE_INFO = {
    RENDER_PASS::SHADOW,
    "Shadow Render Pass",
    {
        GRAPHICS::SHADOW_COLOR,
        GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER,
        GRAPHICS::SHADOW_TEX,
    },
    FLAG::DEPTH,  // This actually enables the depth test from overridePipelineCreateInfo. Not sure if I like this.
    {
        std::string(Texture::Shadow::MAP_CUBE_ARRAY_ID),
    },
};
Default::Default(Pass::Handler& handler, const index&& offset)
    : RenderPass::Shadow::Base{handler, std::forward<const index>(offset), &DEFAULT_CREATE_INFO} {}

}  // namespace Shadow
}  // namespace RenderPass
