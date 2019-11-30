/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "RenderPassShadow.h"

#include <array>

#include "ConstantsAll.h"
#include "Shadow.h"
// HANDLERS
#include "PipelineHandler.h"
#include "DescriptorHandler.h"
#include "ParticleHandler.h"
#include "RenderPassHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

namespace RenderPass {
namespace Shadow {

Base::Base(RenderPass::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo)
    : RenderPass::Base{handler, std::forward<const index>(offset), pCreateInfo} {
    status_ = STATUS::PENDING_PIPELINE;
}

void Base::createAttachments() {
    resources_.depthStencilAttachment = {static_cast<uint32_t>(resources_.attachments.size()),
                                         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

    resources_.attachments.push_back({});
    resources_.attachments.back().format = pTextures_[0]->samplers[0].imgCreateInfo.format;
    resources_.attachments.back().samples = VK_SAMPLE_COUNT_1_BIT;
    resources_.attachments.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    resources_.attachments.back().storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resources_.attachments.back().stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resources_.attachments.back().stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resources_.attachments.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resources_.attachments.back().finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    resources_.attachments.back().flags = 0;
}

void Base::createDependencies() {
    ::RenderPass::Base::createDependencies();
    if (pipelineBindDataList_.hasKey(GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER)) {
        resources_.dependencies.push_back({
            VK_SUBPASS_EXTERNAL,
            pipelineBindDataList_.getOffset(GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER),
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
        });
    }
}

void Base::createFramebuffers() {
    /* Views for framebuffer.
     *  - depth
     */
    std::vector<std::vector<VkImageView>> attachmentViewsList(handler().shell().context().imageCount);
    data.framebuffers.resize(attachmentViewsList.size());

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = pass;
    createInfo.width = extent_.width;
    createInfo.height = extent_.height;
    createInfo.layers = 1;

    for (uint8_t frameIndex = 0; frameIndex < attachmentViewsList.size(); frameIndex++) {
        auto& attachmentViews = attachmentViewsList[frameIndex];

        attachmentViews.push_back(pTextures_[0]->samplers[0].layerResourceMap.at(0).view);
        assert(attachmentViews.back() != VK_NULL_HANDLE);

        createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        createInfo.pAttachments = attachmentViews.data();

        vk::assert_success(
            vkCreateFramebuffer(handler().shell().context().dev, &createInfo, nullptr, &data.framebuffers[frameIndex]));
    }
}

namespace {
std::array<PIPELINE, 5> COLOR_LIST = {
    GRAPHICS::DEFERRED_MRT_COLOR,              //
    GRAPHICS::DEFERRED_MRT_WF_COLOR,           //
    GRAPHICS::TESSELLATION_TRIANGLE_DEFERRED,  //
    GRAPHICS::GEOMETRY_SILHOUETTE_DEFERRED,    //
    // GRAPHICS::PRTCL_WAVE_DEFERRED,        //
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

void Base::record(const uint8_t frameIndex, const PASS& surrogatePassType, std::vector<PIPELINE>& surrogatePipelineTypes,
                  const VkCommandBuffer& priCmd) {
    if (getStatus() != STATUS::READY) update();
    if (getStatus() == STATUS::READY) {
        beginPass(priCmd, frameIndex, VK_SUBPASS_CONTENTS_INLINE);

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

        vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);

        // PRTCL_FOUNTAIN_EULER_DEFERRED
        itSurrogate = std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(),
                                PIPELINE{GRAPHICS::PRTCL_FOUNTAIN_EULER_DEFERRED});
        if (itSurrogate != std::end(surrogatePipelineTypes) &&
            pipelineBindDataList_.hasKey(GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER)) {
            handler().particleHandler().recordDraw(TYPE, pipelineBindDataList_.getValue(GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER),
                                                   priCmd, frameIndex);

            surrogatePipelineTypes.erase(itSurrogate);
            vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
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
    PASS::SHADOW,
    "Shadow Render Pass",
    {
        GRAPHICS::SHADOW_COLOR,
        GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER,
        GRAPHICS::SHADOW_TEX,
    },
    FLAG::DEPTH,  // This actually enables the depth test from overridePipelineCreateInfo. Not sure if I like this.
    {
        std::string(Texture::Shadow::MAP_2D_ARRAY_ID),
    },
    {},
    {},
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    {
        {{UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, GRAPHICS::ALL_ENUM}, {2}},
    },
};
Default::Default(Handler& handler, const index&& offset)
    : RenderPass::Shadow::Base{handler, std::forward<const index>(offset), &DEFAULT_CREATE_INFO} {}

}  // namespace Shadow
}  // namespace RenderPass
