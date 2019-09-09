
#include "RenderPassDeferred.h"

#include "ConstantsAll.h"
#include "Deferred.h"
#include "RenderPassShadow.h"
// HANDLERS
#include "PipelineHandler.h"
#include "DescriptorHandler.h"
#include "RenderPassHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

namespace RenderPass {
namespace Deferred {

// DEFERRED

const CreateInfo Deferred_CREATE_INFO = {
    PASS::DEFERRED,
    "Deferred Render Pass",
    {
        PIPELINE::DEFERRED_BEZIER_4,
        PIPELINE::DEFERRED_MRT_LINE,
        PIPELINE::DEFERRED_MRT_COLOR,
        PIPELINE::DEFERRED_MRT_TEX,
        // PIPELINE::DEFERRED_SSAO,
        PIPELINE::DEFERRED_COMBINE,
    },
    (FLAG::SWAPCHAIN | FLAG::DEPTH | (::Deferred::DO_MSAA ? FLAG::MULTISAMPLE : FLAG::NONE)),
    {
        std::string(RenderPass::SWAPCHAIN_TARGET_ID),
        // TODO: Make below work with above in the base class.
        // std::string(Texture::Deferred::POS_NORM_2D_ARRAY_ID),
        // std::string(Texture::Deferred::POS_2D_ID),
        // std::string(Texture::Deferred::NORM_2D_ID),
        // std::string(Texture::Deferred::DIFFUSE_2D_ID),
        // std::string(Texture::Deferred::AMBIENT_2D_ID),
        // std::string(Texture::Deferred::SPECULAR_2D_ID),
    },
    {PASS::SHADOW},
};

Base::Base(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::Base{handler, std::forward<const index>(offset), &Deferred_CREATE_INFO}, doSSAO_(false) {
    status_ = STATUS::PENDING_MESH | STATUS::PENDING_PIPELINE;
}

// TODO: should something like this be on the base class????
void Base::init() {
    // Initialize the dependent pass types. Currently they all will come prior to the
    // main loop pass.
    for (const auto& [passType, offset] : dependentTypeOffsetPairs_) {
        // Boy is this going to be confusing if it ever doesn't work right.
        if (passType == TYPE) {
            RenderPass::Base::init();
        } else {
            const auto& pPass = handler().getPass(offset);
            if (!pPass->isIntialized()) const_cast<RenderPass::Base*>(pPass.get())->init();
        }
    }
}

void Base::record(const uint8_t frameIndex) {
    if (getStatus() != STATUS::READY) update();
    if (getStatus() == STATUS::READY) {
        beginInfo_.framebuffer = data.framebuffers[frameIndex];
        auto& priCmd = data.priCmds[frameIndex];

        vkResetCommandBuffer(priCmd, 0);

        VkCommandBufferBeginInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bufferInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vk::assert_success(vkBeginCommandBuffer(priCmd, &bufferInfo));

        // SHADOW
        {
            const auto& pPass = handler().getPass(dependentTypeOffsetPairs_[0].second);
            assert(pPass->TYPE == PASS::SHADOW);
            std::vector<PIPELINE> pipelineTypes;
            pipelineTypes.reserve(pipelineBindDataList_.size());
            for (const auto& [pipelineType, value] : pipelineBindDataList_.getKeyOffsetMap())
                pipelineTypes.push_back(pipelineType);
            ((Shadow::Default*)pPass.get())->record(frameIndex, TYPE, pipelineTypes, priCmd);
        }

        beginPass(priCmd, frameIndex, VK_SUBPASS_CONTENTS_INLINE);

        auto& pScene = handler().sceneHandler().getActiveScene();

        // MRT (BEZIER 4)
        {
            auto& secCmd = data.secCmds[frameIndex];

            pScene->record(TYPE, PIPELINE::DEFERRED_BEZIER_4, pipelineBindDataList_.getValue(PIPELINE::DEFERRED_BEZIER_4),
                           priCmd, secCmd, frameIndex);

            vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
        }

        // MRT (LINE)
        {
            auto& secCmd = data.secCmds[frameIndex];

            pScene->record(TYPE, PIPELINE::DEFERRED_MRT_LINE, pipelineBindDataList_.getValue(PIPELINE::DEFERRED_MRT_LINE),
                           priCmd, secCmd, frameIndex);

            vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
        }

        // MRT (COLOR)
        {
            auto& secCmd = data.secCmds[frameIndex];

            pScene->record(TYPE, PIPELINE::DEFERRED_MRT_COLOR, pipelineBindDataList_.getValue(PIPELINE::DEFERRED_MRT_COLOR),
                           priCmd, secCmd, frameIndex);

            vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
        }

        // MRT (TEXTURE)
        {
            auto& secCmd = data.secCmds[frameIndex];

            pScene->record(TYPE, PIPELINE::DEFERRED_MRT_TEX, pipelineBindDataList_.getValue(PIPELINE::DEFERRED_MRT_TEX),
                           priCmd, secCmd, frameIndex);

            vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
        }

        // SSAO
        if (doSSAO_) {
            assert(false);  // This used to use descSetBindDataMap2_. I think I made a map so this isn't necessary.

            //// TODO: this definitely only needs to be recorded once per swapchain creation!!!
            // auto itPipelineBindData = pipelineTypeBindDataMap_.find(PIPELINE::DEFERRED_SSAO);
            // assert(itPipelineBindData != pipelineTypeBindDataMap_.end());

            // handler().getScreenQuad()->draw(TYPE, itPipelineBindData->second, descSetBindDataMap2_.begin()->second,
            // priCmd,
            //                                frameIndex);

            // vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
        }

        // COMBINE
        {
            // TODO: this definitely only needs to be recorded once per swapchain creation!!!
            handler().getScreenQuad()->draw(TYPE, pipelineBindDataList_.getValue(PIPELINE::DEFERRED_COMBINE),
                                            getDescSetBindDataMap(PIPELINE::DEFERRED_COMBINE).begin()->second, priCmd,
                                            frameIndex);
        }

        endPass(priCmd);
    }
    // vk::assert_success(vkEndCommandBuffer(data.priCmds[frameIndex]));
}

void Base::update() {
    // Check the mesh status.
    if (handler().getScreenQuad()->getStatus() == STATUS::READY) {
        status_ ^= STATUS::PENDING_MESH;
        RenderPass::Base::update();
    }
}

void Base::createAttachments() {
    // DEPTH/RESOLVE/SWAPCHAIN
    ::RenderPass::Base::createAttachments();

    VkAttachmentDescription attachment = {
        0,                                        // flags VkAttachmentDescriptionFlags
        VK_FORMAT_UNDEFINED,                      // format VkFormat
        getSamples(),                             // samples VkSampleCountFlagBits
        VK_ATTACHMENT_LOAD_OP_CLEAR,              // loadOp VkAttachmentLoadOp
        VK_ATTACHMENT_STORE_OP_STORE,             // storeOp VkAttachmentStoreOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stencilLoadOp VkAttachmentLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stencilStoreOp VkAttachmentStoreOp
        VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout VkImageLayout
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL  // finalLayout VkImageLayout
    };

    // assert(textureIds_.size() == 3 && pTextures_.size() >= textureIds_.size() &&
    //       pTextures_.size() % textureIds_.size() == 0);

    // auto numTex = pTextures_.size() / textureIds_.size();

    // POSITION
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    auto pTexture = handler().textureHandler().getTexture(Texture::Deferred::POS_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());

    // NORMAL
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::NORM_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());

    // DIFFUSE
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::DIFFUSE_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());

