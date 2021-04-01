/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Ocean.h"

#include <cmath>
#include <complex>
#include <random>
#include <string>

#include <Common/Helpers.h>

#include "Deferred.h"
#include "FFT.h"
#if !OCEAN_USE_COMPUTE_QUEUE_DISPATCH
#include "OceanComputeWork.h"
#endif
#include "Tessellation.h"
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

Sampler::CreateInfo getDefaultOceanSampCreateInfo(const std::string&& name, const uint32_t N, const uint32_t M,
                                                  const vk::ImageUsageFlags usageFlags) {
    return {
        name,
        {{
             {::Sampler::USAGE::HEIGHT},     // fourier domain dispersion relation (height)
             {::Sampler::USAGE::NORMAL},     // fourier domain dispersion relation (slope)
             {::Sampler::USAGE::DONT_CARE},  // fourier domain dispersion relation (differential)
         },
         true,
         true},
        vk::ImageViewType::e2DArray,
        {N, M, 1},
        {},
        {},
        SAMPLER::DEFAULT_NEAREST,
        usageFlags,
        {{false, false}, 1},
        vk::Format::eR32G32B32A32Sfloat,
        Sampler::CHANNELS::_4,
        sizeof(float),
    };
}

}  // namespace

// BUFFER VIEW
namespace BufferView {
namespace Ocean {
void MakeResources(Texture::Handler& handler, const ::Ocean::SurfaceCreateInfo& info) {
    auto bitRevOffsets = ::FFT::MakeBitReversalOffsets(info.N);
    handler.makeBufferView(BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_N_ID, vk::Format::eR16Sint,
                           sizeof(uint16_t) * bitRevOffsets.size(), bitRevOffsets.data());
    if (info.N != info.M) bitRevOffsets = ::FFT::MakeBitReversalOffsets(info.N);
    handler.makeBufferView(BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_M_ID, vk::Format::eR16Sint,
                           sizeof(uint16_t) * bitRevOffsets.size(), bitRevOffsets.data());
    auto twiddleFactors = ::FFT::MakeTwiddleFactors((std::max)(info.N, info.M));
    handler.makeBufferView(BufferView::Ocean::FFT_TWIDDLE_FACTORS_ID, vk::Format::eR32G32Sfloat,
                           sizeof(float) * twiddleFactors.size(), twiddleFactors.data());
}
}  // namespace Ocean
}  // namespace BufferView

// TEXUTRE
namespace Texture {
namespace Ocean {

void MakeResources(Texture::Handler& handler, const ::Ocean::SurfaceCreateInfo& info) {
    assert(helpers::isPowerOfTwo(info.N) && helpers::isPowerOfTwo(info.M));

    using namespace Texture::Ocean;

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

    {  // Create textures
        // Wave and fourier data
        Sampler::CreateInfo sampInfo = {
            std::string(WAVE_FOURIER_ID) + " Sampler",
            {{
                 {::Sampler::USAGE::DONT_CARE},  // wave vector
                 {::Sampler::USAGE::DONT_CARE},  // fourier domain
             },
             true,
             true},
            vk::ImageViewType::e2DArray,
            {info.N, info.M, 1},
            {},
            {},
            SAMPLER::DEFAULT_NEAREST,
            vk::ImageUsageFlagBits::eSampled,
            {{false, false}, 1},
            vk::Format::eR32G32B32A32Sfloat,
            Sampler::CHANNELS::_4,
            sizeof(float),
        };
        sampInfo.layersInfo.infos.at(0).pPixel = pWave;
        sampInfo.layersInfo.infos.at(1).pPixel = pHTilde0;
        Texture::CreateInfo texInfo = {std::string(WAVE_FOURIER_ID), {sampInfo}, false, false, COMBINED_SAMPLER::PIPELINE};
        handler.make(&texInfo);

        // Dispersion relation
        sampInfo = getDefaultOceanSampCreateInfo(std::string(DISP_REL_ID) + " Sampler", info.N, info.M,
                                                 (vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc));
        // sampInfo.type = Something; // TODO: Experiment with filters... I'll try to remember.
        texInfo = {std::string(DISP_REL_ID), {sampInfo}, false, false, STORAGE_IMAGE::PIPELINE};
        handler.make(&texInfo);
    }
}

#if OCEAN_USE_COMPUTE_QUEUE_DISPATCH
CreateInfo MakeCopyTexInfo(const uint32_t N, const uint32_t M) {
    auto sampInfo = getDefaultOceanSampCreateInfo(std::string(DISP_REL_COPY_ID) + " Sampler", N, M,
                                                  (vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled));
    // sampInfo.type = Something; // TODO: Experiment with filters... I'll try to remember.
    return {std::string(DISP_REL_COPY_ID), {sampInfo}, false, true, COMBINED_SAMPLER::PIPELINE};
}
#endif

}  // namespace Ocean
}  // namespace Texture

