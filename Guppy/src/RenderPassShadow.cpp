
#include "RenderPassShadow.h"

#include "ConstantsAll.h"
#include "Shadow.h"
// HANDLERS
#include "PipelineHandler.h"
#include "DescriptorHandler.h"
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

    auto& pTexture = pTextures_[0]->samplers[0];

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

void Base::record(const uint8_t frameIndex, const PASS& surrogatePassType, std::vector<PIPELINE>& surrogatePipelineTypes,
                  const VkCommandBuffer& priCmd) {
    if (getStatus() != STATUS::READY) update();
    if (getStatus() == STATUS::READY) {
        beginPass(priCmd, frameIndex, VK_SUBPASS_CONTENTS_INLINE);

        auto& secCmd = data.secCmds[frameIndex];
        auto& pScene = handler().sceneHandler().getActiveScene();

        std::vector<PIPELINE>::iterator itSurrogate;

        // Skip these for now. This means they won't cast a shadow...
        {
            itSurrogate = std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(),
                                    PIPELINE::TESSELLATION_BEZIER_4_DEFERRED);
            if (itSurrogate != surrogatePipelineTypes.end()) surrogatePipelineTypes.erase(itSurrogate);

            itSurrogate =
                std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(), PIPELINE::DEFERRED_MRT_LINE);
            if (itSurrogate != surrogatePipelineTypes.end()) surrogatePipelineTypes.erase(itSurrogate);
        }

        // COLOR
        {
            itSurrogate =
                std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(), PIPELINE::DEFERRED_MRT_COLOR);
            if (itSurrogate != surrogatePipelineTypes.end()) {
                pScene->record(surrogatePassType, *itSurrogate, pipelineBindDataList_.getValue(PIPELINE::SHADOW_COLOR),
                               priCmd, secCmd, frameIndex, &getDescSetBindDataMap(PIPELINE::SHADOW_COLOR).begin()->second);
                surrogatePipelineTypes.erase(itSurrogate);
            }

            // WIREFRAME
            itSurrogate =
                std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(), PIPELINE::DEFERRED_MRT_WF_COLOR);
            if (itSurrogate != surrogatePipelineTypes.end()) {
                pScene->record(surrogatePassType, *itSurrogate, pipelineBindDataList_.getValue(PIPELINE::SHADOW_COLOR),
                               priCmd, secCmd, frameIndex, &getDescSetBindDataMap(PIPELINE::SHADOW_COLOR).begin()->second);
                surrogatePipelineTypes.erase(itSurrogate);
            }

            // TRIANGLE
            itSurrogate = std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(),
                                    PIPELINE::TESSELLATION_TRIANGLE_DEFERRED);
            if (itSurrogate != surrogatePipelineTypes.end()) {
                pScene->record(surrogatePassType, *itSurrogate, pipelineBindDataList_.getValue(PIPELINE::SHADOW_COLOR),
                               priCmd, secCmd, frameIndex, &getDescSetBindDataMap(PIPELINE::SHADOW_COLOR).begin()->second);
                surrogatePipelineTypes.erase(itSurrogate);
            }

            // SILHOUETTE
            itSurrogate = std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(),
                                    PIPELINE::GEOMETRY_SILHOUETTE_DEFERRED);
            if (itSurrogate != surrogatePipelineTypes.end()) {
                pScene->record(surrogatePassType, *itSurrogate, pipelineBindDataList_.getValue(PIPELINE::SHADOW_COLOR),
                               priCmd, secCmd, frameIndex, &getDescSetBindDataMap(PIPELINE::SHADOW_COLOR).begin()->second);
                surrogatePipelineTypes.erase(itSurrogate);
            }

            vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
        }

        // TEXTURE
        {
            itSurrogate =
                std::find(surrogatePipelineTypes.begin(), surrogatePipelineTypes.end(), PIPELINE::DEFERRED_MRT_TEX);
            if (itSurrogate != surrogatePipelineTypes.end()) {
                pScene->record(surrogatePassType, *itSurrogate, pipelineBindDataList_.getValue(PIPELINE::SHADOW_TEX), priCmd,
                               secCmd, frameIndex, &getDescSetBindDataMap(PIPELINE::SHADOW_TEX).begin()->second);
                surrogatePipelineTypes.erase(itSurrogate);
            }
        }

        assert(surrogatePipelineTypes.size() == 1 && surrogatePipelineTypes[0] == PIPELINE::DEFERRED_COMBINE);

        endPass(priCmd);
    }
}

// DEFAULT
const CreateInfo DEFAULT_CREATE_INFO = {
    PASS::SHADOW,
    "Shadow Render Pass",
    {PIPELINE::SHADOW_COLOR, PIPELINE::SHADOW_TEX},
    FLAG::DEPTH,  // This actually enables the depth test from overridePipelineCreateInfo. Not sure if I like this.
    {
        std::string(Texture::Shadow::MAP_2D_ARRAY_ID),
    },
    {},
    {},
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    {
        {{UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, PIPELINE::ALL_ENUM}, {2}},
    },
};
Default::Default(Handler& handler, const index&& offset)
    : RenderPass::Shadow::Base{handler, std::forward<const index>(offset), &DEFAULT_CREATE_INFO} {}

}  // namespace Shadow
}  // namespace RenderPass
