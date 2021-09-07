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
#include "OceanRenderer.h"
#include "RenderPassManager.h"
#include "Tessellation.h"
// HANDLERS
#include "LoadingHandler.h"
#include "PipelineHandler.h"
#include "PassHandler.h"
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
                                                  const vk::ImageUsageFlags usageFlags,
                                                  const std::vector<Sampler::LayerInfo> layerInfos) {
    return {
        name,
        {layerInfos, true, true},
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

Texture::CreateInfo makeDefaultVertInputSampCreateInfo(const std::string&& name, const uint32_t N, const uint32_t M,
                                                       const vk::ImageUsageFlags usageFlags,
                                                       const DESCRIPTOR descriptorType) {
    const std::vector<Sampler::LayerInfo> layerInfos = {
        {::Sampler::USAGE::POSITION},  // position
        {::Sampler::USAGE::NORMAL},    // normal
    };
    auto sampInfo = getDefaultOceanSampCreateInfo(name + " Sampler", N, M, usageFlags, layerInfos);
    sampInfo.type = SAMPLER::DEFAULT;
    return {name, {sampInfo}, false, false, descriptorType};
}

// This is no longer used. I'm keeping it around because I like the comments. I use VkGridMesh for this instead.
// NOTE: I finally figured out how to use an index buffer for tessellation, which makes this function extra not useful. The
// way you do a patch list with an index buffer is just make a list topology, and call vkCmdDrawIndexed the way you normally
// would! Ugh, its so obvious now, but it was not obvious for a very, very long time.
template <typename TVertex, typename TIndex>
void MakeHeightmapVertexInput(const uint32_t X,                     //
                              const uint32_t Y,                     //
                              std::vector<TVertex>& triStripVerts,  //
                              std::vector<TIndex>& triStripIdxs,    //
                              std::vector<TVertex>& patchListVerts  //
) {
    /*
     * Vulkan texture space uv's:
     *      (0,0)------------->(size.x,0)
     *        |                       |
     *        |                       |
     *        |                       |
     *        V                       V
     *      (0,size.y)-------->(size.x,size.y)
     *
     * Example: 4x4
     *
     *  Note: I use counterclockwise as default winding. Not sure why I do this, but this diagram follows that. Maybe I used
     *        counterclockwise because its value is zero making it the defacto default??
     *
     * 0--2--4--6  0xFF
     * | /| /| /|
     * |/ |/ |/ |
     * 1--3--5--7  0xFF
     * | /| /| /|
     * |/ |/ |/ |
     * 8--9--10-11 0xFF
     * | /| /| /|
     * |/ |/ |/ |
     * 12-13-14-15
     *
     */
    assert((X > 1) && (Y > 1));

    // bool doPatchList = (pPatchListVerts != nullptr);  // Patch list
    const uint32_t indicesPerRow = X * 2u;  // Triangle strip

    for (uint32_t y = 0; y < (Y - 1); y++) {
        const uint32_t previousRowOffset = ((y - 1u) *      // Triangle strip: Previous row
                                            indicesPerRow)  // * Indices per row
                                           + (y - 1u);      // + number of primitive reset markers.
        for (uint32_t x = 0; x < X; x++) {
            {  // Triangle strip
                if (y == 0) {
                    triStripVerts.push_back({x, y});
                    triStripIdxs.push_back(static_cast<uint32_t>(triStripIdxs.size()));
                    triStripVerts.push_back({x, y + 1u});
                    triStripIdxs.push_back(static_cast<uint32_t>(triStripIdxs.size()));
                } else {
                    // Odd indices from previous row.
                    triStripIdxs.push_back(triStripIdxs[previousRowOffset + (x * 2u) + 1]);
                    triStripVerts.push_back({x, y + 1u});
                    triStripIdxs.push_back(((y + 1) * X) + x);
                }
            }
            {  // Patch list
                if (x < (X - 1)) {
                    // Create a square with two triangles per iteration.
                    patchListVerts.push_back({x, y});            // 0
                    patchListVerts.push_back({x, y + 1u});       // 1
                    patchListVerts.push_back({x + 1u, y});       // 2
                    patchListVerts.push_back({x + 1u, y + 1u});  // 3
                    patchListVerts.push_back({x + 1u, y});       // 2
                    patchListVerts.push_back({x, y + 1u});       // 1 ... etc.
                }
            }
        }
        // Restart the strip each row.
        triStripIdxs.push_back(VB_INDEX_PRIMITIVE_RESTART);
    }
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

            {  // Wave vector data
                kx = 2.0f * glm::pi<float>() * n / info.Lx;
                kz = 2.0f * glm::pi<float>() * m / info.Lz;
                kMagnitude = sqrt(kx * kx + kz * kz);

                pWave[idx + 0] = kx;
                pWave[idx + 1] = kz;
                pWave[idx + 2] = kMagnitude;
                pWave[idx + 3] = sqrt(::Ocean::g * kMagnitude);
            }

            {  // Fourier domain data
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
        std::vector<Sampler::LayerInfo> layerInfos = {
            {::Sampler::USAGE::HEIGHT},    // fourier domain dispersion relation (height)
            {::Sampler::USAGE::NORMAL},    // fourier domain dispersion relation (slope)
            {::Sampler::USAGE::DONT_CARE}  // fourier domain dispersion relation (differential)
        };
        sampInfo = getDefaultOceanSampCreateInfo(std::string(DISP_REL_ID) + " Sampler", info.N, info.M,
                                                 vk::ImageUsageFlagBits::eStorage, layerInfos);
        texInfo = {std::string(DISP_REL_ID), {sampInfo}, false, false, STORAGE_IMAGE::PIPELINE};
        handler.make(&texInfo);

        // Vertex shader input
        texInfo = makeDefaultVertInputSampCreateInfo(
            std::string(VERT_INPUT_ID), info.N, info.M,
            (vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc), STORAGE_IMAGE::PIPELINE);
        handler.make(&texInfo);
    }
}
CreateInfo MakeCopyTexInfo(const uint32_t N, const uint32_t M) {
    auto texInfo = makeDefaultVertInputSampCreateInfo(
        std::string(VERT_INPUT_COPY_ID), N, M, (vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled),
        COMBINED_SAMPLER::PIPELINE);
    texInfo.perFramebuffer = true;
    return texInfo;
}
}  // namespace Ocean
}  // namespace Texture

