
#include "RenderPassDeferred.h"

#include "ConstantsAll.h"
#include "Deferred.h"
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
        PIPELINE::DEFERRED_MRT_COLOR,
        PIPELINE::DEFERRED_MRT,
        // PIPELINE::DEFERRED_SSAO,
        PIPELINE::DEFERRED_COMBINE,
    },
    (FLAG::SWAPCHAIN | FLAG::DEPTH),
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
};

Deferred::Deferred(RenderPass::Handler& handler, const index&& offset)
    : RenderPass::Base{handler, std::forward<const index>(offset), &Deferred_CREATE_INFO} {
    status_ = STATUS::PENDING_MESH | STATUS::PENDING_PIPELINE;
}

void Deferred::record(const uint8_t frameIndex) {
    if (getStatus() != STATUS::READY) update();
    if (getStatus() == STATUS::READY) {
        beginInfo_.framebuffer = data.framebuffers[frameIndex];
        auto& priCmd = data.priCmds[frameIndex];

        vkResetCommandBuffer(priCmd, 0);

        VkCommandBufferBeginInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        bufferInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vk::assert_success(vkBeginCommandBuffer(priCmd, &bufferInfo));

        beginPass(priCmd, frameIndex, VK_SUBPASS_CONTENTS_INLINE);

        // MRT COLOR
        {
            auto& secCmd = data.secCmds[frameIndex];
            auto& pScene = handler().sceneHandler().getActiveScene();

            auto it = pipelineTypeBindDataMap_.find(PIPELINE::DEFERRED_MRT_COLOR);
            assert(it != pipelineTypeBindDataMap_.end());
            pScene->record(TYPE, it->first, it->second, priCmd, secCmd, frameIndex);

            vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
        }

        // MRT
        {
            auto& secCmd = data.secCmds[frameIndex];
            auto& pScene = handler().sceneHandler().getActiveScene();

            auto it = pipelineTypeBindDataMap_.find(PIPELINE::DEFERRED_MRT);
            assert(it != pipelineTypeBindDataMap_.end());
            pScene->record(TYPE, it->first, it->second, priCmd, secCmd, frameIndex);

            vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
        }

        // SSAO
        {
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
            auto itPipelineBindData = pipelineTypeBindDataMap_.find(PIPELINE::DEFERRED_COMBINE);
            assert(itPipelineBindData != pipelineTypeBindDataMap_.end());

            handler().getScreenQuad()->draw(TYPE, itPipelineBindData->second, descSetBindDataMap_.begin()->second, priCmd,
                                            frameIndex);
        }

        endPass(priCmd);
    }
    // vk::assert_success(vkEndCommandBuffer(data.priCmds[frameIndex]));
}

void Deferred::update() {
    // Check the mesh status.
    if (handler().getScreenQuad()->getStatus() == STATUS::READY) {
        status_ ^= STATUS::PENDING_MESH;

        bool ssaoReady = true;  // false;
        {
            // auto it = pipelineTypeBindDataMap_.find(PIPELINE::DEFERRED_SSAO);
            // assert(it != pipelineTypeBindDataMap_.end());
            // auto& pPipeline = handler().pipelineHandler().getPipeline(it->first);

            //// If the pipeline is not ready try to update once.
            // if (pPipeline->getStatus() != STATUS::READY) pPipeline->updateStatus();

            // if (pPipeline->getStatus() == STATUS::READY) {
            //    // Get or make descriptor bind data.
            //    if (descSetBindDataMap2_.empty())
            //        handler().descriptorHandler().getBindData(it->first, descSetBindDataMap2_, nullptr, nullptr);

            //    assert(descSetBindDataMap2_.size() == 1);  // Not dealing with anything else atm.
            //}

            // ssaoReady = pPipeline->getStatus() == STATUS::READY;
        }

        bool combineReady = false;
        {
            auto it = pipelineTypeBindDataMap_.find(PIPELINE::DEFERRED_COMBINE);
            assert(it != pipelineTypeBindDataMap_.end());
            auto& pPipeline = handler().pipelineHandler().getPipeline(it->first);

            // If the pipeline is not ready try to update once.
            if (pPipeline->getStatus() != STATUS::READY) pPipeline->updateStatus();

            if (pPipeline->getStatus() == STATUS::READY) {
                // Get or make descriptor bind data.
                if (descSetBindDataMap_.empty())
                    handler().descriptorHandler().getBindData(it->first, descSetBindDataMap_, nullptr, nullptr);

                assert(descSetBindDataMap_.size() == 1);  // Not dealing with anything else atm.
            }

            combineReady = pPipeline->getStatus() == STATUS::READY;
        }

        if (ssaoReady && combineReady) {
            status_ ^= STATUS::PENDING_PIPELINE;
            assert(status_ == STATUS::READY);
        }
    }
}

