/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Ocean.h"

#include <cmath>
#include <complex>
#include <random>
#include <string>
#include <vulkan/vulkan.h>

#include "Deferred.h"
#include "Helpers.h"
// HANDLERS
#include "LoadingHandler.h"
#include "ParticleHandler.h"
#include "PipelineHandler.h"
#include "TextureHandler.h"

namespace {

float phillipsSpectrum(const glm::vec2 k, const float kMagnitude, const Ocean::SurfaceCreateInfo& info) {
    if (kMagnitude < 1e-5f) return 0.0f;
    float kHatOmegaHat = glm::dot(k, info.omega) / kMagnitude;  // cosine factor
    float kMagnitude2 = kMagnitude * kMagnitude;
    float damp = exp(-kMagnitude2 * info.l * info.l);
    float phk = info.A * damp * (exp(-1.0f / (kMagnitude2 * info.L * info.L)) / (kMagnitude2 * kMagnitude2)) *
                (kHatOmegaHat * kHatOmegaHat);
    return phk;
}

}  // namespace

// TEXUTRE
namespace Texture {
namespace Ocean {

void MakTextures(Handler& handler, const ::Ocean::SurfaceCreateInfo& info) {
    assert(helpers::isPowerOfTwo(info.N) && helpers::isPowerOfTwo(info.M));

    std::default_random_engine gen;
    std::normal_distribution<float> dist(0.0, 1.0);

    auto dataSize = (static_cast<uint64_t>(info.N) * 4) * static_cast<uint64_t>(info.M) * sizeof(float);

    // Wave vector data
    float* pWave = (float*)malloc(dataSize);
    assert(pWave);

    // Fourier domain amplitude data
    float* pHTilde0 = (float*)malloc(dataSize);
    assert(pHTilde0);

    const int halfN = info.N / 2, halfM = info.M / 2;
    int idx;
    float kx, kz, kMagnitude, phk;
    std::complex<float> hTilde0, hTilde0Conj, xhi;

    for (int i = 0, m = -halfM; i < static_cast<int>(info.M); i++, m++) {
        for (int j = 0, n = -halfN; j < static_cast<int>(info.N); j++, n++) {
            idx = (i * info.N * 4) + (j * 4);

            // Wave vector data
            {
                kx = 2.0f * glm::pi<float>() * n / info.Lx;
                kz = 2.0f * glm::pi<float>() * m / info.Lz;
                kMagnitude = sqrt(kx * kx + kz * kz);

                pWave[idx + 0] = kx;
                pWave[idx + 1] = kz;
                pWave[idx + 2] = kMagnitude;
                pWave[idx + 3] = sqrt(::Ocean::g * kMagnitude);
            }

            // Fourier domain data
            {
                phk = phillipsSpectrum({kx, kz}, kMagnitude, info);
                xhi = {dist(gen), dist(gen)};
                hTilde0 = xhi * sqrt(phk / 2.0f);
                // conjugate
                phk = phillipsSpectrum({-kx, -kz}, kMagnitude, info);
                xhi = {dist(gen), dist(gen)};
                hTilde0Conj = std::conj(xhi * sqrt(phk / 2.0f));

                pHTilde0[idx + 0] = hTilde0.real();
                pHTilde0[idx + 1] = hTilde0.imag();
                pHTilde0[idx + 2] = hTilde0Conj.real();
                pHTilde0[idx + 3] = hTilde0Conj.imag();
            }
        }
    }

    // Create texture
    {
        Sampler::CreateInfo sampInfo = {
            std::string(OCEAN_DATA_ID) + " Sampler",
            {{
                 {::Sampler::USAGE::DONT_CARE},  // wave vector
                 {::Sampler::USAGE::DONT_CARE},  // fourier domain
                 {::Sampler::USAGE::HEIGHT},     // fourier domain dispersion relation (height)
                 {::Sampler::USAGE::NORMAL},     // fourier domain dispersion relation (slope)
                 {::Sampler::USAGE::DONT_CARE},  // fourier domain dispersion relation (differential)
             },
             true,
             true},
            VK_IMAGE_VIEW_TYPE_2D_ARRAY,
            {info.N, info.M, 1},
            {},
            0,
            SAMPLER::DEFAULT_NEAREST,  // Maybe this texture should be split up for filtering layers separately?
            VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            {{false, false}, 1},
            VK_FORMAT_R32G32B32A32_SFLOAT,
            Sampler::CHANNELS::_4,
            sizeof(float),
        };

        for (size_t i = 0; i < sampInfo.layersInfo.infos.size(); i++) {
            if (i == 0)
                sampInfo.layersInfo.infos[i].pPixel = pWave;
            else if (i == 1)
                sampInfo.layersInfo.infos[i].pPixel = pHTilde0;
            else
                sampInfo.layersInfo.infos[i].pPixel = (float*)malloc(dataSize);  // TODO: this shouldn't be necessary
        }

        Texture::CreateInfo waveTexInfo = {std::string(OCEAN_DATA_ID), {sampInfo}, false, false, STORAGE_IMAGE::DONT_CARE};
        handler.make(&waveTexInfo);
    }
}

}  // namespace Ocean
}  // namespace Texture