// SHADER
namespace Shader {
namespace Ocean {
const CreateInfo VERT_CREATE_INFO = {
    SHADER::OCEAN_VERT,
    "Ocean Surface Vertex Shader",
    "ocean/vert.ocean.glsl",
    vk::ShaderStageFlagBits::eVertex,
};
const CreateInfo VERT_CDLOD_CREATE_INFO = {
    SHADER::OCEAN_CDLOD_VERT,
    "Ocean Surface Cdlod Vertex Shader",
    "ocean/vert.cdlod.ocean.glsl",
    vk::ShaderStageFlagBits::eVertex,
};
const CreateInfo DEFERRED_MRT_FRAG_CREATE_INFO = {
    SHADER::OCEAN_DEFERRED_MRT_FRAG,
    "Ocean Surface Deferred Multiple Render Target Fragment Shader",
    "ocean/frag.ocean.deferred.mrt.glsl",
    vk::ShaderStageFlagBits::eFragment,
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
        {{2, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Ocean::VERT_INPUT_COPY_ID}},
    },
};
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {
namespace Ocean {

void getDefaultInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    {  // binding description
        const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
        createInfoRes.bindDescs.push_back({});
        createInfoRes.bindDescs.back().binding = BINDING;
        createInfoRes.bindDescs.back().stride = sizeof(glm::vec2);
        createInfoRes.bindDescs.back().inputRate = vk::VertexInputRate::eVertex;

        // imageOffset
        createInfoRes.attrDescs.push_back({});
        createInfoRes.attrDescs.back().binding = BINDING;
        createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
        createInfoRes.attrDescs.back().format = vk::Format::eR32G32Sfloat;  // vec2
        createInfoRes.attrDescs.back().offset = 0;

        Instance::Cdlod::Ocean::DATA::getInputDescriptions(createInfoRes);
    }

    // bindings
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = createInfoRes.bindDescs.data();
    // attributes
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = vk::PipelineInputAssemblyStateCreateInfo{};
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = vk::PrimitiveTopology::eTriangleList;
}

// WIREFRAME
const CreateInfo OCEAN_WF_CREATE_INFO = {
    GRAPHICS::OCEAN_WF_DEFERRED,
    "Ocean Surface Wireframe (Deferred) Pipeline",
    {SHADER::OCEAN_VERT, SHADER::DEFERRED_MRT_COLOR_FRAG},
    {{DESCRIPTOR_SET::OCEAN_DRAW, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)}},
};
Wireframe::Wireframe(Handler& handler) : Graphics(handler, &OCEAN_WF_CREATE_INFO) {}
Wireframe::Wireframe(Handler& handler, const CreateInfo* pCreateInfo) : Graphics(handler, pCreateInfo) {}
void Wireframe::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}
void Wireframe::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    getDefaultInputAssemblyInfoResources(createInfoRes);
}
void Wireframe::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    Graphics::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.polygonMode = vk::PolygonMode::eLine;
    createInfoRes.rasterizationStateInfo.lineWidth = 1.0f;
    // createInfoRes.rasterizationStateInfo.cullMode = vk::CullModeFlagBits::eNone;
}

