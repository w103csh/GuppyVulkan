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
                                         helpers::getDepthStencilAttachmentLayout(depthFormat_)};

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

// DEFAULT
const CreateInfo DEFAULT_CREATE_INFO = {
    RENDER_PASS::SHADOW_DEFAULT,
    "Shadow Render Pass",
    {
        GRAPHICS::SHADOW_COLOR,
        GRAPHICS::SHADOW_TEX,
    },
    FLAG::DEPTH,  // This actually enables the depth test from overridePipelineCreateInfo. Not sure if I like this.
    {std::string(Texture::Shadow::MAP_2D_ARRAY_ID)},
};
Default::Default(Pass::Handler& handler, const index&& offset)
    : RenderPass::Shadow::Base{handler, std::forward<const index>(offset), &DEFAULT_CREATE_INFO} {}

void Default::record(const uint8_t frameIndex, const RENDER_PASS& surrogatePassType,
                     std::vector<PIPELINE>& surrogatePipelineTypes, const vk::CommandBuffer& priCmd) {
    if (getStatus() != STATUS::READY) update();
    if (getStatus() == STATUS::READY) {
        beginPass(priCmd, frameIndex, vk::SubpassContents::eInline);

        auto& secCmd = data.secCmds[frameIndex];
        auto& pScene = handler().sceneHandler().getActiveScene();

        std::vector<PIPELINE>::iterator itSurrogate;

        // PIPELINE TYPE
        const auto COLOR_TYPE = GRAPHICS::SHADOW_COLOR;
        const auto TEX_TYPE = GRAPHICS::SHADOW_TEX;

        // COLOR
        for (const auto& pipelineType : COLOR_LIST) {
            itSurrogate = std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(), pipelineType);
            if (itSurrogate != surrogatePipelineTypes.end()) {
                pScene->record(surrogatePassType, *itSurrogate, pipelineBindDataList_.getValue(COLOR_TYPE), priCmd, secCmd,
                               frameIndex, &getDescSetBindDataMap(COLOR_TYPE).begin()->second);
                surrogatePipelineTypes.erase(itSurrogate);
            }
        }

        priCmd.nextSubpass(vk::SubpassContents::eInline);

        // TEXTURE
        for (const auto& pipelineType : TEX_LIST) {
            itSurrogate = std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(), pipelineType);
            if (itSurrogate != surrogatePipelineTypes.end()) {
                pScene->record(surrogatePassType, *itSurrogate, pipelineBindDataList_.getValue(TEX_TYPE), priCmd, secCmd,
                               frameIndex, &getDescSetBindDataMap(TEX_TYPE).begin()->second);
                surrogatePipelineTypes.erase(itSurrogate);
            }
        }

        endPass(priCmd);
    }
}

void Default::update(const std::vector<Descriptor::Base*> pDynamicItems) {
    assert(status_ & STATUS::PENDING_PIPELINE);
    assert(pDynamicItems.empty());  // Not accounting for pDynamicItems below atm.

    bool isReady = true;

    {  // These pipeline both have the same bind data so do them in a loop
        std::array<GRAPHICS, 2> pipelineTypes = {GRAPHICS::SHADOW_COLOR, GRAPHICS::SHADOW_TEX};

        // As of now there should be a single basic camera for a single volumetric spot light.
        assert(handler().uniformHandler().camPersBscMgr().pItems.size() == 1);
        auto& pCamera = handler().uniformHandler().camPersBscMgr().pItems.at(0);

        for (const auto pipelineType : pipelineTypes) {
            auto& pPipeline = handler().pipelineHandler().getPipeline(pipelineType);
            if (pPipeline->getStatus() != STATUS::READY) pPipeline->updateStatus();
            if (pPipeline->getStatus() == STATUS::READY) {
                if (pipelineDescSetBindDataMap_.count(pipelineType) == 0) {
                    auto it = pipelineDescSetBindDataMap_.insert({pipelineType, {}});
                    assert(it.second);
                    // Get or make descriptor bind data.
                    handler().descriptorHandler().getBindData(pipelineType, it.first->second, {pCamera.get()});
                    assert(it.first->second.size());
                }
            }

            isReady &= pPipeline->getStatus() == STATUS::READY;
        }
    }

    if (isReady) {
        status_ ^= STATUS::PENDING_PIPELINE;
        assert(status_ == STATUS::READY);
    }
}

// CUBEMAP
const CreateInfo CUBE_CREATE_INFO = {
    RENDER_PASS::SHADOW_CUBE,
    "Shadow Cubemap Render Pass",
    {
        GRAPHICS::SHADOW_COLOR_CUBE,
        GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER,
        GRAPHICS::SHADOW_TEX_CUBE,
    },
    FLAG::DEPTH,  // This actually enables the depth test from overridePipelineCreateInfo. Not sure if I like this.
    {std::string(Texture::Shadow::MAP_CUBE_ARRAY_ID)},
};
Cube::Cube(Pass::Handler& handler, const index&& offset)
    : RenderPass::Shadow::Base{handler, std::forward<const index>(offset), &CUBE_CREATE_INFO} {}

void Cube::record(const uint8_t frameIndex, const RENDER_PASS& surrogatePassType,
                  std::vector<PIPELINE>& surrogatePipelineTypes, const vk::CommandBuffer& priCmd) {
    if (getStatus() != STATUS::READY) update();
    if (getStatus() == STATUS::READY) {
        beginPass(priCmd, frameIndex, vk::SubpassContents::eInline);

        auto& secCmd = data.secCmds[frameIndex];
        auto& pScene = handler().sceneHandler().getActiveScene();

        std::vector<PIPELINE>::iterator itSurrogate;

        // PIPELINE TYPE
        constexpr auto COLOR_TYPE = GRAPHICS::SHADOW_COLOR_CUBE;
        constexpr auto TEX_TYPE = GRAPHICS::SHADOW_TEX_CUBE;

        // COLOR
        for (const auto& pipelineType : COLOR_LIST) {
            itSurrogate = std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(), pipelineType);
            if (itSurrogate != surrogatePipelineTypes.end()) {
                pScene->record(surrogatePassType, *itSurrogate, pipelineBindDataList_.getValue(COLOR_TYPE), priCmd, secCmd,
                               frameIndex, &getDescSetBindDataMap(COLOR_TYPE).begin()->second);
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
                pScene->record(surrogatePassType, *itSurrogate, pipelineBindDataList_.getValue(TEX_TYPE), priCmd, secCmd,
                               frameIndex, &getDescSetBindDataMap(TEX_TYPE).begin()->second);
                surrogatePipelineTypes.erase(itSurrogate);
            }
        }

        endPass(priCmd);
    }
}

}  // namespace Shadow
}  // namespace RenderPass