// SHADER
namespace Shader {
namespace Ocean {
const CreateInfo DISP_COMP_CREATE_INFO = {
    SHADER::OCEAN_DISP_COMP,  //
    "Ocean Sufrace Dispersion Compute Shader",
    "comp.ocean.dispersion.glsl",
    VK_SHADER_STAGE_COMPUTE_BIT,
    {SHADER_LINK::HELPERS},

};
const CreateInfo FFT_COMP_CREATE_INFO = {
    SHADER::OCEAN_FFT_COMP,  //
    "Ocean Surface Fast Fourier Transform Compute Shader",
    "comp.ocean.fft.glsl",
    VK_SHADER_STAGE_COMPUTE_BIT,
    {SHADER_LINK::HELPERS},

};
const CreateInfo VERT_CREATE_INFO = {
    SHADER::OCEAN_VERT,  //
    "Ocean Surface Vertex Shader",
    "vert.ocean.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,

};
}  // namespace Ocean
}  // namespace Shader

// UNIFORM DYNAMIC
namespace UniformDynamic {
namespace Ocean {
namespace Simulation {
Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM_DYNAMIC::OCEAN),
      Buffer::PerFramebufferDataItem<DATA>(pData) {
    assert(helpers::isPowerOfTwo(pCreateInfo->info.N) && helpers::isPowerOfTwo(pCreateInfo->info.M));
    data_.nLog2 = static_cast<uint32_t>(log2(pCreateInfo->info.N));
    data_.mLog2 = static_cast<uint32_t>(log2(pCreateInfo->info.M));
    data_.t = 0.0f;
    setData();
}
}  // namespace Simulation
}  // namespace Ocean
}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
const CreateInfo OCEAN_DEFAULT_CREATE_INFO = {
    DESCRIPTOR_SET::OCEAN_DEFAULT,
    "_DS_OCEAN",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_DEFAULT}},
        {{2, 0}, {UNIFORM_DYNAMIC::OCEAN}},
        {{3, 0}, {STORAGE_IMAGE::PIPELINE, Texture::Ocean::OCEAN_DATA_ID}},
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
    {DESCRIPTOR_SET::OCEAN_DEFAULT},
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
           createInfoRes.shaderStageInfos[0].stage == VK_SHADER_STAGE_COMPUTE_BIT);
    // Add the specialization to the shader info.
    createInfoRes.shaderStageInfos[0].pSpecializationInfo = &createInfoRes.specializationInfo.back();
}

// FFT (COMPUTE)
const CreateInfo FFT_CREATE_INFO = {
    COMPUTE::OCEAN_FFT,
    "Ocean Surface FFT Compute Pipeline",
    {SHADER::OCEAN_FFT_COMP},
    {DESCRIPTOR_SET::OCEAN_DEFAULT},
};
FFT::FFT(Handler& handler) : Compute(handler, &FFT_CREATE_INFO) {}

// WIREFRAME
const CreateInfo HFF_WF_CREATE_INFO = {
    GRAPHICS::OCEAN_WF_DEFERRED,
    "Ocean Surface Wireframe (Deferred) Pipeline",
    {SHADER::OCEAN_VERT, SHADER::DEFERRED_MRT_COLOR_FRAG},
    {DESCRIPTOR_SET::OCEAN_DEFAULT},
};
Wireframe::Wireframe(Handler& handler) : Graphics(handler, &HFF_WF_CREATE_INFO), DO_BLEND(false), IS_DEFERRED(true) {}

void Wireframe::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}

void Wireframe::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    ::HeightFieldFluid::VertexData::getInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_VERTEX);
    Storage::Vector4::GetInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_VERTEX);  // Not used
    Instance::Obj3d::DATA::getInputDescriptions(createInfoRes);

    // bindings
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = createInfoRes.bindDescs.data();
    // attributes
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = {};
    createInfoRes.inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    createInfoRes.inputAssemblyStateInfo.pNext = nullptr;
    createInfoRes.inputAssemblyStateInfo.flags = 0;
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_TRUE;
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
}

}  // namespace Ocean

}  // namespace Pipeline