// WIREFRAME (TESSELLATION)
#if !(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
const CreateInfo OCEAN_WF_TESS_CREATE_INFO = {
    GRAPHICS::OCEAN_WF_TESS_DEFERRED,
    "Ocean Surface Wireframe Tessellation (Deferred) Pipeline",
    {
        SHADER::OCEAN_VERT,
        SHADER::PHONG_TRI_COLOR_TESC,
        SHADER::PHONG_TRI_COLOR_TESE,
        SHADER::DEFERRED_MRT_COLOR_FRAG,
    },
    {
        {DESCRIPTOR_SET::OCEAN_DRAW,
         (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eTessellationControl |
          vk::ShaderStageFlagBits::eTessellationEvaluation | vk::ShaderStageFlagBits::eFragment)},
        {DESCRIPTOR_SET::TESS_PHONG,
         (vk::ShaderStageFlagBits::eTessellationControl | vk::ShaderStageFlagBits::eTessellationEvaluation)},
    },
};
WireframeTess::WireframeTess(Handler& handler) : Wireframe(handler, &OCEAN_WF_TESS_CREATE_INFO) {}
void WireframeTess::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    Wireframe::getInputAssemblyInfoResources(createInfoRes);
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = vk::PrimitiveTopology::ePatchList;
}
void WireframeTess::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    Wireframe::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = vk::PrimitiveTopology::ePatchList;
}
void WireframeTess::getTessellationInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.useTessellationInfo = true;
    createInfoRes.tessellationStateInfo = vk::PipelineTessellationStateCreateInfo{};
    createInfoRes.tessellationStateInfo.patchControlPoints = 3;
}
#endif

// SURFACE
const CreateInfo OCEAN_SURFACE_CREATE_INFO = {
    GRAPHICS::OCEAN_SURFACE_DEFERRED,
    "Ocean Surface (Deferred) Pipeline",
    {SHADER::OCEAN_VERT, SHADER::OCEAN_DEFERRED_MRT_FRAG},
    {{DESCRIPTOR_SET::OCEAN_DRAW, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)}},
};
Surface::Surface(Handler& handler) : Graphics(handler, &OCEAN_SURFACE_CREATE_INFO) {}
Surface::Surface(Handler& handler, const CreateInfo* pCreateInfo) : Graphics(handler, pCreateInfo) {}
void Surface::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}
void Surface::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    getDefaultInputAssemblyInfoResources(createInfoRes);
}

// SURFACE (TESSELLATION)
#if !(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
const CreateInfo OCEAN_SURFACE_TESS_CREATE_INFO = {
    GRAPHICS::OCEAN_SURFACE_TESS_DEFERRED,
    "Ocean Surface Tessellation (Deferred) Pipeline",
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
SurfaceTess::SurfaceTess(Handler& handler) : Surface(handler, &OCEAN_SURFACE_TESS_CREATE_INFO) {}
void SurfaceTess::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    Surface::getInputAssemblyInfoResources(createInfoRes);
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = vk::PrimitiveTopology::ePatchList;
}
void SurfaceTess::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    Surface::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = vk::PrimitiveTopology::ePatchList;
}
void SurfaceTess::getTessellationInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.useTessellationInfo = true;
    createInfoRes.tessellationStateInfo = vk::PipelineTessellationStateCreateInfo{};
    createInfoRes.tessellationStateInfo.patchControlPoints = 3;
}
#endif