// SHADER
namespace Shader {
namespace Ocean {
const CreateInfo VERT_CREATE_INFO = {
    SHADER::OCEAN_VERT,  //
    "Ocean Surface Vertex Shader",
    "vert.ocean.glsl",
    vk::ShaderStageFlagBits::eVertex,
    {},
    {{"_DISPATCH_ON_COMPUTE_QUEUE", OCEAN_USE_COMPUTE_QUEUE_DISPATCH ? "1" : "0"}},
};
const CreateInfo DEFERRED_MRT_FRAG_CREATE_INFO = {
    SHADER::OCEAN_DEFERRED_MRT_FRAG,                                  //
    "Ocean Surface Deferred Multiple Render Target Fragment Shader",  //
    "frag.ocean.deferred.mrt.glsl",                                   //
    vk::ShaderStageFlagBits::eFragment,                               //
    {SHADER_LINK::COLOR_FRAG, SHADER_LINK::DEFAULT_MATERIAL},
};
}  // namespace Ocean
}  // namespace Shader

// UNIFORM DYNAMIC
namespace UniformDynamic {
namespace Ocean {
namespace SimulationDraw {
Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM_DYNAMIC::OCEAN_DRAW),
      Buffer::DataItem<DATA>(pData) {
    pData->lambda = pCreateInfo->info.lambda;
    dirty = true;
}
}  // namespace SimulationDraw
}  // namespace Ocean
}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
const CreateInfo OCEAN_DRAW_CREATE_INFO = {
    DESCRIPTOR_SET::OCEAN_DRAW,
    "_DS_OCEAN",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_DEFAULT}},
        {{2, 0}, {UNIFORM_DYNAMIC::OCEAN_DRAW}},
#if OCEAN_USE_COMPUTE_QUEUE_DISPATCH
        {{3, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Ocean::DISP_REL_COPY_ID}},
#else
        {{3, 0}, {STORAGE_IMAGE::PIPELINE, Texture::Ocean::DISP_REL_ID}},
#endif
    },
};
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {

namespace Ocean {

// WIREFRAME
const CreateInfo OCEAN_WF_CREATE_INFO = {
    GRAPHICS::OCEAN_WF_DEFERRED,
    "Ocean Surface Wireframe (Deferred) Pipeline",
    {SHADER::OCEAN_VERT, SHADER::DEFERRED_MRT_COLOR_FRAG},
    {
        {DESCRIPTOR_SET::OCEAN_DRAW, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)},
    },
};
Wireframe::Wireframe(Handler& handler) : Graphics(handler, &OCEAN_WF_CREATE_INFO), DO_BLEND(false), IS_DEFERRED(true) {}

void Wireframe::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}

void Wireframe::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    ::HeightFieldFluid::VertexData::getInputDescriptions(createInfoRes, vk::VertexInputRate::eVertex);
    Storage::Vector4::GetInputDescriptions(createInfoRes, vk::VertexInputRate::eVertex);  // Not used
    Instance::Obj3d::DATA::getInputDescriptions(createInfoRes);

    // bindings
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = createInfoRes.bindDescs.data();
    // attributes
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = vk::PipelineInputAssemblyStateCreateInfo{};
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_TRUE;
    createInfoRes.inputAssemblyStateInfo.topology = vk::PrimitiveTopology::eLineStrip;
}

// SURFACE
const CreateInfo OCEAN_SURFACE_CREATE_INFO = {
    GRAPHICS::OCEAN_SURFACE_DEFERRED,
    "Ocean Surface (Deferred) Pipeline",
    {
        SHADER::OCEAN_VERT,
        SHADER::PHONG_TRI_COLOR_TESC,
        SHADER::PHONG_TRI_COLOR_TESE,
        SHADER::OCEAN_DEFERRED_MRT_FRAG,
    },
    {
        {DESCRIPTOR_SET::OCEAN_DRAW,
         (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eTessellationControl |
          vk::ShaderStageFlagBits::eTessellationEvaluation | vk::ShaderStageFlagBits::eFragment)},
        {DESCRIPTOR_SET::TESS_PHONG,
         (vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eTessellationEvaluation)},
    },
};
Surface::Surface(Handler& handler)
    : Graphics(handler, &OCEAN_SURFACE_CREATE_INFO),
      DO_BLEND(false),
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
      DO_TESSELLATE(false),
#else
      DO_TESSELLATE(true),
#endif
      IS_DEFERRED(true) {
}

void Surface::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}

