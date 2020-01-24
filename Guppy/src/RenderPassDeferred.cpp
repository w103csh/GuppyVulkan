/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "RenderPassDeferred.h"

#include "ConstantsAll.h"
#include "Deferred.h"
#include "RenderPassCubeMap.h"
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
        GRAPHICS::DEFERRED_MRT_PT,
// TODO: make tess/geom pipeline/mesh optional based on the context flags.
#ifndef VK_USE_PLATFORM_MACOS_MVK
        GRAPHICS::TESS_BEZIER_4_DEFERRED,
#endif
        GRAPHICS::DEFERRED_MRT_LINE,
        GRAPHICS::DEFERRED_MRT_COLOR,
        GRAPHICS::DEFERRED_MRT_COLOR_RFL_RFR,
        GRAPHICS::HFF_CLMN_DEFERRED,
        GRAPHICS::HFF_WF_DEFERRED,
        GRAPHICS::HFF_OCEAN_DEFERRED,
        GRAPHICS::OCEAN_WF_DEFERRED,
#ifndef VK_USE_PLATFORM_MACOS_MVK
        GRAPHICS::DEFERRED_MRT_WF_COLOR,
#endif
        GRAPHICS::PRTCL_WAVE_DEFERRED,
        GRAPHICS::PRTCL_FOUNTAIN_DEFERRED,
#ifndef VK_USE_PLATFORM_MACOS_MVK
        // GRAPHICS::GEOMETRY_SILHOUETTE_DEFERRED,
        GRAPHICS::TESS_PHONG_TRI_COLOR_DEFERRED,
        GRAPHICS::TESS_PHONG_TRI_COLOR_WF_DEFERRED,
