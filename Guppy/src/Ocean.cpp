/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
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
#include "Tessellation.h"
// HANDLERS
#include "LoadingHandler.h"
#include "ParticleHandler.h"
#include "PipelineHandler.h"
#include "TextureHandler.h"
#include "UniformHandler.h"

namespace {

class OceanHeightmapSource : public IHeightmapSource {
   public:
    // OceanHeightmapSource(::Ocean::SurfaceCreateInfo& info) : info_(info) {}
    OceanHeightmapSource() = default;

    // inline int GetSizeX() override { return info_.N; }
    inline int GetSizeX() override { return 4096; }
    // inline int GetSizeY() override { return info_.M; }
    inline int GetSizeY() override { return 2048; }

    unsigned short GetHeightAt(int x, int y) override { return 0; }

    void GetAreaMinMaxZ(int x, int y, int sizeX, int sizeY, unsigned short& minZ, unsigned short& maxZ) override {}

   private:
    //::Ocean::SurfaceCreateInfo& info_;
};

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
            std::string(DATA_ID) + " Sampler",
            {{
                 {::Sampler::USAGE::DONT_CARE},  // wave vector
                 {::Sampler::USAGE::DONT_CARE},  // fourier domain
                 {::Sampler::USAGE::HEIGHT},     // fourier domain dispersion relation (height)
                 {::Sampler::USAGE::NORMAL},     // fourier domain dispersion relation (slope)
                 {::Sampler::USAGE::DONT_CARE},  // fourier domain dispersion relation (differential)
             },
             true,
             true},
            vk::ImageViewType::e2DArray,
            {info.N, info.M, 1},
            {},
            {},
            SAMPLER::DEFAULT_NEAREST,  // Maybe this texture should be split up for filtering layers separately?
            vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled,
            {{false, false}, 1},
            vk::Format::eR32G32B32A32Sfloat,
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

        Texture::CreateInfo waveTexInfo = {std::string(DATA_ID), {sampInfo}, false, false, STORAGE_IMAGE::DONT_CARE};
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
    vk::ShaderStageFlagBits::eCompute,
};
const CreateInfo FFT_COMP_CREATE_INFO = {
    SHADER::OCEAN_FFT_COMP,  //
    "Ocean Surface Fast Fourier Transform Compute Shader",
    "comp.ocean.fft.glsl",
    vk::ShaderStageFlagBits::eCompute,
};
const CreateInfo VERT_CREATE_INFO = {
    SHADER::OCEAN_VERT,  //
    "Ocean Surface Vertex Shader",
    "vert.ocean.glsl",
    vk::ShaderStageFlagBits::eVertex,
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

namespace Uniform {
namespace CDLOD {
namespace QuadTree {
Base::Base(const Buffer::Info&& info, DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Descriptor::Base(UNIFORM::CDLOD_QUAD_TREE),
      Buffer::DataItem<DATA>(pData) {
    dirty = true;
}
}  // namespace QuadTree
}  // namespace CDLOD
}  // namespace Uniform

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
    data_.lambda = pCreateInfo->info.lambda;
    data_.t = 0.0f;
    setData();
}
void Base::updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) {
    data_.t = time;
    setData(frameIndex);
}
}  // namespace Simulation
}  // namespace Ocean

namespace CDLOD {
namespace Grid {
Base::Base(const Buffer::Info&& info, DATA* pData, const Buffer::CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM_DYNAMIC::OCEAN),
      Buffer::PerFramebufferDataItem<DATA>(pData) {
    setData();
}
void Base::updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) {
    assert(false);
    setData(frameIndex);
}
}  // namespace Grid
}  // namespace CDLOD

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
        {{3, 0}, {STORAGE_IMAGE::PIPELINE, Texture::Ocean::DATA_ID}},
        {{4, 0}, {UNIFORM_TEXEL_BUFFER::PIPELINE, BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_N_ID}},
        {{5, 0}, {UNIFORM_TEXEL_BUFFER::PIPELINE, BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_M_ID}},
        {{6, 0}, {UNIFORM_TEXEL_BUFFER::PIPELINE, BufferView::Ocean::FFT_TWIDDLE_FACTORS_ID}},
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
    {DESCRIPTOR_SET::OCEAN_DEFAULT},
    {},
    {PUSH_CONSTANT::FFT_ROW_COL_OFFSET},
    {::Ocean::FFT_LOCAL_SIZE, 1, 1},
};
FFT::FFT(Handler& handler) : Compute(handler, &FFT_CREATE_INFO) {}