void Surface::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    ::HeightFieldFluid::VertexData::getInputDescriptions(createInfoRes, vk::VertexInputRate::eVertex);
    Storage::Vector4::GetInputDescriptions(createInfoRes, vk::VertexInputRate::eVertex);  // Not used
    Instance::Obj3d::DATA::getInputDescriptions(createInfoRes);

    // bindings
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = createInfoRes.bindDescs.data();
    // attributes
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = vk::PipelineInputAssemblyStateCreateInfo{};
    if (DO_TESSELLATE) {
        createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
        createInfoRes.inputAssemblyStateInfo.topology = vk::PrimitiveTopology::ePatchList;
    } else {
        createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_TRUE;
        createInfoRes.inputAssemblyStateInfo.topology = vk::PrimitiveTopology::eTriangleStrip;
    }
}

void Surface::getTesselationInfoResources(CreateInfoResources& createInfoRes) {
    if (!DO_TESSELLATE) return;
    createInfoRes.useTessellationInfo = true;
    createInfoRes.tessellationStateInfo = vk::PipelineTessellationStateCreateInfo{};
    createInfoRes.tessellationStateInfo.patchControlPoints = 3;
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
      drawMode(GRAPHICS::OCEAN_SURFACE_DEFERRED),
      normalOffset_(Particle::Buffer::BAD_OFFSET),
      indexWFRes_{},
      info_(pCreateInfo->info) {
    assert(info_.N == info_.M);  // Needs to be square currently.
    assert(info_.N == FFT_LOCAL_SIZE * FFT_WORKGROUP_SIZE);
    assert(info_.N == DISP_LOCAL_SIZE * DISP_WORKGROUP_SIZE);
    assert(helpers::isPowerOfTwo(info_.N) && helpers::isPowerOfTwo(info_.M));

    const auto& N = info_.N;
    const auto& M = info_.M;

    for (uint32_t i = 0; i < static_cast<uint32_t>(pDescriptors_.size()); i++)
        if (pDescriptors[i]->getDescriptorType() == DESCRIPTOR{STORAGE_BUFFER_DYNAMIC::NORMAL}) normalOffset_ = i;
    assert(normalOffset_ != Particle::Buffer::BAD_OFFSET);

    const int halfN = N / 2, halfM = M / 2;
    verticesHFF_.reserve(N * M);
    verticesHFF_.reserve(N * M);
    for (int i = 0, m = -halfM; i < static_cast<int>(M); i++, m++) {
        for (int j = 0, n = -halfN; j < static_cast<int>(N); j++, n++) {
            // VERTEX
            verticesHFF_.push_back({
                // position
                {
                    n * info_.Lx / N,
                    0.0f,
                    m * info_.Lz / M,
                },
                // normal
                // image offset
                {static_cast<int>(j), static_cast<int>(i)},
            });
        }
    }

    // INDEX
    {
        bool doPatchList = true;
        if (!doPatchList) {
            size_t numIndices = static_cast<size_t>(static_cast<size_t>(N) * (2 + ((static_cast<size_t>(M) - 2) * 2)) +
                                                    static_cast<size_t>(M) - 1);
            indices_.resize(numIndices);
        } else {
            indices_.reserve((N - 1) * (M - 1) * 6);
        }

        size_t index = 0;
        for (uint32_t row = 0; row < M - 1; row++) {
            auto rowOffset = row * N;
            for (uint32_t col = 0; col < N; col++) {
                if (!doPatchList) {
                    // Triangle strip indices (surface)
                    indices_[index + 1] = (row + 1) * N + col;
                    indices_[index] = row * N + col;
                    index += 2;
                } else {
                    // Patch list (surface)
                    if (col < N - 1) {
                        indices_.push_back(col + rowOffset);
                        indices_.push_back(col + rowOffset + N);
                        indices_.push_back(col + rowOffset + 1);
                        indices_.push_back(col + rowOffset + 1);
                        indices_.push_back(col + rowOffset + N);
                        indices_.push_back(col + rowOffset + N + 1);
                    }
                }
                // Line strip indices (wireframe)
                indicesWF_.push_back(row * N + col + N);
                indicesWF_.push_back(indicesWF_[indicesWF_.size() - 1] - N);
                if (col + 1 < N) {
                    indicesWF_.push_back(indicesWF_[indicesWF_.size() - 1] + 1);
                    indicesWF_.push_back(indicesWF_[indicesWF_.size() - 3]);
                }
            }
            if (!doPatchList) {
                indices_[index++] = VB_INDEX_PRIMITIVE_RESTART;
            }
            indicesWF_.push_back(VB_INDEX_PRIMITIVE_RESTART);
        }
    }

    status_ |= STATUS::PENDING_BUFFERS;
    draw_ = true;
    paused_ = false;
}

void Buffer::draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                  const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                  const uint8_t frameIndex) const {
    // TODO: Checking the frame count here is a bad solution. If the simulation needs to be paused then the draw should
    // always delayed a frame while waiting for the compute queue work. I'm just going to leave this for now.
    if (handler().game().getFrameCount() == 0 || pPipelineBindData->type != PIPELINE{drawMode}) return;

    const auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    switch (drawMode) {
        case GRAPHICS::OCEAN_WF_DEFERRED: {
            cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);
            cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                   descSetBindData.descriptorSets[setIndex], descSetBindData.dynamicOffsets);
            const std::array<vk::Buffer, 3> buffers = {
                verticesHFFRes_.buffer,
                pDescriptors_[normalOffset_]->BUFFER_INFO.bufferInfo.buffer,  // Not used
                pInstObj3d_->BUFFER_INFO.bufferInfo.buffer,

            };
            const std::array<vk::DeviceSize, 3> offsets = {
                0,
                pDescriptors_[normalOffset_]->BUFFER_INFO.memoryOffset,  // Not used
                pInstObj3d_->BUFFER_INFO.memoryOffset,
            };
            cmd.bindVertexBuffers(0, buffers, offsets);
            cmd.bindIndexBuffer(indexWFRes_.buffer, 0, vk::IndexType::eUint32);
            cmd.drawIndexed(static_cast<uint32_t>(indicesWF_.size()),  // uint32_t indexCount
                            pInstObj3d_->BUFFER_INFO.count,            // uint32_t instanceCount
                            0,                                         // uint32_t firstIndex
                            0,                                         // int32_t vertexOffset
                            0);                                        // uint32_t firstInstance
        } break;
        case GRAPHICS::OCEAN_SURFACE_DEFERRED: {
            cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);
            cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                   static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                   descSetBindData.descriptorSets[setIndex].data(),
                                   static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                   descSetBindData.dynamicOffsets.data());
            const std::array<vk::Buffer, 3> buffers = {
                verticesHFFRes_.buffer,
                pDescriptors_[normalOffset_]->BUFFER_INFO.bufferInfo.buffer,
                pInstObj3d_->BUFFER_INFO.bufferInfo.buffer,
            };
            const std::array<vk::DeviceSize, 3> offsets = {
                0,
                pDescriptors_[normalOffset_]->BUFFER_INFO.memoryOffset,
                pInstObj3d_->BUFFER_INFO.memoryOffset,
            };
            cmd.bindVertexBuffers(0, buffers, offsets);
            cmd.bindIndexBuffer(indexRes_.buffer, 0, vk::IndexType::eUint32);
            cmd.drawIndexed(static_cast<uint32_t>(indices_.size()),  // uint32_t indexCount
                            pInstObj3d_->BUFFER_INFO.count,          // uint32_t instanceCount
                            0,                                       // uint32_t firstIndex
                            0,                                       // int32_t vertexOffset
                            0);                                      // uint32_t firstInstance
        } break;
        default: {
            assert(false);
        } break;
    }
}