// BUFFER
namespace Ocean {

Buffer::Buffer(Particle::Handler& handler, const Particle::Buffer::index&& offset, const CreateInfo* pCreateInfo,
               std::shared_ptr<Material::Base>& pMaterial,
               const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors,
               std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData)
    : Buffer::Base(handler, std::forward<const Particle::Buffer::index>(offset), pCreateInfo, pMaterial, pDescriptors),
      Obj3d::InstanceDraw(pInstanceData),
      normalOffset_(Particle::Buffer::BAD_OFFSET),
      indexWFRes_{},
      drawMode(GRAPHICS::OCEAN_WF_DEFERRED) {
    assert(helpers::isPowerOfTwo(pCreateInfo->info.N) && helpers::isPowerOfTwo(pCreateInfo->info.M));

    for (uint32_t i = 0; i < static_cast<uint32_t>(pDescriptors_.size()); i++)
        if (pDescriptors[i]->getDescriptorType() == DESCRIPTOR{STORAGE_BUFFER_DYNAMIC::NORMAL}) normalOffset_ = i;
    assert(normalOffset_ != Particle::Buffer::BAD_OFFSET);

    workgroupSize_.x = pCreateInfo->info.N;
    workgroupSize_.y = pCreateInfo->info.M;
    assert(workgroupSize_.x > 1 && workgroupSize_.y > 1);

    // IMAGE
    // TODO: move the loop inside this function into the constructor here.
    Texture::Ocean::MakTextures(handler.textureHandler(), pCreateInfo->info);

    const int halfN = workgroupSize_.x / 2, halfM = workgroupSize_.y / 2;
    verticesHFF_.reserve(workgroupSize_.x * workgroupSize_.y);
    verticesHFF_.reserve(workgroupSize_.x * workgroupSize_.y);
    for (int i = 0, m = -halfM; i < static_cast<int>(workgroupSize_.y); i++, m++) {
        for (int j = 0, n = -halfN; j < static_cast<int>(workgroupSize_.x); j++, n++) {
            // VERTEX
            verticesHFF_.push_back({
                // position
                {
                    n * pCreateInfo->info.Lx / workgroupSize_.x,
                    0.0f,
                    m * pCreateInfo->info.Lz / workgroupSize_.y,
                },
                // normal
                // image offset
                {static_cast<int>(j), static_cast<int>(i)},
            });
        }
    }

    // INDEX
    {
        size_t numIndices = static_cast<size_t>(static_cast<size_t>(workgroupSize_.x) *
                                                    (2 + ((static_cast<size_t>(workgroupSize_.y) - 2) * 2)) +
                                                static_cast<size_t>(workgroupSize_.y) - 1);
        indices_.resize(numIndices);

        size_t index = 0;
        for (uint32_t row = 0; row < workgroupSize_.y - 1; row++) {
            for (uint32_t col = 0; col < workgroupSize_.x; col++) {
                // Triangle strip indices (surface)
                indices_[index + 1] = (row + 1) * workgroupSize_.x + col;
                indices_[index] = row * workgroupSize_.x + col;
                index += 2;
                // Line strip indices (wireframe)
                indicesWF_.push_back(row * workgroupSize_.x + col + workgroupSize_.x);
                indicesWF_.push_back(indicesWF_[indicesWF_.size() - 1] - workgroupSize_.x);
                if (col + 1 < workgroupSize_.x) {
                    indicesWF_.push_back(indicesWF_[indicesWF_.size() - 1] + 1);
                    indicesWF_.push_back(indicesWF_[indicesWF_.size() - 3]);
                }
            }
            indices_[index++] = VB_INDEX_PRIMITIVE_RESTART;
            indicesWF_.push_back(VB_INDEX_PRIMITIVE_RESTART);
        }
    }

    status_ |= STATUS::PENDING_BUFFERS;
    draw_ = true;
    paused_ = false;
}

void Buffer::loadBuffers() {
    const auto& ctx = handler().shell().context();
    pLdgRes_ = handler().loadingHandler().createLoadingResources();

    // VERTEX
    BufferResource stgRes = {};
    helpers::createBuffer(
        ctx.dev, ctx.memProps, ctx.debugMarkersEnabled, pLdgRes_->transferCmd,
        static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
        sizeof(HeightFieldFluid::VertexData) * verticesHFF_.size(), NAME + " vertex", stgRes, verticesHFFRes_,
        verticesHFF_.data());
    pLdgRes_->stgResources.push_back(std::move(stgRes));

    // INDEX (SURFACE)
    assert(indices_.size());
    stgRes = {};
    helpers::createBuffer(
        ctx.dev, ctx.memProps, ctx.debugMarkersEnabled, pLdgRes_->transferCmd,
        static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
        sizeof(VB_INDEX_TYPE) * indices_.size(), NAME + " index (surface)", stgRes, indexRes_, indices_.data());
    pLdgRes_->stgResources.push_back(std::move(stgRes));

    // INDEX (WIREFRAME)
    assert(indicesWF_.size());
    stgRes = {};
    helpers::createBuffer(
        ctx.dev, ctx.memProps, ctx.debugMarkersEnabled, pLdgRes_->transferCmd,
        static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
        sizeof(VB_INDEX_TYPE) * indicesWF_.size(), NAME + " index (wireframe)", stgRes, indexWFRes_, indicesWF_.data());
    pLdgRes_->stgResources.push_back(std::move(stgRes));
}

void Buffer::destroy() {
    Base::destroy();
    auto& dev = handler().shell().context().dev;
    if (verticesHFF_.size()) {
        vkDestroyBuffer(dev, verticesHFFRes_.buffer, nullptr);
        vkFreeMemory(dev, verticesHFFRes_.memory, nullptr);
    }
    if (indicesWF_.size()) {
        vkDestroyBuffer(dev, indexWFRes_.buffer, nullptr);
        vkFreeMemory(dev, indexWFRes_.memory, nullptr);
    }
}

void Buffer::draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                  const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                  const uint8_t frameIndex) const {
    if (pPipelineBindData->type != PIPELINE{drawMode}) return;

    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    switch (drawMode) {
        case GRAPHICS::OCEAN_WF_DEFERRED: {
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);
            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());
            const VkBuffer buffers[] = {
                verticesHFFRes_.buffer,
                pDescriptors_[normalOffset_]->BUFFER_INFO.bufferInfo.buffer,  // Not used
                pInstObj3d_->BUFFER_INFO.bufferInfo.buffer,

            };
            const VkDeviceSize offsets[] = {
                0,
                pDescriptors_[normalOffset_]->BUFFER_INFO.memoryOffset,  // Not used
                pInstObj3d_->BUFFER_INFO.memoryOffset,
            };
            vkCmdBindVertexBuffers(cmd, 0, 3, buffers, offsets);
            vkCmdBindIndexBuffer(cmd, indexWFRes_.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(                              //
                cmd,                                       // VkCommandBuffer commandBuffer
                static_cast<uint32_t>(indicesWF_.size()),  // uint32_t indexCount
                pInstObj3d_->BUFFER_INFO.count,            // uint32_t instanceCount
                0,                                         // uint32_t firstIndex
                0,                                         // int32_t vertexOffset
                0                                          // uint32_t firstInstance
            );
        } break;
        default: {
            assert(false);
        } break;
    }
}

