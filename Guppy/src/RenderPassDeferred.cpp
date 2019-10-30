
#include "RenderPassDeferred.h"

#include "ConstantsAll.h"
#include "Deferred.h"
#include "RenderPassShadow.h"
// HANDLERS
#include "ParticleHandler.h"
#include "PipelineHandler.h"
#include "DescriptorHandler.h"
#include "RenderPassHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

namespace RenderPass {
namespace Deferred {

// DEFERRED

const CreateInfo DEFERRED_CREATE_INFO = {
    PASS::DEFERRED,
    "Deferred Render Pass",
    {
// TODO: make tess/geom pipeline/mesh optional based on the context flags.
#ifndef VK_USE_PLATFORM_MACOS_MVK
        PIPELINE::TESSELLATION_BEZIER_4_DEFERRED,
#endif
        PIPELINE::DEFERRED_MRT_LINE,
        PIPELINE::DEFERRED_MRT_COLOR,
#ifndef VK_USE_PLATFORM_MACOS_MVK
        PIPELINE::DEFERRED_MRT_WF_COLOR,
#endif
        PIPELINE::PARTICLE_WAVE_DEFERRED,
        PIPELINE::PARTICLE_FOUNTAIN_DEFERRED,
#ifndef VK_USE_PLATFORM_MACOS_MVK
        // PIPELINE::GEOMETRY_SILHOUETTE_DEFERRED,
        PIPELINE::TESSELLATION_TRIANGLE_DEFERRED,
#endif
        PIPELINE::DEFERRED_MRT_TEX,
        // The two below are dependent on one another... Make this a thing?
        PIPELINE::PARTICLE_EULER_COMPUTE,
        PIPELINE::PARTICLE_FOUNTAIN_EULER_DEFERRED,
        // PIPELINE::DEFERRED_SSAO,
        PIPELINE::DEFERRED_COMBINE,
    },
    (FLAG::SWAPCHAIN | FLAG::DEPTH | /*FLAG::DEPTH_INPUT_ATTACHMENT |*/
     (::Deferred::DO_MSAA ? FLAG::MULTISAMPLE : FLAG::NONE)),
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
    : RenderPass::Base{handler, std::forward<const index>(offset), &DEFERRED_CREATE_INFO},
      inputAttachmentOffset_(0),
      inputAttachmentCount_(0),
      combinePassIndex_(0),
      doSSAO_(false) {
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

        // COMPUTE
        {
            auto& pPipelineBindData = pipelineBindDataList_.getValue(PIPELINE::PARTICLE_EULER_COMPUTE);
            handler().particleHandler().recordDispatch(TYPE, pPipelineBindData, priCmd, frameIndex);
        }

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

        for (const auto& pPipelineBindData : pipelineBindDataList_.getValues()) {
            // Push constant
            switch (pPipelineBindData->type) {
                case PIPELINE::TESSELLATION_TRIANGLE_DEFERRED:
                case PIPELINE::GEOMETRY_SILHOUETTE_DEFERRED:
                case PIPELINE::TESSELLATION_BEZIER_4_DEFERRED:
                case PIPELINE::DEFERRED_MRT_COLOR:
                case PIPELINE::DEFERRED_MRT_LINE: {
                    ::Deferred::PushConstant pushConstant = {::Deferred::PASS_FLAG::NONE};
                    vkCmdPushConstants(priCmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                                       static_cast<uint32_t>(sizeof(::Deferred::PushConstant)), &pushConstant);
                } break;
                case PIPELINE::DEFERRED_MRT_WF_COLOR: {
                    ::Deferred::PushConstant pushConstant = {::Deferred::PASS_FLAG::WIREFRAME};
                    vkCmdPushConstants(priCmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                                       static_cast<uint32_t>(sizeof(::Deferred::PushConstant)), &pushConstant);
                } break;
                default:;
            }
            // Draw
            switch (pPipelineBindData->type) {
                case PIPELINE::DEFERRED_COMBINE: {
                    // TODO: this definitely only needs to be recorded once per swapchain creation!!!
                    handler().getScreenQuad()->draw(TYPE, pipelineBindDataList_.getValue(pPipelineBindData->type),
                                                    getDescSetBindDataMap(pPipelineBindData->type).begin()->second, priCmd,
                                                    frameIndex);
                } break;
                case PIPELINE::DEFERRED_SSAO: {
                    // This hasn't been tested in a long time. Might work to just let through.
                    // TODO: this definitely only needs to be recorded once per swapchain creation!!!
                    assert(doSSAO_ && false);
                } break;
                case PIPELINE::PARTICLE_FOUNTAIN_EULER_DEFERRED:
                case PIPELINE::PARTICLE_FOUNTAIN_DEFERRED: {
                    handler().particleHandler().recordDraw(TYPE, pPipelineBindData, priCmd, frameIndex);
                    vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
                } break;
                default: {
                    // MRT PASSES
                    auto& secCmd = data.secCmds[frameIndex];
                    pScene->record(TYPE, pPipelineBindData->type, pPipelineBindData, priCmd, secCmd, frameIndex);
                    vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
                } break;
            }
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
        0,                                 // flags VkAttachmentDescriptionFlags
        VK_FORMAT_UNDEFINED,               // format VkFormat
        getSamples(),                      // samples VkSampleCountFlagBits
        VK_ATTACHMENT_LOAD_OP_CLEAR,       // loadOp VkAttachmentLoadOp
        VK_ATTACHMENT_STORE_OP_STORE,      // storeOp VkAttachmentStoreOp
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,   // stencilLoadOp VkAttachmentLoadOp
        VK_ATTACHMENT_STORE_OP_DONT_CARE,  // stencilStoreOp VkAttachmentStoreOp
        // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,  // TRY THIS!!!
        VK_IMAGE_LAYOUT_UNDEFINED,                // initialLayout VkImageLayout
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL  // finalLayout VkImageLayout
    };

    // assert(textureIds_.size() == 3 && pTextures_.size() >= textureIds_.size() &&
    //       pTextures_.size() % textureIds_.size() == 0);

    // auto numTex = pTextures_.size() / textureIds_.size();

    inputAttachmentOffset_ = static_cast<uint32_t>(resources_.colorAttachments.size());

    // POSITION
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    auto pTexture = handler().textureHandler().getTexture(Texture::Deferred::POS_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back(
        {resources_.colorAttachments.back().attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

    // NORMAL
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::NORM_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back(
        {resources_.colorAttachments.back().attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

    // DIFFUSE
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::DIFFUSE_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back(
        {resources_.colorAttachments.back().attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

    // AMBIENT
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::AMBIENT_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back(
        {resources_.colorAttachments.back().attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

    // SPECULAR
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::SPECULAR_2D_ID);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back(
        {resources_.colorAttachments.back().attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

    inputAttachmentCount_ = static_cast<uint32_t>(resources_.colorAttachments.size()) - inputAttachmentOffset_;

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

    assert(inputAttachmentOffset_ == 1);  // Swapchain should be in 0

    uint32_t mrtPipelineCount = 0;
    for (const auto& [pipelineType, badData] : pipelineBindDataList_.getKeyOffsetMap()) {
        if (pipelineType == PIPELINE::PARTICLE_EULER_COMPUTE) continue;
        if (pipelineType == PIPELINE::DEFERRED_SSAO && !doSSAO_) continue;
        mrtPipelineCount++;
    }

    // MRT
    subpassDesc = {};
    subpassDesc.colorAttachmentCount = inputAttachmentCount_;
    subpassDesc.pColorAttachments = &resources_.colorAttachments[inputAttachmentOffset_];
    subpassDesc.pResolveAttachments = nullptr;
    subpassDesc.pDepthStencilAttachment = pipelineData_.usesDepth ? &resources_.depthStencilAttachment : nullptr;
    resources_.subpasses.assign(mrtPipelineCount, subpassDesc);

    // SSAO
    if (doSSAO_) {
        assert(resources_.colorAttachments.size() > (size_t)inputAttachmentCount_ + (size_t)inputAttachmentOffset_);
        subpassDesc = {};
        subpassDesc.colorAttachmentCount = 1;
        subpassDesc.pColorAttachments =
            &resources_.colorAttachments[(size_t)inputAttachmentOffset_ + (size_t)inputAttachmentCount_];
        subpassDesc.pResolveAttachments = nullptr;
        subpassDesc.pDepthStencilAttachment = nullptr;
        resources_.subpasses.push_back(subpassDesc);
    }

    if (usesMultiSample()) assert(resources_.resolveAttachments.size() == 1);

    // COMBINE
    subpassDesc = {};
    subpassDesc.inputAttachmentCount = static_cast<uint32_t>(resources_.inputAttachments.size());
    subpassDesc.pInputAttachments = resources_.inputAttachments.data();
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &resources_.colorAttachments[0];  // SWAPCHAIN
    subpassDesc.pResolveAttachments = resources_.resolveAttachments.data();
    subpassDesc.pDepthStencilAttachment = nullptr;
    resources_.subpasses.push_back(subpassDesc);

    assert(resources_.subpasses.size() == pipelineBindDataList_.size());
    combinePassIndex_ = static_cast<uint32_t>(pipelineBindDataList_.size() - 1);
}

void Base::createDependencies() {
    // TODO: There might need to be an external pass dependency for the shadow passes.

    VkSubpassDependency depthDependency = {
        0,  // srcSubpass
        0,  // dstSubpass
        // Both stages might have to access the depth-buffer
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,      // srcStageMask
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,      // dstStageMask
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,                                                // srcAccessMask
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,  // dstAccessMask
        VK_DEPENDENCY_BY_REGION_BIT,                                                                 // dependencyFlags
    };
    // VkSubpassDependency colorDependency = {
    //    0,                                              // srcSubpass
    //    0,                                              // dstSubpass
    //    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
    //    VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // dstStageMask
    //    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // srcAccessMask
    //    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // dstAccessMask
    //    VK_DEPENDENCY_BY_REGION_BIT,
    //};

    uint32_t subpass = 0;
    for (const auto& [pipelineType, badData] : pipelineBindDataList_.getKeyOffsetMap()) {
        if (pipelineType == PIPELINE::PARTICLE_EULER_COMPUTE) continue;
        if (pipelineType == PIPELINE::DEFERRED_SSAO && !doSSAO_) continue;
        if (pipelineType == PIPELINE::PARTICLE_FOUNTAIN_EULER_DEFERRED) {
            // Dispatch writes into a storage buffer. Draw consumes that buffer as an instance vertex buffer.
            resources_.dependencies.push_back({
                VK_SUBPASS_EXTERNAL,
                subpass,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
            });
        }
        depthDependency.srcSubpass = subpass;
        depthDependency.dstSubpass = subpass + 1;
        resources_.dependencies.push_back(depthDependency);
        subpass++;
    }

    if (doSSAO_) {
        assert(false);
    } else {
        // Color input attachment dependency
        VkSubpassDependency combineDependency = {
            0,                                              // srcSubpass
            0,                                              // dstSubpass
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,          // dstStageMask
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,           // srcAccessMask
            VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,            // dstAccessMask
            VK_DEPENDENCY_BY_REGION_BIT,                    // dependencyFlags
        };

        combineDependency.dstSubpass = static_cast<uint32_t>(resources_.subpasses.size() - 1);
        for (subpass = 0; subpass < combinePassIndex_; subpass++) {
            combineDependency.srcSubpass = subpass;
            resources_.dependencies.push_back(combineDependency);
        }
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