// WIREFRAME (CDLOD)
const CreateInfo OCEAN_WF_CDLOD_CREATE_INFO = {
    GRAPHICS::OCEAN_WF_CDLOD_DEFERRED,
    "Ocean Wireframe Cdlod (Deferred) Pipeline",
    {SHADER::OCEAN_CDLOD_VERT, SHADER::OCEAN_DEFERRED_MRT_FRAG},
    {
        {DESCRIPTOR_SET::OCEAN_DRAW, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)},
        {DESCRIPTOR_SET::CDLOD_DEFAULT, (vk::ShaderStageFlagBits::eVertex)},
    },
    {},
    {PUSH_CONSTANT::CDLOD},
};
WireframeCdlod::WireframeCdlod(Handler& handler) : Wireframe(handler, &OCEAN_WF_CDLOD_CREATE_INFO) {}

// SURFACE (CDLOD)
const CreateInfo OCEAN_SURFACE_CDLOD_CREATE_INFO = {
    GRAPHICS::OCEAN_SURFACE_CDLOD_DEFERRED,
    "Ocean Surface Cdlod (Deferred) Pipeline",
    {SHADER::OCEAN_CDLOD_VERT, SHADER::OCEAN_DEFERRED_MRT_FRAG},
    {
        {DESCRIPTOR_SET::OCEAN_DRAW, (vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment)},
        {DESCRIPTOR_SET::CDLOD_DEFAULT, (vk::ShaderStageFlagBits::eVertex)},
    },
    {},
    {PUSH_CONSTANT::CDLOD},
};
SurfaceCdlod::SurfaceCdlod(Handler& handler) : Surface(handler, &OCEAN_SURFACE_CDLOD_CREATE_INFO) {}

}  // namespace Ocean
}  // namespace Pipeline

// INSTANCE
namespace Instance {
namespace Cdlod {
namespace Ocean {
void DATA::getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes) {
    const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.bindDescs.push_back({});
    createInfoRes.bindDescs.back().binding = BINDING;
    createInfoRes.bindDescs.back().stride = sizeof(DATA);
    createInfoRes.bindDescs.back().inputRate = vk::VertexInputRate::eInstance;

    // data0
    auto offset = offsetof(DATA, data0);
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = vk::Format::eR32G32B32A32Sfloat;  // vec4
    createInfoRes.attrDescs.back().offset = static_cast<uint32_t>(offset);

    // data1
    offset = offsetof(DATA, data1);
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = vk::Format::eR32G32B32A32Sfloat;  // vec4
    createInfoRes.attrDescs.back().offset = static_cast<uint32_t>(offset);
}
Base::Base(const Buffer::Info&& info, DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)), Buffer::DataItem<DATA>(pData) {
    dirty = true;
}
}  // namespace Ocean
}  // namespace Cdlod
}  // namespace Instance

