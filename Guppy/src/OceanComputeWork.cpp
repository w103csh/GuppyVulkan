/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "OceanComputeWork.h"

#include "Descriptor.h"
#include "FFT.h"
#include "Ocean.h"
#include "RenderPassManager.h"
// HANLDERS
#include "ParticleHandler.h"
#include "PassHandler.h"
#include "SceneHandler.h"
#include "TextureHandler.h"
#include "UniformHandler.h"

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
const CreateInfo VERT_INPUT_COMP_CREATE_INFO = {
    SHADER::OCEAN_VERT_INPUT_COMP,
    "Ocean Surface Vertex Input Compute Shader",
    "comp.ocean.vertInput.glsl",
    vk::ShaderStageFlagBits::eCompute,
};
}  // namespace Ocean
}  // namespace Shader

// UNIFORM DYNAMIC
namespace UniformDynamic {
namespace Ocean {
namespace SimulationDispatch {
Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM_DYNAMIC::OCEAN_DISPATCH),
      Buffer::DataItem<DATA>(pData),
      internalTime_(0.0f) {
    assert(helpers::isPowerOfTwo(pCreateInfo->info.N) && helpers::isPowerOfTwo(pCreateInfo->info.M));
    pData_->data0[0] = pCreateInfo->info.lambda;                                          // lambda
    pData_->data0[1] = 0.0f;                                                              // time
    pData_->data0[2] = (pCreateInfo->info.Lx / static_cast<float>(pCreateInfo->info.N));  // Lx scale
    pData_->data0[3] = (pCreateInfo->info.Lz / static_cast<float>(pCreateInfo->info.M));  // Lz scale
    pData_->data1[0] = static_cast<uint32_t>(log2(pCreateInfo->info.N));                  // log2(N)
    pData_->data1[1] = static_cast<uint32_t>(log2(pCreateInfo->info.M));                  // log2(M)
    dirty = true;
}
void Base::update(const float elapsedTime) {
    internalTime_ += elapsedTime;
    pData_->data0[1] = internalTime_;
    dirty = true;
}
}  // namespace SimulationDispatch
}  // namespace Ocean
}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
const CreateInfo OCEAN_DISPATCH_CREATE_INFO = {
    DESCRIPTOR_SET::OCEAN_DISPATCH,
    "_DS_OCEAN",
    {
        {{0, 0}, {UNIFORM_DYNAMIC::OCEAN_DISPATCH}},
        {{1, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Ocean::WAVE_FOURIER_ID}},
        {{2, 0}, {STORAGE_IMAGE::PIPELINE, Texture::Ocean::DISP_REL_ID}},
        {{3, 0}, {UNIFORM_TEXEL_BUFFER::PIPELINE, BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_N_ID}},
        {{4, 0}, {UNIFORM_TEXEL_BUFFER::PIPELINE, BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_M_ID}},
        {{5, 0}, {UNIFORM_TEXEL_BUFFER::PIPELINE, BufferView::Ocean::FFT_TWIDDLE_FACTORS_ID}},
        {{6, 0}, {STORAGE_IMAGE::PIPELINE, Texture::Ocean::VERT_INPUT_ID}},
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

// VERTEX INPUT (COMPUTE)
const CreateInfo VERTEX_INPUT_CREATE_INFO = {
    COMPUTE::OCEAN_VERT_INPUT,
    "Ocean Surface Vertex Input Compute Pipeline",
    {SHADER::OCEAN_VERT_INPUT_COMP},
    {{DESCRIPTOR_SET::OCEAN_DISPATCH, vk::ShaderStageFlagBits::eCompute}},
    {},
    {},
    {::Ocean::DISP_LOCAL_SIZE, ::Ocean::DISP_LOCAL_SIZE, 1},
};
VertexInput::VertexInput(Handler& handler) : Compute(handler, &VERTEX_INPUT_CREATE_INFO) {}

}  // namespace Ocean

}  // namespace Pipeline

