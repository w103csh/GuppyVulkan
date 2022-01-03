/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "RenderPassDeferred.h"

#include "ComputeWorkManager.h"
#include "ConstantsAll.h"
#include "Deferred.h"
#include "RenderPassCubeMap.h"
#include "RenderPassManager.h"
#include "RenderPassShadow.h"
#if USE_VOLUMETRIC_LIGHTING
// ...
#endif
// HANDLERS
#include "ParticleHandler.h"
#include "PipelineHandler.h"
#include "DescriptorHandler.h"
#include "PassHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"

namespace RenderPass {
namespace Deferred {

// clang-format off
const CreateInfo DEFERRED_CREATE_INFO = {
    RENDER_PASS::DEFERRED,
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
        GRAPHICS::OCEAN_SURFACE_DEFERRED,
#if !(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
        GRAPHICS::OCEAN_WF_TESS_DEFERRED,
        GRAPHICS::OCEAN_SURFACE_TESS_DEFERRED,
#endif
        GRAPHICS::OCEAN_WF_CDLOD_DEFERRED,
        GRAPHICS::OCEAN_SURFACE_CDLOD_DEFERRED,
        GRAPHICS::CDLOD_TEX_DEFERRED,
        GRAPHICS::CDLOD_WF_DEFERRED,
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
    },
    (
        FLAG::SWAPCHAIN | FLAG::DEPTH | /*FLAG::DEPTH_INPUT_ATTACHMENT |*/
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
        RENDER_PASS::SHADOW_DEFAULT,
        RENDER_PASS::SHADOW_CUBE,
        RENDER_PASS::SKYBOX_NIGHT,
    },
    {
#if USE_VOLUMETRIC_LIGHTING
        // ...
#endif
    },
};
// clang-format on

Base::Base(Pass::Handler& handler, const index&& offset)
    : RenderPass::Base{handler, std::forward<const index>(offset), &DEFERRED_CREATE_INFO},
      inputAttachmentOffset_(0),
      inputAttachmentCount_(0),
      doSSAO_(false) {
    status_ = (STATUS::PENDING_MESH | STATUS::PENDING_PIPELINE);
}

// TODO: should something like this be on the base class????
void Base::init() {
    // Initialize the dependent pass types. Currently they all will come prior to the
    // main loop pass.
    for (const auto& [passType, offset] : dependentTypeOffsetPairs_) {
        // Boy is this going to be confusing if it ever doesn't work right.
        if (passType == TYPE) {
            RenderPass::Base::init();
            // Override the depth format here (I'm too lazy to do anything else atm) !!!
            depthFormat_ = vk::Format::eD32SfloatS8Uint;
        } else {
            const auto& pPass = handler().renderPassMgr().getPass(offset);
            if (!pPass->isIntialized()) const_cast<RenderPass::Base*>(pPass.get())->init();
        }
    }
}

uint32_t Base::getSubpassId(const PIPELINE& type) const {
    if (type == PIPELINE{GRAPHICS::DEFERRED_COMBINE}) return cmbSubpass_;
    return mrtSubpass_;
}

void Base::record(const uint8_t frameIndex) {
    if (getStatus() != STATUS::READY) update();
    if (getStatus() == STATUS::READY) {
        beginInfo_.framebuffer = data.framebuffers[frameIndex];
        auto& priCmd = data.priCmds[frameIndex];

        priCmd.reset({});

        vk::CommandBufferBeginInfo bufferInfo = {vk::CommandBufferUsageFlagBits::eOneTimeSubmit};
        priCmd.begin(bufferInfo);

        // COMPUTE
        for (const auto& pPipelineBindData : pipelineBindDataList_.getValues()) {
            if (std::visit(Pipeline::IsCompute{}, pPipelineBindData->type)) {
                handler().particleHandler().recordDispatch(TYPE, pPipelineBindData, priCmd, frameIndex);
            }
        }

        // SKYBOX
        if (true) {
            const auto& pPass = handler().renderPassMgr().getPass(dependentTypeOffsetPairs_[2].second);
            assert(pPass->TYPE == RENDER_PASS::SKYBOX_NIGHT);
            static_cast<RenderPass::CubeMap::SkyboxNight*>(pPass.get())->record(frameIndex, priCmd);
        }

        // SHADOW (DEFAULT)
        if (true) {
            const auto& pPass = handler().renderPassMgr().getPass(dependentTypeOffsetPairs_[0].second);
            assert(pPass->TYPE == RENDER_PASS::SHADOW_DEFAULT);
            std::vector<PIPELINE> pipelineTypes;
            pipelineTypes.reserve(pipelineBindDataList_.size());
            for (const auto& [pipelineType, value] : pipelineBindDataList_.getKeyOffsetMap())
                pipelineTypes.push_back(pipelineType);
            static_cast<Shadow::Cube*>(pPass.get())->record(frameIndex, TYPE, pipelineTypes, priCmd);
        }

        // SHADOW (CUBEMAP)
        if (true) {
            const auto& pPass = handler().renderPassMgr().getPass(dependentTypeOffsetPairs_[1].second);
            assert(pPass->TYPE == RENDER_PASS::SHADOW_CUBE);
            std::vector<PIPELINE> pipelineTypes;
            pipelineTypes.reserve(pipelineBindDataList_.size());
            for (const auto& [pipelineType, value] : pipelineBindDataList_.getKeyOffsetMap())
                pipelineTypes.push_back(pipelineType);
            static_cast<Shadow::Cube*>(pPass.get())->record(frameIndex, TYPE, pipelineTypes, priCmd);
        }

        beginPass(priCmd, frameIndex, vk::SubpassContents::eInline);

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
                        priCmd.pushConstants(pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                                             static_cast<uint32_t>(sizeof(::Deferred::PushConstant)), &pushConstant);
                    } break;
                    case GRAPHICS::DEFERRED_MRT_WF_COLOR: {
                        ::Deferred::PushConstant pushConstant = {::Deferred::PASS_FLAG::WIREFRAME};
                        priCmd.pushConstants(pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
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
                    case GRAPHICS::PRTCL_FOUNTAIN_DEFERRED: {
                        // PARTICLE GRAPHICS
                        handler().particleHandler().recordDraw(TYPE, pPipelineBindData, priCmd, frameIndex);
                    } break;
                    case GRAPHICS::OCEAN_WF_DEFERRED:
                    case GRAPHICS::OCEAN_SURFACE_DEFERRED:
#if !(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
                    case GRAPHICS::OCEAN_WF_TESS_DEFERRED:
                    case GRAPHICS::OCEAN_SURFACE_TESS_DEFERRED:
#endif
                    case GRAPHICS::OCEAN_WF_CDLOD_DEFERRED:
                    case GRAPHICS::OCEAN_SURFACE_CDLOD_DEFERRED:
                    case GRAPHICS::CDLOD_WF_DEFERRED:
                    case GRAPHICS::CDLOD_TEX_DEFERRED: {
                        // SCENE RENDERERS
                        handler().sceneHandler().recordRenderer(TYPE, pPipelineBindData, priCmd);
                    } break;
                    default: {
                        // MRT PASSES
                        auto& secCmd = data.secCmds[frameIndex];
                        pScene->record(TYPE, pPipelineBindData->type, pPipelineBindData, priCmd, secCmd, frameIndex);
                    } break;
                    case GRAPHICS::DEFERRED_COMBINE: {
                        priCmd.nextSubpass(vk::SubpassContents::eInline);
                        // TODO: this definitely only needs to be recorded once per swapchain creation!!!
                        handler().renderPassMgr().getScreenQuad()->draw(
                            TYPE, pipelineBindDataList_.getValue(pPipelineBindData->type),
                            getDescSetBindDataMap(pPipelineBindData->type).begin()->second, priCmd, frameIndex);
                    } break;
                }
            }
        }