void Deferred::createAttachmentsAndSubpasses() {
    if (pipelineData_.usesDepth) {
        // REFERENCE
        resources_.depthStencilAttachment = {static_cast<uint32_t>(resources_.attachments.size()),
                                             VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};
        // DEPTH
        resources_.attachments.push_back({});
        resources_.attachments.back().format = getDepthFormat();
        resources_.attachments.back().samples = getSamples();
        resources_.attachments.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        resources_.attachments.back().storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        resources_.attachments.back().stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        resources_.attachments.back().stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        resources_.attachments.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        resources_.attachments.back().finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        resources_.attachments.back().flags = 0;
    }

    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    // SWAPCHAIN ATTACHMENT
    resources_.attachments.push_back({});
    resources_.attachments.back().format = VK_FORMAT_B8G8R8A8_UNORM;  // getFormat();
    resources_.attachments.back().samples = VK_SAMPLE_COUNT_1_BIT;
    resources_.attachments.back().loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    resources_.attachments.back().storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    resources_.attachments.back().stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    resources_.attachments.back().stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    resources_.attachments.back().initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    resources_.attachments.back().finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    resources_.attachments.back().flags = 0;

    VkAttachmentDescription attachment = {
        0,                                        // flags VkAttachmentDescriptionFlags
        VK_FORMAT_UNDEFINED,                      // format VkFormat
        VK_SAMPLE_COUNT_1_BIT,                    // samples VkSampleCountFlagBits
        VK_ATTACHMENT_LOAD_OP_CLEAR,              // loadOp VkAttachmentLoadOp
        VK_ATTACHMENT_STORE_OP_STORE,             // storeOp VkAttachmentStoreOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,          // stencilLoadOp VkAttachmentLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,         // stencilStoreOp VkAttachmentStoreOp
        VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout VkImageLayout
                                                  // getFinalLayout()                   // finalLayout VkImageLayout
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

    // NORMAL
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::NORM_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;

    // DIFFUSE
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::DIFFUSE_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;

    // AMBIENT
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::AMBIENT_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;

    // SPECULAR
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::SPECULAR_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;

    // SSAO
    bool doSSAO = false;
    if (doSSAO) {
        resources_.colorAttachments.push_back(
            {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
        resources_.attachments.push_back(attachment);
        pTexture = handler().textureHandler().getTexture(Texture::Deferred::SSAO_2D_ID);
        resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
        assert(resources_.colorAttachments.size() == 7);
    } else {
        assert(resources_.colorAttachments.size() == 6);
    }

    VkSubpassDescription subpassDesc;

    // MRT
    subpassDesc = {};
    subpassDesc.colorAttachmentCount = 5;
    subpassDesc.pColorAttachments = &resources_.colorAttachments[1];  // POSITION/NORMAL/DIFFUSE/AMBIENT/SPECULAR
    subpassDesc.pResolveAttachments = nullptr;
    subpassDesc.pDepthStencilAttachment = pipelineData_.usesDepth ? &resources_.depthStencilAttachment : nullptr;
    resources_.subpasses.assign(2, subpassDesc);  // TEXTURE/COLOR

    // SSAO
    if (doSSAO) {
        subpassDesc = {};
        subpassDesc.colorAttachmentCount = 1;
        subpassDesc.pColorAttachments = &resources_.colorAttachments[6];  // SSAO
        subpassDesc.pResolveAttachments = nullptr;
        subpassDesc.pDepthStencilAttachment = nullptr;
        resources_.subpasses.push_back(subpassDesc);
    }

    // COMBINE
    subpassDesc = {};
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &resources_.colorAttachments[0];  // SWAPCHAIN
    subpassDesc.pResolveAttachments = nullptr;
    subpassDesc.pDepthStencilAttachment = nullptr;
    resources_.subpasses.push_back(subpassDesc);
}

void Deferred::createDependencies() {
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

    // Below should make sense
    bool doSSAO = false;
    if (doSSAO) {
        dependency = {};
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
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
        dependency = {};
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependency.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        // Color to combine.
        dependency.srcSubpass = 0;
        dependency.dstSubpass = 2;
        resources_.dependencies.push_back(dependency);
        // Texture to combine.
        dependency.srcSubpass = 1;
        dependency.dstSubpass = 2;
        resources_.dependencies.push_back(dependency);
    }
}

void Deferred::updateClearValues() {
    // Depth/Swapchain
    Base::updateClearValues();
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
    //// SSAO
    // clearValues_.push_back({});
    // clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
}

void Deferred::createFramebuffers() {
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

        //// SSAO
        // pTexture = handler().textureHandler().getTexture(Texture::Deferred::SSAO_2D_ID);
        // attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
        // assert(attachmentViews.back() != VK_NULL_HANDLE);

        createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        createInfo.pAttachments = attachmentViews.data();

        vk::assert_success(
            vkCreateFramebuffer(handler().shell().context().dev, &createInfo, nullptr, &data.framebuffers[frameIndex]));
    }
}

}  // namespace Deferred
}  // namespace RenderPass