// GRAPHICS WORK
namespace GraphicsWork {

OceanSurface::OceanSurface(Pass::Handler& handler, const OceanCreateInfo* pCreateInfo, ::Ocean::Renderer* pRenderer)
    : Base(handler, pCreateInfo),
      pRenderer_(pRenderer),
      surfaceInfo_(pCreateInfo->surfaceInfo),
      gridMeshDims_(0),
      gridMesh_(handler.shell().context()),
      pInstanceData_(nullptr) {
    // Validate the surface info.
    assert(surfaceInfo_.N == surfaceInfo_.M);  // Needs to be square currently.
    assert(surfaceInfo_.N == ::Ocean::FFT_LOCAL_SIZE * ::Ocean::FFT_WORKGROUP_SIZE);
    assert(surfaceInfo_.N == ::Ocean::DISP_LOCAL_SIZE * ::Ocean::DISP_WORKGROUP_SIZE);
    assert(helpers::isPowerOfTwo(surfaceInfo_.N) && helpers::isPowerOfTwo(surfaceInfo_.M));

    drawMode = GRAPHICS::OCEAN_SURFACE_CDLOD_DEFERRED;
    status_ |= STATUS::PENDING_BUFFERS;
}

void OceanSurface::init() {
    {  // Create the instance data
        assert(surfaceInfo_.N == surfaceInfo_.M);
        gridMeshDims_ = 32;

        const uint32_t instanceCount = surfaceInfo_.N / gridMeshDims_;
        // This is just for testing. I removed the model matrix from the vertex input, so this is what centers the debug
        // ocean quad.
        const glm::vec2 offset = {-(surfaceInfo_.Lx / 2.0f), -(surfaceInfo_.Lz / 2.0f)};

        Instance::Cdlod::Ocean::CreateInfo instInfo = {};
        Instance::Cdlod::Ocean::DATA instData = {};

        // Scale (This is really uniform (dynamic) data)
        instData.data0.z = surfaceInfo_.Lx / instanceCount;
        instData.data0.w = surfaceInfo_.Lz / instanceCount;
        instData.data1.z = 1.0f / instanceCount;
        instData.data1.w = 1.0f / instanceCount;

        for (float y = 0; y < instanceCount; y++) {
            float v = glm::mix(0.0f, 1.0f, (y / instanceCount));
            for (float x = 0; x < instanceCount; x++) {
                float u = glm::mix(0.0f, 1.0f, (x / instanceCount));

                // Offset
                instData.data0.x = glm::mix(0.0f, surfaceInfo_.Lx, u) + offset.x;
                instData.data0.y = glm::mix(0.0f, surfaceInfo_.Lz, v) + offset.y;
                instData.data1.x = u;
                instData.data1.y = v;

                instInfo.data.push_back(instData);
            }
        }

        pInstanceData_ = pRenderer_->makeInstance(&instInfo);
    }
}

void OceanSurface::destroy() {
    pRenderer_ = nullptr;
    surfaceInfo_ = {};
    gridMeshDims_ = 0;
    gridMesh_.destroy();
    assert(pInstanceData_.use_count() == 1);
    pInstanceData_ = nullptr;
}

void OceanSurface::record(const PASS passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                          const vk::CommandBuffer& cmd) {
    const auto frameIndex = handler().passHandler().renderPassMgr().getFrameIndex();
    const auto& descSetBindData = getDescriptorSetBindData(pPipelineBindData->type);

    const auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);
    cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                           descSetBindData.descriptorSets[setIndex], descSetBindData.dynamicOffsets);

    switch (drawMode) {
        case GRAPHICS::OCEAN_WF_DEFERRED:
        case GRAPHICS::OCEAN_SURFACE_DEFERRED:
#if !(defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
        case GRAPHICS::OCEAN_WF_TESS_DEFERRED:
        case GRAPHICS::OCEAN_SURFACE_TESS_DEFERRED:
#endif
        case GRAPHICS::OCEAN_WF_CDLOD_DEFERRED:
        case GRAPHICS::OCEAN_SURFACE_CDLOD_DEFERRED: {
            const std::vector<vk::Buffer> buffers = {gridMesh_.GetVertexBuffer().buffer,
                                                     pInstanceData_->BUFFER_INFO.bufferInfo.buffer};
            const std::vector<vk::DeviceSize> offsets = {0, pInstanceData_->BUFFER_INFO.memoryOffset};
            cmd.bindVertexBuffers(0, buffers, offsets);
            cmd.bindIndexBuffer(gridMesh_.GetIndexBuffer().buffer, 0, vk::IndexType::eUint32);

            const int totalIndices = gridMesh_.GetDimensions() * gridMesh_.GetDimensions() * 2 * 3;
            cmd.drawIndexed(totalIndices, pInstanceData_->BUFFER_INFO.count, 0, 0, 0);
        } break;
        default: {
            assert(false);
        } break;
    }
}

void OceanSurface::load(std::unique_ptr<LoadingResource>& pLdgRes) {
    const auto& ctx = handler().shell().context();

    assert(surfaceInfo_.N == surfaceInfo_.M);
    gridMesh_.SetDimensions(gridMeshDims_);
    gridMesh_.CreateBuffers(*pLdgRes);
}

}  // namespace GraphicsWork