        endPass(priCmd);

#if USE_VOLUMETRIC_LIGHTING  // Uses the depth target from the main deferred pass.
        // ...
#endif
    }
    // data.priCmds[frameIndex].end();
}

void Base::update(const std::vector<Descriptor::Base*> pDynamicItems) {
    // Check the mesh status.
    if (handler().renderPassMgr().getScreenQuad()->getStatus() == STATUS::READY) {
        status_ ^= STATUS::PENDING_MESH;
        RenderPass::Base::update(pDynamicItems);
    }
}

void Base::updateSubmitResource(SubmitResource& resource, const uint8_t frameIndex) const {
    handler().compWorkMgr().updateRenderPassSubmitResource(TYPE, resource, frameIndex);
    ::RenderPass::Base::updateSubmitResource(resource, frameIndex);
}

void Base::createAttachments() {
    // DEPTH/RESOLVE/SWAPCHAIN
    ::RenderPass::Base::createAttachments();

    vk::AttachmentDescription2 attachment = {};
    attachment.format = vk::Format::eUndefined;
    attachment.samples = getSamples();
    attachment.loadOp = vk::AttachmentLoadOp::eClear;
    attachment.storeOp = vk::AttachmentStoreOp::eStore;
    attachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachment.initialLayout = vk::ImageLayout::eUndefined;
    attachment.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    // assert(textureIds_.size() == 3 && pTextures_.size() >= textureIds_.size() &&
    //       pTextures_.size() % textureIds_.size() == 0);

    // auto numTex = pTextures_.size() / textureIds_.size();

    inputAttachmentOffset_ = static_cast<uint32_t>(resources_.colorAttachments.size());

    // POSITION
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), vk::ImageLayout::eColorAttachmentOptimal});
    resources_.attachments.push_back(attachment);
    auto pTexture = handler().textureHandler().getTexture(Texture::Deferred::POS_2D_ID);
    assert(pTexture != nullptr);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back({
        resources_.colorAttachments.back().attachment,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageAspectFlagBits::eColor,
    });

    // NORMAL
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), vk::ImageLayout::eColorAttachmentOptimal});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::NORM_2D_ID);
    assert(pTexture != nullptr);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back({
        resources_.colorAttachments.back().attachment,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageAspectFlagBits::eColor,
    });

    // DIFFUSE
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), vk::ImageLayout::eColorAttachmentOptimal});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::DIFFUSE_2D_ID);
    assert(pTexture != nullptr);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back({
        resources_.colorAttachments.back().attachment,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageAspectFlagBits::eColor,
    });

    // AMBIENT
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), vk::ImageLayout::eColorAttachmentOptimal});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::AMBIENT_2D_ID);
    assert(pTexture != nullptr);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back({
        resources_.colorAttachments.back().attachment,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageAspectFlagBits::eColor,
    });

    // SPECULAR
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), vk::ImageLayout::eColorAttachmentOptimal});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::SPECULAR_2D_ID);
    assert(pTexture != nullptr);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back({
        resources_.colorAttachments.back().attachment,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageAspectFlagBits::eColor,
    });

    // FLAGS
    resources_.colorAttachments.push_back(
        {static_cast<uint32_t>(resources_.attachments.size()), vk::ImageLayout::eColorAttachmentOptimal});
    resources_.attachments.push_back(attachment);
    pTexture = handler().textureHandler().getTexture(Texture::Deferred::FLAGS_2D_ID);
    assert(pTexture != nullptr);
    resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
    assert(resources_.attachments.back().samples == getSamples());
    resources_.inputAttachments.push_back({
        resources_.colorAttachments.back().attachment,
        vk::ImageLayout::eShaderReadOnlyOptimal,
        vk::ImageAspectFlagBits::eColor,
    });

    inputAttachmentCount_ = static_cast<uint32_t>(resources_.colorAttachments.size()) - inputAttachmentOffset_;

    // SSAO
    if (doSSAO_) {
        resources_.colorAttachments.push_back(
            {static_cast<uint32_t>(resources_.attachments.size()), vk::ImageLayout::eColorAttachmentOptimal});
        resources_.attachments.push_back(attachment);
        pTexture = handler().textureHandler().getTexture(Texture::Deferred::SSAO_2D_ID);
        assert(pTexture != nullptr);
        resources_.attachments.back().format = pTexture->samplers[0].imgCreateInfo.format;
        assert(resources_.colorAttachments.size() == 8);
    } else {
        assert(resources_.colorAttachments.size() == 7);
    }