namespace ComputeWork {

const CreateInfo CREATE_INFO = {
    COMPUTE_WORK::OCEAN,
    "Ocean Surface Simulation Compute Work",
    {
        COMPUTE::OCEAN_DISP,
        COMPUTE::OCEAN_FFT,
        COMPUTE::OCEAN_VERT_INPUT,
    },
};

Ocean::Ocean(Pass::Handler& handler, const index&& offset)
    : Base(handler, std::forward<const index>(offset), &CREATE_INFO),
      startFrameCount_(UINT64_MAX),
      pauseFrameCount_(UINT64_MAX),
      pGraphicsWork_(nullptr),
      pOcnSimDpch_(nullptr),
      pVertInputTex_(nullptr),
      pVertInputTexCopies_{nullptr, nullptr, nullptr} {}

const std::vector<Descriptor::Base*> Ocean::getDynamicDataItems(const PIPELINE pipelineType) const {
    if (pOcnSimDpch_ == nullptr) {
        // This is created in the particle handler currently... Fix this and the const_cast if you ever see a
        // good opportunity.
        assert(handler().uniformHandler().ocnSimDpchMgr().pItems.size());
        const_cast<UniformDynamic::Ocean::SimulationDispatch::Base*>(pOcnSimDpch_) =
            &handler().uniformHandler().ocnSimDpchMgr().getTypedItem(0);
    }
    return {pOcnSimDpch_};
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
        case COMPUTE::OCEAN_VERT_INPUT: {
            // Barrier for second fft pass
            vk::MemoryBarrier barrier = {vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eShaderRead};
            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {},
                                {barrier}, {}, {});

            cmd.dispatch(::Ocean::DISP_WORKGROUP_SIZE, ::Ocean::DISP_WORKGROUP_SIZE, 1);
        } break;
        default: {
            assert(false);
        } break;
    }
}

void Ocean::updateRenderPassSubmitResource(RenderPass::SubmitResource& resource, const uint8_t frameIndex) const {
    if (status_ == STATUS::READY) {
        const auto& ctx = handler().shell().context();
        const auto frameCount = handler().game().getFrameCount();

        const bool notPausedNotFirstFrame = (!getPaused() && (startFrameCount_ != frameCount));

        // When the simulation starts draw needs wait after the first frame because it lags a single frame behind. Also, when
        // the simulation is paused we need to keep waiting until all copies are done.
        if (notPausedNotFirstFrame || (getPaused() && ((frameCount - pauseFrameCount_) < ctx.imageCount))) {
            const auto workIndex = ((frameIndex + handler().shell().context().imageCount - 1) % 3);
            resource.waitSemaphores[resource.waitSemaphoreCount] = resources.semaphores[workIndex];
            resource.waitDstStageMasks[resource.waitSemaphoreCount] = vk::PipelineStageFlagBits::eVertexShader;
            resource.waitSemaphoreCount++;
        }

        // If not the first frame (surface doesn't draw on first frame), and the simulation is not paused then add a signal
        // semaphore to indicate the heightmap copy is not in use.
        if (notPausedNotFirstFrame) {
            resource.signalSemaphores[resource.signalSemaphoreCount] = resources.drawSemaphores[frameIndex];
            resource.signalSemaphoreCount++;
        }
    }
}