void Buffer::dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                      const uint8_t frameIndex) const {
#if !OCEAN_USE_COMPUTE_QUEUE_DISPATCH
    ComputeWork::Ocean::dispatch(passType, pPipelineBindData, descSetBindData, cmd, std::forward<const uint8_t>(frameIndex));
#endif
}

void Buffer::loadBuffers() {
    const auto& ctx = handler().shell().context();
    pLdgRes_ = handler().loadingHandler().createLoadingResources();

    // VERTEX
    BufferResource stgRes = {};
    ctx.createBuffer(pLdgRes_->transferCmd, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                     sizeof(HeightFieldFluid::VertexData) * verticesHFF_.size(), NAME + " vertex", stgRes, verticesHFFRes_,
                     verticesHFF_.data());
    pLdgRes_->stgResources.push_back(std::move(stgRes));

    // INDEX (SURFACE)
    assert(indices_.size());
    stgRes = {};
    ctx.createBuffer(pLdgRes_->transferCmd, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                     sizeof(IndexBufferType) * indices_.size(), NAME + " index (surface)", stgRes, indexRes_,
                     indices_.data());
    pLdgRes_->stgResources.push_back(std::move(stgRes));

    // INDEX (WIREFRAME)
    assert(indicesWF_.size());
    stgRes = {};
    ctx.createBuffer(pLdgRes_->transferCmd, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                     sizeof(IndexBufferType) * indicesWF_.size(), NAME + " index (wireframe)", stgRes, indexWFRes_,
                     indicesWF_.data());
    pLdgRes_->stgResources.push_back(std::move(stgRes));
}

void Buffer::destroy() {
    Base::destroy();
    const auto& ctx = handler().shell().context();
    if (verticesHFF_.size()) ctx.destroyBuffer(verticesHFFRes_);
    if (indicesWF_.size()) ctx.destroyBuffer(indexWFRes_);
}

}  // namespace Ocean