void Buffer::dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                      const uint8_t frameIndex) const {
    assert(LOCAL_SIZE.z == 1 && workgroupSize_.z == 1);
    assert(workgroupSize_.x % LOCAL_SIZE.x == 0 && workgroupSize_.y % LOCAL_SIZE.y == 0);

    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    const VkMemoryBarrier memoryBarrierCompute = {
        VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,  // srcAccessMask
        VK_ACCESS_SHADER_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,  // dstAccessMask
    };

    switch (std::visit(Pipeline::GetCompute{}, pPipelineBindData->type)) {
        case COMPUTE::FFT_ONE: {
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());

            vkCmdDispatch(cmd, 1, 4, 1);

            vkCmdPipelineBarrier(cmd,                                   //
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
                                 0,                                     // dependencyFlags
                                 1,                                     // memoryBarrierCount
                                 &memoryBarrierCompute,                 // pMemoryBarriers
                                 0, nullptr, 0, nullptr);

            vkCmdDispatch(cmd, 4, 1, 1);

        } break;
        case COMPUTE::OCEAN_DISP: {
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());

            vkCmdDispatch(cmd, workgroupSize_.x, workgroupSize_.y, 1);

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1,
                                 &memoryBarrierCompute, 0, nullptr, 0, nullptr);

        } break;
        case COMPUTE::OCEAN_FFT: {
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());

            vkCmdDispatch(cmd, 1, workgroupSize_.y, 1);

            vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 1,
                                 &memoryBarrierCompute, 0, nullptr, 0, nullptr);

            vkCmdDispatch(cmd, workgroupSize_.x, 1, 1);

        } break;
        default: {
            assert(false);
        } break;
    }
}

}  // namespace Ocean