#if USE_VOLUMETRIC_LIGHTING
    // ...
#endif
}

void Base::createSubpassDescriptions() {
    vk::SubpassDescription2 subpassDesc;

    assert(inputAttachmentOffset_ == 1);  // Swapchain should be in 0

    // MRT
    subpassDesc = vk::SubpassDescription2{};
    subpassDesc.colorAttachmentCount = inputAttachmentCount_;
    subpassDesc.pColorAttachments = &resources_.colorAttachments[inputAttachmentOffset_];
    subpassDesc.pResolveAttachments = nullptr;
    subpassDesc.pDepthStencilAttachment = pipelineData_.usesDepth ? &resources_.depthStencilAttachment : nullptr;
#if USE_VOLUMETRIC_LIGHTING
    // ...
#endif
    resources_.subpasses.push_back(subpassDesc);
    assert(mrtSubpass_ == (resources_.subpasses.size() - 1));

    // SSAO
    if (doSSAO_) {
        assert(false);  // I didn't update this logic when I fixed the number of subpasses/dependencies.
        // assert(resources_.colorAttachments.size() > (size_t)inputAttachmentCount_ + (size_t)inputAttachmentOffset_);
        // subpassDesc = vk::SubpassDescription2{};
        // subpassDesc.colorAttachmentCount = 1;
        // subpassDesc.pColorAttachments =
        //    &resources_.colorAttachments[(size_t)inputAttachmentOffset_ + (size_t)inputAttachmentCount_];
        // subpassDesc.pResolveAttachments = nullptr;
        // subpassDesc.pDepthStencilAttachment = nullptr;
        // resources_.subpasses.push_back(subpassDesc);
    }

    if (usesMultiSample()) assert(resources_.resolveAttachments.size() == 1);

    // COMBINE
    subpassDesc = vk::SubpassDescription2{};
    subpassDesc.inputAttachmentCount = static_cast<uint32_t>(resources_.inputAttachments.size());
    subpassDesc.pInputAttachments = resources_.inputAttachments.data();
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &resources_.colorAttachments[0];  // SWAPCHAIN
    subpassDesc.pResolveAttachments = resources_.resolveAttachments.data();
    subpassDesc.pDepthStencilAttachment = nullptr;
    resources_.subpasses.push_back(subpassDesc);
    assert(cmbSubpass_ == (resources_.subpasses.size() - 1));
}