#endif
        GRAPHICS::DEFERRED_MRT_TEX,
        GRAPHICS::DEFERRED_MRT_SKYBOX,
        GRAPHICS::PRTCL_FOUNTAIN_EULER_DEFERRED,
        GRAPHICS::PRTCL_ATTR_PT_DEFERRED,
        GRAPHICS::PRTCL_CLOTH_DEFERRED,
        // GRAPHICS::DEFERRED_SSAO,
        GRAPHICS::DEFERRED_COMBINE,
        /**
         * These compute passes have sister graphics passes earlier in the list. There should be some kind of validation, or
         * potentially a data structure change so that this is more explicit. Also, its kind of misleading that these are at
         * the end of the list when they will always be executed first, but the subpass dependency code is easier to debug
         * with the indices being accurate for the graphics pass order.
         */
        COMPUTE::PRTCL_EULER,
        COMPUTE::PRTCL_ATTR,
        COMPUTE::PRTCL_CLOTH,
        COMPUTE::PRTCL_CLOTH_NORM,
        COMPUTE::HFF_HGHT,
        COMPUTE::HFF_NORM,
        COMPUTE::OCEAN_DISP,
        COMPUTE::OCEAN_FFT,
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
    {
        PASS::SHADOW,
        PASS::SKYBOX_NIGHT,
    },
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
        for (const auto& pPipelineBindData : pipelineBindDataList_.getValues()) {
            if (std::visit(Pipeline::IsCompute{}, pPipelineBindData->type)) {
                handler().particleHandler().recordDispatch(TYPE, pPipelineBindData, priCmd, frameIndex);
            }
        }

        // SKYBOX
        if (true) {
            const auto& pPass = handler().getPass(dependentTypeOffsetPairs_[1].second);
            assert(pPass->TYPE == PASS::SKYBOX_NIGHT);
            static_cast<RenderPass::CubeMap::SkyboxNight*>(pPass.get())->record(frameIndex, priCmd);
        }

        // SHADOW
        if (true) {
            const auto& pPass = handler().getPass(dependentTypeOffsetPairs_[0].second);
            assert(pPass->TYPE == PASS::SHADOW);
            std::vector<PIPELINE> pipelineTypes;
            pipelineTypes.reserve(pipelineBindDataList_.size());
            for (const auto& [pipelineType, value] : pipelineBindDataList_.getKeyOffsetMap())
                pipelineTypes.push_back(pipelineType);
            static_cast<Shadow::Default*>(pPass.get())->record(frameIndex, TYPE, pipelineTypes, priCmd);
        }

        beginPass(priCmd, frameIndex, VK_SUBPASS_CONTENTS_INLINE);

        auto& pScene = handler().sceneHandler().getActiveScene();

        for (const auto& pPipelineBindData : pipelineBindDataList_.getValues()) {
            if (std::visit(Pipeline::IsGraphics{}, pPipelineBindData->type)) {
                auto graphicsType = std::visit(Pipeline::GetGraphics{}, pPipelineBindData->type);
                // Push constant
                switch (graphicsType) {
                    case GRAPHICS::GEOMETRY_SILHOUETTE_DEFERRED:
                    case GRAPHICS::TESS_BEZIER_4_DEFERRED:
                    case GRAPHICS::TESS_PHONG_TRI_COLOR_DEFERRED:
                    case GRAPHICS::TESS_PHONG_TRI_COLOR_WF_DEFERRED:
                    case GRAPHICS::DEFERRED_MRT_COLOR:
                    case GRAPHICS::DEFERRED_MRT_PT:
                    case GRAPHICS::DEFERRED_MRT_LINE: {
                        ::Deferred::PushConstant pushConstant = {::Deferred::PASS_FLAG::NONE};
                        vkCmdPushConstants(priCmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                                           static_cast<uint32_t>(sizeof(::Deferred::PushConstant)), &pushConstant);
                    } break;
                    case GRAPHICS::DEFERRED_MRT_WF_COLOR: {
                        ::Deferred::PushConstant pushConstant = {::Deferred::PASS_FLAG::WIREFRAME};
                        vkCmdPushConstants(priCmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                                           static_cast<uint32_t>(sizeof(::Deferred::PushConstant)), &pushConstant);
                    } break;
                    default:;
                }
                // Draw
                switch (graphicsType) {
                    case GRAPHICS::DEFERRED_SSAO: {
                        // This hasn't been tested in a long time. Might work to just let through.
                        // TODO: this definitely only needs to be recorded once per swapchain creation!!!
                        assert(doSSAO_ && false);
                    } break;
                    case GRAPHICS::PRTCL_FOUNTAIN_EULER_DEFERRED:
                    case GRAPHICS::PRTCL_ATTR_PT_DEFERRED:
                    case GRAPHICS::PRTCL_CLOTH_DEFERRED:
                    case GRAPHICS::HFF_CLMN_DEFERRED:
                    case GRAPHICS::HFF_WF_DEFERRED:
                    case GRAPHICS::HFF_OCEAN_DEFERRED:
                    case GRAPHICS::OCEAN_WF_DEFERRED:
                    case GRAPHICS::PRTCL_FOUNTAIN_DEFERRED: {
                        // PARTICLE GRAPHICS
                        handler().particleHandler().recordDraw(TYPE, pPipelineBindData, priCmd, frameIndex);
                        vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
                    } break;
                    default: {
                        // MRT PASSES
                        auto& secCmd = data.secCmds[frameIndex];
                        pScene->record(TYPE, pPipelineBindData->type, pPipelineBindData, priCmd, secCmd, frameIndex);
                        vkCmdNextSubpass(priCmd, VK_SUBPASS_CONTENTS_INLINE);
                    } break;
                    case GRAPHICS::DEFERRED_COMBINE: {
                        // TODO: this definitely only needs to be recorded once per swapchain creation!!!
                        handler().getScreenQuad()->draw(TYPE, pipelineBindDataList_.getValue(pPipelineBindData->type),
                                                        getDescSetBindDataMap(pPipelineBindData->type).begin()->second,
                                                        priCmd, frameIndex);
                    } break;
                }
            }
        }

        endPass(priCmd);
    }
    // vk::assert_success(vkEndCommandBuffer(data.priCmds[frameIndex]));
}

