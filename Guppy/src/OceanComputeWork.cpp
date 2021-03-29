/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "OceanComputeWork.h"

#include "FFT.h"
#include "Ocean.h"
#include "RenderPassManager.h"
// HANLDERS
#include "PassHandler.h"
#include "ParticleHandler.h"
#include "TextureHandler.h"

// SHADER
namespace Shader {
namespace Ocean {
const CreateInfo DISP_COMP_CREATE_INFO = {
    SHADER::OCEAN_DISP_COMP,
    "Ocean Sufrace Dispersion Compute Shader",
    "comp.ocean.dispersion.glsl",
    vk::ShaderStageFlagBits::eCompute,
};
const CreateInfo FFT_COMP_CREATE_INFO = {
    SHADER::OCEAN_FFT_COMP,
    "Ocean Surface Fast Fourier Transform Compute Shader",
    "comp.ocean.fft.glsl",
    vk::ShaderStageFlagBits::eCompute,
};
}  // namespace Ocean
}  // namespace Shader

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
const CreateInfo OCEAN_DISPATCH_CREATE_INFO = {
    DESCRIPTOR_SET::OCEAN_DISPATCH,
    "_DS_OCEAN",
    {
        {{0, 0}, {UNIFORM_DYNAMIC::OCEAN}},
        {{1, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Ocean::WAVE_FOURIER_ID}},
        {{2, 0}, {STORAGE_IMAGE::PIPELINE, Texture::Ocean::DISP_REL_ID}},
        {{3, 0}, {UNIFORM_TEXEL_BUFFER::PIPELINE, BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_N_ID}},
        {{4, 0}, {UNIFORM_TEXEL_BUFFER::PIPELINE, BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_M_ID}},
        {{5, 0}, {UNIFORM_TEXEL_BUFFER::PIPELINE, BufferView::Ocean::FFT_TWIDDLE_FACTORS_ID}},
    },
};
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {

namespace Ocean {

// DISPERSION (COMPUTE)
const CreateInfo DISP_CREATE_INFO = {
    COMPUTE::OCEAN_DISP,
    "Ocean Surface Dispersion Compute Pipeline",
    {SHADER::OCEAN_DISP_COMP},
    {{DESCRIPTOR_SET::OCEAN_DISPATCH, vk::ShaderStageFlagBits::eCompute}},
    {},
    {},
    {::Ocean::DISP_LOCAL_SIZE, ::Ocean::DISP_LOCAL_SIZE, 1},
};
Dispersion::Dispersion(Handler& handler)
    : Compute(handler, &DISP_CREATE_INFO), omega0_(2.0f * glm::pi<float>() / ::Ocean::T) {}

void Dispersion::getShaderStageInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.specializationMapEntries.push_back({{}});

    // Use specialization constants to pass number of samples to the shader (used for MSAA resolve)
    createInfoRes.specializationMapEntries.back().back().constantID = 0;
    createInfoRes.specializationMapEntries.back().back().offset = 0;
    createInfoRes.specializationMapEntries.back().back().size = sizeof(omega0_);

    createInfoRes.specializationInfo.push_back({});
    createInfoRes.specializationInfo.back().mapEntryCount =
        static_cast<uint32_t>(createInfoRes.specializationMapEntries.back().size());
    createInfoRes.specializationInfo.back().pMapEntries = createInfoRes.specializationMapEntries.back().data();
    createInfoRes.specializationInfo.back().dataSize = sizeof(omega0_);
    createInfoRes.specializationInfo.back().pData = &omega0_;

    assert(createInfoRes.shaderStageInfos.size() == 1 &&
           createInfoRes.shaderStageInfos[0].stage == vk::ShaderStageFlagBits::eCompute);
    // Add the specialization to the shader info.
    createInfoRes.shaderStageInfos[0].pSpecializationInfo = &createInfoRes.specializationInfo.back();
}

// FFT (COMPUTE)
const CreateInfo FFT_CREATE_INFO = {
    COMPUTE::OCEAN_FFT,
    "Ocean Surface FFT Compute Pipeline",
    {SHADER::OCEAN_FFT_COMP},
    {{DESCRIPTOR_SET::OCEAN_DISPATCH, vk::ShaderStageFlagBits::eCompute}},
    {},
    {PUSH_CONSTANT::FFT_ROW_COL_OFFSET},
    {::Ocean::FFT_LOCAL_SIZE, 1, 1},
};
FFT::FFT(Handler& handler) : Compute(handler, &FFT_CREATE_INFO) {}

}  // namespace Ocean

}  // namespace Pipeline