void Ocean::copyImage(const vk::CommandBuffer cmd, const uint8_t frameIndex) {
    const auto& ctx = handler().shell().context();

    /* Copy the current dispersion relation data to an image for the next frame. This means the ocean surface draws
     * the data calculated during the previous frame (using the indices this way just makes reusing the previous code
     * easier).
     */
    const auto copyFrameIndex = ((frameIndex + 1) % ctx.imageCount);
    const auto& srcSampler = pVertInputTex_->samplers[0];
    const auto& dstSampler = pVertInputTexCopies_[copyFrameIndex]->samplers[0];

    // Set values that don't change first.
    vk::ImageMemoryBarrier imgBarrier = {};
    imgBarrier.image = dstSampler.image;
    imgBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imgBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imgBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    imgBarrier.subresourceRange.layerCount = dstSampler.imgCreateInfo.arrayLayers;
    imgBarrier.subresourceRange.levelCount = 1;

    {  // Barrier for final compute work, and image transition for the copy.
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

void Ocean::init() {
    const auto& ctx = handler().shell().context();
    // RESOURCES
    createCommandBuffers(ctx.imageCount);
    createSemaphores(ctx.imageCount, ctx.imageCount);
    createFences(ctx.imageCount);
    // The following submit resources are always the same so set the sizes.
    resources.submit.commandBuffers.resize(1);
    resources.submit.signalSemaphores.resize(1);
}

void Ocean::tick() {
    if (status_ != STATUS::READY) {
        const auto& ctx = handler().shell().context();

        // Store pointers to the dispersion relationship textures for convenience/speed.
        pVertInputTex_ = handler().textureHandler().getTexture(Texture::Ocean::VERT_INPUT_ID).get();
        assert(pVertInputTex_ != nullptr);
        assert(ctx.imageCount == 3);  // Potential image count problem.
        for (uint32_t i = 0; i < ctx.imageCount; i++) {
            pVertInputTexCopies_[i] = handler().textureHandler().getTexture(Texture::Ocean::VERT_INPUT_COPY_ID, i).get();
            assert(pVertInputTexCopies_[i] != nullptr);
        }

        // Store a pointer to the graphics work for convenience.
        pGraphicsWork_ = handler().sceneHandler().ocnRenderer.pGraphicsWork.get();

        // Set the descriptor set bind data. This function should be called on first tick at earliest.
        assert(getDescSetBindDataMaps().empty());
        setDescSetBindData();

        status_ = STATUS::READY;
        onTogglePause();
    }
}

void Ocean::frame() {
    assert(status_ == STATUS::READY);

    const auto& ctx = handler().shell().context();
    const auto frameIndex = handler().renderPassMgr().getFrameIndex();
    const auto frameCount = handler().game().getFrameCount();

    const auto& fence = resources.fences[frameIndex];
    const auto& cmd = resources.cmds[frameIndex];

    // Need to copy for (imageCount - 1) frames after pause so that all image layers have the last set of dispatch's data. A
    // dispatch is only ever needed when a copy is also required.
    const bool needCopy = (!getPaused() || ((frameCount - pauseFrameCount_) < (ctx.imageCount - 1)));

    // Wait for fence every frame, or the if the simulation was just paused.
    if (needCopy) {
        /* TODO: This is really bad to wait for a fence here. This should probably be moved tick(), and not be waited on
         * indefinetly. I put a comment about this in TODO.new.txt.
         */
        vk::Result result = ctx.dev.waitForFences(fence, VK_TRUE, UINT64_MAX);
        assert(result == vk::Result::eSuccess);
        ctx.dev.resetFences({fence});

        // TODO: This concept needs some work obviously...
        if (!pGraphicsWork_->getDraw()) pGraphicsWork_->toggleDraw();
    }

    // Update shader resources when simulation is not paused.
    if (!getPaused()) {
        pOcnSimDpch_->update(handler().shell().getElapsedTime<float>());
        handler().uniformHandler().ocnSimDpchMgr().updateData(ctx.dev, pOcnSimDpch_->BUFFER_INFO);
    }

    // Record command buffers.
    if (needCopy) {
        cmd.begin(vk::CommandBufferBeginInfo{});

        // Dispatch if not paused.
        if (!getPaused()) {
            const auto& pipelineBindDataList = getPipelineBindDataList();
            assert(pipelineBindDataList.size() == 3);  // OCEAN_DISP/OCEAN_FFT/OCEAN_VERT_INPUT
            dispatch(TYPE, pipelineBindDataList.getValue(0), getDescSetBindData(TYPE, 0), cmd, frameIndex);  // DISP
            dispatch(TYPE, pipelineBindDataList.getValue(1), getDescSetBindData(TYPE, 1), cmd, frameIndex);  // FFT
            dispatch(TYPE, pipelineBindDataList.getValue(2), getDescSetBindData(TYPE, 1), cmd, frameIndex);  // VERT_INPUT
        }

        // Copy the compute work data to a per-frame heightmap image for drawing.
        copyImage(cmd, frameIndex);

        {  // Finalize submit resources
            cmd.end();
            resources.submit.commandBuffers[0] = cmd;
            resources.submit.signalSemaphores[0] = resources.semaphores[frameIndex];
            /* Once the simulation starts the surface draw uses the copied heightmap from the previous frame. The surface
             * draw submits a signal semaphore that we need to wait on here when we are copying to the heightmap that could
             * still be apart of a draw. If the simulation is paused, and we still need to copy (which is implied) then we
             * need to wait as well.
             *
             * Note: This class governs the use of the render semaphores, so this should be safe.
             */
            resources.submit.waitSemaphores.clear();
            const bool needWait = (!getPaused() && ((frameCount - startFrameCount_) > (ctx.imageCount - 1))) || getPaused();
            if (needWait) {
                const auto drawIndex = ((frameIndex + 1) % 3);
                resources.submit.waitSemaphores.push_back(resources.drawSemaphores[drawIndex]);
                resources.submit.waitDstStageMask = vk::PipelineStageFlagBits::eTransfer;
            }
            resources.submit.fence = fence;
            resources.hasData = true;
        }
    }
}

void Ocean::togglePause() {
    const auto frameCount = handler().game().getFrameCount();
    startFrameCount_ = getPaused() ? UINT64_MAX : frameCount;
    pauseFrameCount_ = getPaused() ? frameCount : UINT64_MAX;
}

void Ocean::destroy() {
    startFrameCount_ = UINT64_MAX;
    pauseFrameCount_ = UINT64_MAX;
    pOcnSimDpch_ = nullptr;
    pVertInputTex_ = nullptr;
    pVertInputTexCopies_ = {};
}

}  // namespace ComputeWork