// WIREFRAME
const CreateInfo OCEAN_WF_CREATE_INFO = {
    GRAPHICS::OCEAN_WF_DEFERRED,
    "Ocean Surface Wireframe (Deferred) Pipeline",
    {SHADER::OCEAN_VERT, SHADER::DEFERRED_MRT_COLOR_FRAG},
    {DESCRIPTOR_SET::OCEAN_DEFAULT},
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
    {DESCRIPTOR_SET::OCEAN_DEFAULT, DESCRIPTOR_SET::TESS_PHONG},
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
      info_(pCreateInfo->info),
      cdlodQuadTree_(),
      cdlodRenderer_(handler.shell().context()) {
    assert(info_.N == info_.M);  // Needs to be square currently.
    assert(info_.N == FFT_LOCAL_SIZE * FFT_WORKGROUP_SIZE);
    assert(info_.N == DISP_LOCAL_SIZE * DISP_WORKGROUP_SIZE);
    assert(helpers::isPowerOfTwo(info_.N) && helpers::isPowerOfTwo(info_.M));

    const auto& N = info_.N;
    const auto& M = info_.M;

    for (uint32_t i = 0; i < static_cast<uint32_t>(pDescriptors_.size()); i++)
        if (pDescriptors[i]->getDescriptorType() == DESCRIPTOR{STORAGE_BUFFER_DYNAMIC::NORMAL}) normalOffset_ = i;
    assert(normalOffset_ != Particle::Buffer::BAD_OFFSET);

    // IMAGE
    // TODO: move the loop inside this function into the constructor here.
    Texture::Ocean::MakTextures(handler.textureHandler(), info_);

    initCDLOD();

    // BUFFER VIEWS
    {
        auto bitRevOffsets = FFT::MakeBitReversalOffsets(N);
        handler.textureHandler().makeBufferView(BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_N_ID, vk::Format::eR16Sint,
                                                sizeof(uint16_t) * bitRevOffsets.size(), bitRevOffsets.data());
        if (N != M) bitRevOffsets = FFT::MakeBitReversalOffsets(N);
        handler.textureHandler().makeBufferView(BufferView::Ocean::FFT_BIT_REVERSAL_OFFSETS_M_ID, vk::Format::eR16Sint,
                                                sizeof(uint16_t) * bitRevOffsets.size(), bitRevOffsets.data());
        auto twiddleFactors = FFT::MakeTwiddleFactors((std::max)(N, M));
        handler.textureHandler().makeBufferView(BufferView::Ocean::FFT_TWIDDLE_FACTORS_ID, vk::Format::eR32G32Sfloat,
                                                sizeof(float) * twiddleFactors.size(), twiddleFactors.data());
    }

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

void Buffer::draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                  const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                  const uint8_t frameIndex) const {
    if (pPipelineBindData->type != PIPELINE{drawMode}) return;

    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

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
            cmd.drawIndexed(                               //
                static_cast<uint32_t>(indicesWF_.size()),  // uint32_t indexCount
                pInstObj3d_->BUFFER_INFO.count,            // uint32_t instanceCount
                0,                                         // uint32_t firstIndex
                0,                                         // int32_t vertexOffset
                0                                          // uint32_t firstInstance
            );
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
            cmd.drawIndexed(                             //
                static_cast<uint32_t>(indices_.size()),  // uint32_t indexCount
                pInstObj3d_->BUFFER_INFO.count,          // uint32_t instanceCount
                0,                                       // uint32_t firstIndex
                0,                                       // int32_t vertexOffset
                0                                        // uint32_t firstInstance
            );
        } break;
        default: {
            assert(false);
        } break;
    }
}

void Buffer::dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                      const uint8_t frameIndex) const {
    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    const vk::MemoryBarrier memoryBarrierCompute = {
        vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,  // srcAccessMask
        vk::AccessFlagBits::eShaderWrite | vk::AccessFlagBits::eShaderRead,  // dstAccessMask
    };

    switch (std::visit(Pipeline::GetCompute{}, pPipelineBindData->type)) {
        case COMPUTE::FFT_ONE: {
            cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

            cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                   static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                   descSetBindData.descriptorSets[setIndex].data(),
                                   static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                   descSetBindData.dynamicOffsets.data());

            cmd.dispatch(1, 4, 1);

            cmd.pipelineBarrier(                            //
                vk::PipelineStageFlagBits::eComputeShader,  // srcStageMask
                vk::PipelineStageFlagBits::eComputeShader,  // dstStageMask
                {},                                         // dependencyFlags
                {memoryBarrierCompute},                     // pMemoryBarriers
                {}, {});

            cmd.dispatch(4, 1, 1);

        } break;
        case COMPUTE::OCEAN_DISP: {
            cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

            cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                   static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                   descSetBindData.descriptorSets[setIndex].data(),
                                   static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                   descSetBindData.dynamicOffsets.data());

            cmd.dispatch(DISP_WORKGROUP_SIZE, DISP_WORKGROUP_SIZE, 1);

            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {},
                                {memoryBarrierCompute}, {}, {});

        } break;
        case COMPUTE::OCEAN_FFT: {
            cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

            cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                   static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                   descSetBindData.descriptorSets[setIndex].data(),
                                   static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                   descSetBindData.dynamicOffsets.data());

            FFT::RowColumnOffset offset = 1;  // row
            cmd.pushConstants(pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                              static_cast<uint32_t>(sizeof(FFT::RowColumnOffset)), &offset);

            cmd.dispatch(FFT_WORKGROUP_SIZE, 1, 1);

            cmd.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eComputeShader, {},
                                {memoryBarrierCompute}, {}, {});

            offset = 0;  // column
            cmd.pushConstants(pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                              static_cast<uint32_t>(sizeof(FFT::RowColumnOffset)), &offset);

            cmd.dispatch(FFT_WORKGROUP_SIZE, 1, 1);

        } break;
        default: {
            assert(false);
        } break;
    }
}