namespace ComputeWork {

const CreateInfo CREATE_INFO = {
    COMPUTE_WORK::OCEAN,
    "Ocean Surface Simulation Compute Pass",
    {COMPUTE::OCEAN_DISP, COMPUTE::OCEAN_FFT},
};

Ocean::Ocean(Pass::Handler& handler, const index&& offset)
    : Base(handler, std::forward<const index>(offset), &CREATE_INFO),
      pRenderSignalSemaphores_(nullptr),
      pDispRelTex_(nullptr),
      pDispRelTexCopies_{nullptr, nullptr, nullptr} {}

void Ocean::prepare() {
    if (status_ != STATUS::READY) {
        const auto& ctx = handler().shell().context();

        // Store pointers to the dispersion relationship textures for convenience/speed.
        pDispRelTex_ = handler().textureHandler().getTexture(Texture::Ocean::DISP_REL_ID).get();
        assert(pDispRelTex_ != nullptr);
        assert(ctx.imageCount == 3);  // Potential image count problem.
        for (uint32_t i = 0; i < ctx.imageCount; i++) {
            pDispRelTexCopies_[i] = handler().textureHandler().getTexture(Texture::Ocean::DISP_REL_COPY_ID, i).get();
            assert(pDispRelTexCopies_[i] != nullptr);
        }

        // Store a pointer to the RENDER_PASS::DEFERRED signal semapores.
        pRenderSignalSemaphores_ = &handler().renderPassMgr().getPass(RENDER_PASS::DEFERRED)->data.semaphores;

        // Set the descriptor set bind data. This function should be called on first tick at earliest.
        assert(getDescSetBindDataMaps().empty());
        setDescSetBindData();

        status_ = STATUS::READY;
    }
}

void Ocean::record() {
    const auto& ctx = handler().shell().context();
    const auto frameIndex = handler().renderPassMgr().getFrameIndex();
    const auto frameCount = handler().game().getFrameCount();

    const auto& fence = resources.fences[frameIndex];
    const auto& cmd = resources.cmds[frameIndex];

    assert(status_ == STATUS::READY);

    {  // Prepare submit resources
        // TODO: I think that the update to the simulation time should be done after this wait. I believe this is why there
        // are micro-stutters occaisionally.
        vk::Result result = ctx.dev.waitForFences(fence, VK_TRUE, UINT64_MAX);
        assert(result == vk::Result::eSuccess);
        ctx.dev.resetFences({fence});
        cmd.begin(vk::CommandBufferBeginInfo{});
    }

    {  // Record command buffers
        const auto& pipelineBindDataList = getPipelineBindDataList();
        assert(pipelineBindDataList.size() == 2);  // OCEAN_DISP/OCEAN_FFT

        // Dispatches
        dispatch(TYPE, pipelineBindDataList.getValue(0), getDescSetBindData(TYPE, 0), cmd, frameIndex);  // OCEAN_DISP
        dispatch(TYPE, pipelineBindDataList.getValue(1), getDescSetBindData(TYPE, 1), cmd, frameIndex);  // OCEAN_FFT

        // Copy the dispersion relationship data image
        copyDispRelImg(cmd, frameIndex);
    }

    {  // Finalize submit resources
        cmd.end();
        resources.submit.commandBuffers[0] = cmd;
        resources.submit.signalSemaphores[0] = resources.semaphores[frameIndex];
        // TODO: There should be a better way to check if there is a semaphore that needs to be waited on. This is going to
        // break as soon as the ocean surface needs to be paused. I'll cross that bridge when I get there...
        if (frameCount > 2) {
            resources.submit.waitSemaphores.clear();
            const auto renderIndex = ((frameIndex + 1) % 3);
            resources.submit.waitSemaphores.push_back((*pRenderSignalSemaphores_)[renderIndex]);
            resources.submit.waitDstStageMask = vk::PipelineStageFlagBits::eTransfer;
        }
        resources.submit.fence = fence;
    }
}

const std::vector<Descriptor::Base*> Ocean::getDynamicDataItems(const PIPELINE pipelineType) const {
    /* TODO: The particle handler owns the only dynamic item we need. The ownership should probably move to a better
     * location - especially if there is more than one is ever created (which is why its dynamic in the first place...).
     * Also, doesn't this need to have memory barriers protecting it? Maybe this is the source of the micro-stutters. Really
     * there should be a separation of dispatch/draw data, so that there are no memory barriers required.
     */
    return {handler().particleHandler().ocnMgr.pItems.at(0).get()};
}

void Ocean::dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                     const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                     const uint8_t frameIndex) {
    const auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                           static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                           descSetBindData.descriptorSets[setIndex].data(),
                           static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                           descSetBindData.dynamicOffsets.data());

    switch (std::visit(Pipeline::GetCompute{}, pPipelineBindData->type)) {
        case COMPUTE::OCEAN_DISP: {
            cmd.dispatch(::Ocean::DISP_WORKGROUP_SIZE, ::Ocean::DISP_WORKGROUP_SIZE, 1);
        } break;
        case COMPUTE::OCEAN_FFT: {
            FFT::RowColumnOffset offset = 1;  // row
            cmd.pushConstants(pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                              static_cast<uint32_t>(sizeof(FFT::RowColumnOffset)), &offset);

            // Barrier for dispersion
            vk::MemoryBarrier barrier = {vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead};
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {},
                                {barrier}, {}, {});

            cmd.dispatch(::Ocean::FFT_WORKGROUP_SIZE, 1, 1);

            offset = 0;  // column
            cmd.pushConstants(pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                              static_cast<uint32_t>(sizeof(FFT::RowColumnOffset)), &offset);

            // Barrier for first fft pass
            barrier = {vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead};
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {},
                                {barrier}, {}, {});

            cmd.dispatch(::Ocean::FFT_WORKGROUP_SIZE, 1, 1);
        } break;
        default: {
            assert(false);
        } break;
    }
}