void Base::update(const std::vector<Descriptor::Base*> pDynamicItems) {
    // Check the mesh status.
    if (handler().getScreenQuad()->getStatus() == STATUS::READY) {
        status_ ^= STATUS::PENDING_MESH;
        RenderPass::Base::update(pDynamicItems);
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
    assert(pTexture != nullptr);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back(
        {resources_.colorAttachments.back().attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

    // NORMAL
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::NORM_2D_ID);
    assert(pTexture != nullptr);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back(
        {resources_.colorAttachments.back().attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

    // DIFFUSE
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::DIFFUSE_2D_ID);
    assert(pTexture != nullptr);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back(
        {resources_.colorAttachments.back().attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

    // AMBIENT
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::AMBIENT_2D_ID);
    assert(pTexture != nullptr);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back(
        {resources_.colorAttachments.back().attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

    // SPECULAR
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::SPECULAR_2D_ID);
    assert(pTexture != nullptr);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back(
        {resources_.colorAttachments.back().attachment, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL});

    // FLAGS
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::FLAGS_2D_ID);
    assert(pTexture != nullptr);
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
        assert(pTexture != nullptr);
        resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
        assert(resources_.colorAttachments.size() == 8);
    } else {
        assert(resources_.colorAttachments.size() == 7);
    }
}

void Base::createSubpassDescriptions() {
    VkSubpassDescription subpassDesc;

    assert(inputAttachmentOffset_ == 1);  // Swapchain should be in 0

    uint32_t mrtPipelineCount = 0;
    uint32_t compPipelineCount = 0;
    for (const auto& pipelineType : pipelineBindDataList_.getKeys()) {
        if (std::visit(Pipeline::IsCompute{}, pipelineType))
            compPipelineCount++;
        else if (pipelineType == PIPELINE{GRAPHICS::DEFERRED_SSAO} && !doSSAO_)
            continue;
        else if (pipelineType == PIPELINE{GRAPHICS::DEFERRED_COMBINE})
            continue;
        else
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

    assert(resources_.subpasses.size() == pipelineBindDataList_.size() - compPipelineCount);
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

    uint32_t subpass = 1;
    for (uint32_t offset = subpass; offset < pipelineBindDataList_.size(); offset++) {
        if (offset == 0) continue;
        const auto& pipelineType = pipelineBindDataList_.getKey(subpass);
        if (std::visit(Pipeline::IsCompute{}, pipelineType)) continue;
        if (pipelineType == PIPELINE{GRAPHICS::DEFERRED_SSAO} && !doSSAO_) continue;
        // TODO: How could this know which external subpass to wait on?
        if (pipelineType == PIPELINE{GRAPHICS::PRTCL_FOUNTAIN_EULER_DEFERRED} ||
            pipelineType == PIPELINE{GRAPHICS::PRTCL_ATTR_PT_DEFERRED} ||
            pipelineType == PIPELINE{GRAPHICS::PRTCL_CLOTH_DEFERRED} ||
            pipelineType == PIPELINE{GRAPHICS::HFF_OCEAN_DEFERRED}) {
            // Dispatch writes into a storage buffer. Draw consumes that buffer as a vertex buffer.
            resources_.dependencies.push_back({
                VK_SUBPASS_EXTERNAL,
                subpass,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
            });
        }
        if (pipelineType == PIPELINE{GRAPHICS::HFF_CLMN_DEFERRED} || pipelineType == PIPELINE{GRAPHICS::HFF_WF_DEFERRED} ||
            pipelineType == PIPELINE{GRAPHICS::OCEAN_WF_DEFERRED}) {
            // Dispatch writes into a storage buffer. Draw consumes that buffer as a shader object.
            resources_.dependencies.push_back({
                VK_SUBPASS_EXTERNAL,
                subpass,
                VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                VK_ACCESS_SHADER_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
            });
        }
        if (pipelineType == PIPELINE{GRAPHICS::DEFERRED_MRT_COLOR_RFL_RFR} ||
            pipelineType == PIPELINE{GRAPHICS::DEFERRED_MRT_SKYBOX}) {
            // Dispatch writes into a storage buffer. Draw consumes that buffer as a shader object.
            resources_.dependencies.push_back({
                VK_SUBPASS_EXTERNAL,
                subpass,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                VK_ACCESS_SHADER_READ_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
            });
        }
        depthDependency.srcSubpass = subpass - 1;
        depthDependency.dstSubpass = subpass;
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

        combineDependency.dstSubpass = pipelineBindDataList_.getOffset(PIPELINE{GRAPHICS::DEFERRED_COMBINE});
        for (subpass = 0; subpass < pipelineBindDataList_.size(); subpass++) {
            const auto& pipelineType = pipelineBindDataList_.getKey(subpass);
            if (std::visit(Pipeline::IsCompute{}, pipelineType)) continue;
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
    clearValues_.back().color = VkClearColorValue{0.0f, 0.0f, 0.0f, 0.0f};
    // Normal
    clearValues_.push_back({});
    clearValues_.back().color = VkClearColorValue{0.0f, 0.0f, 0.0f, 0.0f};
    // Diffuse
    clearValues_.push_back({});
    clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
    // Ambient
    clearValues_.push_back({});
    clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
    // Specular
    clearValues_.push_back({});
    clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
    // Flags
    clearValues_.push_back({});
    clearValues_.back().color = VkClearColorValue{0u, 0u, 0u, 0u};
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

        // FLAGS
        pTexture = handler().textureHandler().getTexture(Texture::Deferred::FLAGS_2D_ID);
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