    // AMBIENT
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::AMBIENT_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());

    // SPECULAR
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::SPECULAR_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());

    // SSAO
    if (doSSAO_) {
        resources_.colorAttachments.push_back(
            {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        resources_.attachments.push_back(attachment);
        pTexture = handler().textureHandler().getTexture(Texture::Deferred::SSAO_2D_ID);
        resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
        assert(resources_.colorAttachments.size() == 7);
    } else {
        assert(resources_.colorAttachments.size() == 6);
    }
}

void Base::createSubpassDescriptions() {
    VkSubpassDescription subpassDesc;

    // MRT
    subpassDesc = {};
    subpassDesc.colorAttachmentCount = 5;
    subpassDesc.pColorAttachments = &resources_.colorAttachments[1];  // POSITION/NORMAL/DIFFUSE/AMBIENT/SPECULAR
    subpassDesc.pResolveAttachments = nullptr;
    subpassDesc.pDepthStencilAttachment = pipelineData_.usesDepth ? &resources_.depthStencilAttachment : nullptr;
    resources_.subpasses.assign(4, subpassDesc);  // TEXTURE/COLOR/LINE/BEZIER4

    // SSAO
    if (doSSAO_) {
        subpassDesc = {};
        subpassDesc.colorAttachmentCount = 1;
        subpassDesc.pColorAttachments = &resources_.colorAttachments[6];  // SSAO
        subpassDesc.pResolveAttachments = nullptr;
        subpassDesc.pDepthStencilAttachment = nullptr;
        resources_.subpasses.push_back(subpassDesc);
    }

    if (usesMultiSample()) assert(resources_.resolveAttachments.size() == 1);

    // COMBINE
    subpassDesc = {};
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &resources_.colorAttachments[0];  // SWAPCHAIN
    subpassDesc.pResolveAttachments = resources_.resolveAttachments.data();
    subpassDesc.pDepthStencilAttachment = nullptr;
    resources_.subpasses.push_back(subpassDesc);

    assert(resources_.subpasses.size() == pipelineBindDataList_.size());
}

void Base::createDependencies() {
    VkSubpassDependency dependency = {};

    // Garbage from before
    dependency = {};
    dependency.srcSubpass = 0;
    dependency.dstSubpass = 1;
    dependency.dependencyFlags = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    dependency.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    resources_.dependencies.push_back(dependency);
    dependency.dstSubpass = 1;
    resources_.dependencies.push_back(dependency);
    dependency.dstSubpass = 2;
    resources_.dependencies.push_back(dependency);
    dependency.dstSubpass = 3;
    resources_.dependencies.push_back(dependency);

    // Below should make sense
    dependency = {};
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    if (doSSAO_) {
        assert(false);
        // Color to SSAO.
        dependency.srcSubpass = 0;
        dependency.dstSubpass = 2;
        resources_.dependencies.push_back(dependency);
        // Texture to SSAO.
        dependency.srcSubpass = 1;
        dependency.dstSubpass = 2;
        resources_.dependencies.push_back(dependency);
        // SSAO to combine
        dependency.srcSubpass = 2;
        dependency.dstSubpass = 3;
        resources_.dependencies.push_back(dependency);
    } else {
        // Bezier 4 to Line
        dependency.srcSubpass = 1;
        dependency.dstSubpass = 2;
        resources_.dependencies.push_back(dependency);
        // Line to Color
        dependency.srcSubpass = 2;
        dependency.dstSubpass = 3;
        resources_.dependencies.push_back(dependency);

        // I think the reason why a dependency is not needed from the "color" pipelines to
        // the "texture" ones is because they are not compatible, and the driver can detect
        // it somehow. Not sure though.

        // Bezier 4 to combine.
        dependency.srcSubpass = 0;
        dependency.dstSubpass = 4;
        resources_.dependencies.push_back(dependency);
        // Line to combine.
        dependency.srcSubpass = 1;
        dependency.dstSubpass = 4;
        resources_.dependencies.push_back(dependency);
        // Color to combine.
        dependency.srcSubpass = 2;
        dependency.dstSubpass = 4;
        resources_.dependencies.push_back(dependency);
        // Texture to combine.
        dependency.srcSubpass = 3;
        dependency.dstSubpass = 4;
        resources_.dependencies.push_back(dependency);
    }
}

void Base::updateClearValues() {
    // Depth/Swapchain
    RenderPass::Base::updateClearValues();
    // Position
    clearValues_.push_back({});
    clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
    // Normal
    clearValues_.push_back({});
    clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
    // Diffuse
    clearValues_.push_back({});
    clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
    // Ambient
    clearValues_.push_back({});
    clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
    // Specular
    clearValues_.push_back({});
    clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
    // SSAO
    if (doSSAO_) {
        clearValues_.push_back({});
        clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
    }
}

void Base::createFramebuffers() {
    /* Views for framebuffer.
     *  - depth
     *  - position
     *  - normal
     *  - color
     */
    std::vector<std::vector<VkImageView>> attachmentViewsList(handler().shell().context().imageCount);
    data.framebuffers.resize(attachmentViewsList.size());

    VkFramebufferCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    createInfo.renderPass = pass;
    createInfo.width = extent_.width;
    createInfo.height = extent_.height;
    createInfo.layers = 1;

    // assert(textureIds_.size() == 3 && pTextures_.size() >= textureIds_.size() &&
    //       pTextures_.size() % textureIds_.size() == 0);

    // auto numTex = pTextures_.size() / textureIds_.size();

    for (uint8_t frameIndex = 0; frameIndex < attachmentViewsList.size(); frameIndex++) {
        auto& attachmentViews = attachmentViewsList[frameIndex];

        // DEPTH
        if (pipelineData_.usesDepth) {
            assert(depth_.view != VK_NULL_HANDLE);
            attachmentViews.push_back(depth_.view);
        }

        // MULTI-SAMPLE
        if (usesMultiSample()) {
            assert(images_.size() == 1);
            assert(images_[0].view != VK_NULL_HANDLE);
            attachmentViews.push_back(images_[0].view);  // TODO: should there be one per swapchain image????????????????
        }

        // SWAPCHAIN
        attachmentViews.push_back(handler().getSwapchainViews()[frameIndex]);
        assert(attachmentViews.back() != VK_NULL_HANDLE);

        // POSITION
        auto pTexture = handler().textureHandler().getTexture(Texture::Deferred::POS_2D_ID);
        attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
        assert(attachmentViews.back() != VK_NULL_HANDLE);

        // NORMAL
        pTexture = handler().textureHandler().getTexture(Texture::Deferred::NORM_2D_ID);
        attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
        assert(attachmentViews.back() != VK_NULL_HANDLE);

        // DIFFUSE
        pTexture = handler().textureHandler().getTexture(Texture::Deferred::DIFFUSE_2D_ID);
        attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
        assert(attachmentViews.back() != VK_NULL_HANDLE);

        // AMBIENT
        pTexture = handler().textureHandler().getTexture(Texture::Deferred::AMBIENT_2D_ID);
        attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
        assert(attachmentViews.back() != VK_NULL_HANDLE);

        // SPECULAR
        pTexture = handler().textureHandler().getTexture(Texture::Deferred::SPECULAR_2D_ID);
        attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
        assert(attachmentViews.back() != VK_NULL_HANDLE);

        // SSAO
        if (doSSAO_) {
            pTexture = handler().textureHandler().getTexture(Texture::Deferred::SSAO_2D_ID);
            attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
            assert(attachmentViews.back() != VK_NULL_HANDLE);
        }

        createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        createInfo.pAttachments = attachmentViews.data();

        vk::assert_success(
            vkCreateFramebuffer(handler().shell().context().dev, &createInfo, nullptr, &data.framebuffers[frameIndex]));
    }
}

}  // namespace Deferred
}  // namespace RenderPass