void Ocean::init() {
    const auto& ctx = handler().shell().context();
    // RESOURCES
    createCommandBuffers(ctx.imageCount);
    createSemaphores(ctx.imageCount);
    createFences(ctx.imageCount);
    // The following submit resources are always the same so set the sizes.
    resources.submit.commandBuffers.resize(1);
    resources.submit.signalSemaphores.resize(1);
}

void Ocean::copyDispRelImg(const vk::CommandBuffer cmd, const uint8_t frameIndex) {
    const auto& ctx = handler().shell().context();

    /* Copy the current dispersion relation data to an image for the next frame. This means the ocean surface draws
     * the data calculated during the previous frame (using the indices this way just makes reusing the previous code
     * easier).
     */
    const auto copyFrameIndex = ((frameIndex + 1) % ctx.imageCount);
    const auto& srcSampler = pDispRelTex_->samplers[0];
    const auto& dstSampler = pDispRelTexCopies_[copyFrameIndex]->samplers[0];

    // Set values that don't change first.
    vk::ImageMemoryBarrier imgBarrier = {};
    imgBarrier.image = dstSampler.image;
    imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imgBarrier.subresourceRange.layerCount = dstSampler.imgCreateInfo.arrayLayers;
    imgBarrier.subresourceRange.levelCount = 1;

    {  // Barrier for second fft pass, and image transition for the copy.
        vk::MemoryBarrier memBarrier = {vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eTransferRead};

        imgBarrier.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imgBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
        imgBarrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
        imgBarrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eTransfer, {},
                            {memBarrier}, {}, {imgBarrier});
    }

    {  // Copy the image data.
        vk::ImageCopy region = {};
        region.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.srcSubresource.layerCount = srcSampler.imgCreateInfo.arrayLayers;
        region.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        region.dstSubresource.layerCount = dstSampler.imgCreateInfo.arrayLayers;
        region.extent = srcSampler.imgCreateInfo.extent;

        cmd.copyImage(srcSampler.image, vk::ImageLayout::eGeneral, dstSampler.image, vk::ImageLayout::eTransferDstOptimal,
                      {region});
    }

    {  // Image transition out from transfer. Not sure what the dst settings should be...
        imgBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        imgBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        imgBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        imgBarrier.dstAccessMask = {};

        cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eBottomOfPipe, {}, {}, {},
                            {imgBarrier});
    }
}

}  // namespace ComputeWork