void Base::createDependencies() {
    // MRT -> COMBINE
    vk::SubpassDependency2 dependency = {};
    dependency.srcSubpass = mrtSubpass_;
    dependency.dstSubpass = cmbSubpass_;
    dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    dependency.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    dependency.dstAccessMask = vk::AccessFlagBits::eInputAttachmentRead;
    dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;
    resources_.dependencies.push_back(dependency);
}

void Base::updateClearValues() {
    // Depth/Swapchain
    RenderPass::Base::updateClearValues();
    // Position
    clearValues_.push_back({});
    clearValues_.back().color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f};
    // Normal
    clearValues_.push_back({});
    clearValues_.back().color = std::array<float, 4>{0.0f, 0.0f, 0.0f, 0.0f};
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
    clearValues_.back().color = std::array<uint32_t, 4>{0u, 0u, 0u, 0u};
    // SSAO
    if (doSSAO_) {
        clearValues_.push_back({});
        clearValues_.back().color = DEFAULT_CLEAR_COLOR_VALUE;
    }
#if USE_VOLUMETRIC_LIGHTING
    // ...
#endif
}

void Base::createFramebuffers() {
    /* Views for framebuffer.
     *  - depth
     *  - position
     *  - normal
     *  - color
     */
    std::vector<std::vector<vk::ImageView>> attachmentViewsList(handler().shell().context().imageCount);
    data.framebuffers.resize(attachmentViewsList.size());

    vk::FramebufferCreateInfo createInfo = {};
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
            assert(depth_.view);
            attachmentViews.push_back(depth_.view);
        }

        // MULTI-SAMPLE
        if (usesMultiSample()) {
            assert(images_.size() == 1);
            assert(images_[0].view);
            attachmentViews.push_back(images_[0].view);  // TODO: should there be one per swapchain image????????????????
        }

        // SWAPCHAIN
        attachmentViews.push_back(handler().renderPassMgr().getSwapchainViews()[frameIndex]);
        assert(attachmentViews.back());

        // POSITION
        auto pTexture = handler().textureHandler().getTexture(Texture::Deferred::POS_2D_ID);
        attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
        assert(attachmentViews.back());

        // NORMAL
        pTexture = handler().textureHandler().getTexture(Texture::Deferred::NORM_2D_ID);
        attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
        assert(attachmentViews.back());

        // DIFFUSE
        pTexture = handler().textureHandler().getTexture(Texture::Deferred::DIFFUSE_2D_ID);
        attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
        assert(attachmentViews.back());

        // AMBIENT
        pTexture = handler().textureHandler().getTexture(Texture::Deferred::AMBIENT_2D_ID);
        attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
        assert(attachmentViews.back());

        // SPECULAR
        pTexture = handler().textureHandler().getTexture(Texture::Deferred::SPECULAR_2D_ID);
        attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
        assert(attachmentViews.back());

        // FLAGS
        pTexture = handler().textureHandler().getTexture(Texture::Deferred::FLAGS_2D_ID);
        attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
        assert(attachmentViews.back());

        // SSAO
        if (doSSAO_) {
            pTexture = handler().textureHandler().getTexture(Texture::Deferred::SSAO_2D_ID);
            attachmentViews.push_back(pTexture->samplers[0].layerResourceMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL).view);
            assert(attachmentViews.back());
        }

#if USE_VOLUMETRIC_LIGHTING
        // ...
#endif

        createInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
        createInfo.pAttachments = attachmentViews.data();

        data.framebuffers[frameIndex] =
            handler().shell().context().dev.createFramebuffer(createInfo, handler().shell().context().pAllocator);
    }
}

}  // namespace Deferred
}  // namespace RenderPass