void Buffer::initCDLOD() {
    struct Settings {
        int LeafQuadTreeNodeSize;
        int RenderGridResolutionMult;
        int LODLevelCount;
        float MinViewRange = 35000.0f;
        float MaxViewRange = 100000.0f;
        float LODLevelDistanceRatio = 2.0f;
        // DemoRenderer members
        int maxRenderGridResolutionMult;
        int terrainGridMeshDims;
    } settings;

    // SETUP
    {
        // RENDERER
        for (auto& gridMesh : cdlodRenderer_.m_gridMeshes) {
            auto pLdgRes = handler().loadingHandler().createLoadingResources();
            gridMesh.CreateBuffers(*pLdgRes.get());
            handler().loadingHandler().loadSubmit(std::move(pLdgRes));
        }

        settings.LeafQuadTreeNodeSize = 8;
        settings.RenderGridResolutionMult = 4;
        settings.LODLevelCount = 8;

        // SETUP/VALIDATION
        {
            if (!helpers::isPowerOfTwo(settings.LeafQuadTreeNodeSize) || (settings.LeafQuadTreeNodeSize < 2) ||
                (settings.LeafQuadTreeNodeSize > 1024)) {
                assert(false && "CDLOD:LeafQuadTreeNodeSize setting is incorrect");
                abort();
            }

            settings.maxRenderGridResolutionMult = 1;
            while (settings.maxRenderGridResolutionMult * settings.LeafQuadTreeNodeSize <= 128)
                settings.maxRenderGridResolutionMult *= 2;
            settings.maxRenderGridResolutionMult /= 2;

            if (!helpers::isPowerOfTwo(settings.RenderGridResolutionMult) || (settings.RenderGridResolutionMult < 1) ||
                (settings.LeafQuadTreeNodeSize > settings.maxRenderGridResolutionMult)) {
                assert(false && "CDLOD:RenderGridResolutionMult setting is incorrect");
                abort();
            }

            if ((settings.LODLevelCount < 2) || (settings.LODLevelCount > CDLODQuadTree::c_maxLODLevels)) {
                assert(false && "CDLOD:LODLevelCount setting is incorrect");
                abort();
            }

            if ((settings.MinViewRange < 1.0f) || (settings.MinViewRange > 10000000.0f)) {
                assert(false && "MinViewRange setting is incorrect");
                abort();
            }
            if ((settings.MaxViewRange < 1.0f) || (settings.MaxViewRange > 10000000.0f) ||
                (settings.MinViewRange > settings.MaxViewRange)) {
                assert(false && "MaxViewRange setting is incorrect");
                abort();
            }

            if ((settings.LODLevelDistanceRatio < 1.5f) || (settings.LODLevelDistanceRatio > 16.0f)) {
                assert(false && "LODLevelDistanceRatio setting is incorrect");
                abort();
            }

            settings.terrainGridMeshDims = settings.LeafQuadTreeNodeSize * settings.RenderGridResolutionMult;
        }

        // CREATE
        {
            MapDimensions mapDims = {};
            mapDims.MinX = -20480.0;
            mapDims.MinY = -10240.0;
            mapDims.MinZ = -600.00;
            mapDims.SizeX = 40960.0;
            mapDims.SizeY = 20480.0;
            mapDims.SizeZ = 600.00;

            // OceanHeightmapSource heightMapSrc(info_);
            OceanHeightmapSource heightMapSrc;

            CDLODQuadTree::CreateDesc createDesc = {};
            createDesc.pHeightmap = &heightMapSrc;
            createDesc.LeafRenderNodeSize = settings.LeafQuadTreeNodeSize;
            createDesc.LODLevelCount = settings.LODLevelCount;
            createDesc.MapDims = mapDims;

            assert(createDesc.pHeightmap);

            cdlodQuadTree_.Create(createDesc);
        }

        // UNIFORM
        {
            CDLODRenderer::UniformData quadTreeData;
            cdlodRenderer_.SetIndependentGlobalVertexShaderConsts(quadTreeData, cdlodQuadTree_);
            handler().uniformHandler().cdlodQdTrMgr().insert(handler().shell().context().dev, true, {quadTreeData});
        }
    }

    // RENDER
    {
        const auto& camera = handler().uniformHandler().getMainCamera();
        auto planes = camera.getFrustumPlanes();

        CDLODQuadTree::LODSelectionOnStack<4096> cdlodSelection(camera.getPosition(), camera.getViewRange() * 30.0f,
                                                                planes.data(), settings.LODLevelDistanceRatio);

        cdlodQuadTree_.LODSelect(&cdlodSelection);

        //
        // Check if we have too small visibility distance that causes morph between LOD levels to be incorrect.
        if (cdlodSelection.IsVisDistTooSmall()) {
            assert(false && "Visibility distance might be too low for LOD morph to work correctly!");
        }
        //////////////////////////////////////////////////////////////////////////

        // cdlodRenderer_.Render();
    }
}

}  // namespace Ocean