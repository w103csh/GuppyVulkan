/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef OCEAN_H
#define OCEAN_H

#include <glm/glm.hpp>
#include <string_view>
#include <type_traits>
#include <vulkan/vulkan.hpp>

#include "ConstantsAll.h"
#include "DescriptorManager.h"
#include "HeightFieldFluid.h"
#include "ParticleBuffer.h"
#include "Pipeline.h"

// clang-format off
namespace Pass    { class Handler; }
namespace Texture { class Base; }
namespace Texture { class Handler; }
// clang-format on

#define OCEAN_USE_COMPUTE_QUEUE_DISPATCH true

namespace Ocean {

/**
 * Unfortunately the ocean data sample dimensions need to be square because of how the fft compute shader works currently.
 * The sample dimensions also need to be known here because of the pipeline creation order, and having to use a text
 * replacement routine for the local size value. Still not sure if specialization info could be used for something like that.
 * It didn't work the first time I tried it.
 */
constexpr uint32_t N = 256;
constexpr uint32_t M = N;
constexpr uint32_t FFT_LOCAL_SIZE = 64;
constexpr uint32_t FFT_WORKGROUP_SIZE = N / FFT_LOCAL_SIZE;
constexpr uint32_t DISP_LOCAL_SIZE = 32;
constexpr uint32_t DISP_WORKGROUP_SIZE = N / DISP_LOCAL_SIZE;

constexpr float T = 200.0f;  // wave repeat time
constexpr float g = 9.81f;   // gravity

struct SurfaceCreateInfo {
    SurfaceCreateInfo()
        : Lx(1000.0f),  //
          Lz(1000.0f),
          N(::Ocean::N),
          M(::Ocean::M),
          V(31.0f),
          omega(1.0f, 0.0f),
          l(1.0f),
          A(2e-5f),
          L(),
          lambda(-1.0f) {
        L = (V * V) / g;
    }
    float Lx;         // grid size (meters)
    float Lz;         // grid size (meters)
    uint32_t N;       // grid size (discrete Lx)
    uint32_t M;       // grid size (discrete Lz)
    float V;          // wind speed (meters/second)
    glm::vec2 omega;  // wind direction (normalized)
    float l;          // small wave cutoff (meters)
    float A;          // Phillips spectrum constant (wave amplitude?)
    float L;          // largest possible waves from continuous wind speed V
    float lambda;     // horizontal displacement scale factor
};

}  // namespace Ocean

// BUFFER VIEW
namespace BufferView {
namespace Ocean {
constexpr std::string_view FFT_BIT_REVERSAL_OFFSETS_N_ID = "Ocean FFT BRO N";
constexpr std::string_view FFT_BIT_REVERSAL_OFFSETS_M_ID = "Ocean FFT BRO M";
constexpr std::string_view FFT_TWIDDLE_FACTORS_ID = "Ocean FFT TF";
void MakeResources(Texture::Handler& handler, const ::Ocean::SurfaceCreateInfo& info);
}  // namespace Ocean
}  // namespace BufferView

// TEXTURE
namespace Texture {
class Handler;
struct CreateInfo;
namespace Ocean {
constexpr std::string_view WAVE_FOURIER_ID = "Ocean Wave & Fourier Data Texture";
constexpr std::string_view DISP_REL_ID = "Ocean Dispersion Relation Data Texture";
void MakeResources(Texture::Handler& handler, const ::Ocean::SurfaceCreateInfo& info);
#if OCEAN_USE_COMPUTE_QUEUE_DISPATCH
constexpr std::string_view DISP_REL_COPY_ID = "Ocean Dispersion Relation Copy Data Texture";
CreateInfo MakeCopyTexInfo(const uint32_t N, const uint32_t M);
#endif
}  // namespace Ocean
}  // namespace Texture

// SHADER
namespace Shader {
namespace Ocean {
extern const CreateInfo VERT_CREATE_INFO;
extern const CreateInfo DEFERRED_MRT_FRAG_CREATE_INFO;
}  // namespace Ocean
}  // namespace Shader

// UNIFORM DYNAMIC
namespace UniformDynamic {
namespace Ocean {
namespace SimulationDraw {
struct DATA {
    float lambda;  // horizontal displacement scale factor
};
struct CreateInfo : Buffer::CreateInfo {
    ::Ocean::SurfaceCreateInfo info;
};
class Base : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);
};
using Manager = Descriptor::Manager<Descriptor::Base, Base, std::shared_ptr>;
}  // namespace SimulationDraw
}  // namespace Ocean
}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
extern const CreateInfo OCEAN_DRAW_CREATE_INFO;
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {
class Handler;
namespace Ocean {

// WIREFRAME
class Wireframe : public Graphics {
   public:
    const bool DO_BLEND;
    const bool IS_DEFERRED;

    Wireframe(Handler& handler);

   private:
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
};

// SURFACE
class Surface : public Graphics {
   public:
    const bool DO_BLEND;
    const bool DO_TESSELLATE;
    const bool IS_DEFERRED;

    Surface(Handler& handler);

   private:
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
    void getTesselationInfoResources(CreateInfoResources& createInfoRes) override;
};

}  // namespace Ocean
}  // namespace Pipeline

// BUFFER
namespace Ocean {

struct CreateInfo : Particle::Buffer::CreateInfo {
    SurfaceCreateInfo info;
};

class Buffer : public Particle::Buffer::Base, public Obj3d::InstanceDraw {
   public:
    Buffer(Particle::Handler& handler, const Particle::Buffer::index&& offset, const CreateInfo* pCreateInfo,
           std::shared_ptr<Material::Base>& pMaterial, const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors,
           std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData);

    void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
              const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
              const uint8_t frameIndex) const override;
    void dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                  const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                  const uint8_t frameIndex) const override;

    // There is no more per-frame draw data.
    void update(const float time, const float elapsed, const uint32_t frameIndex) override {}

    GRAPHICS drawMode;

   private:
    void loadBuffers() override;
    void destroy() override;

    uint32_t normalOffset_;

    std::vector<HeightFieldFluid::VertexData> verticesHFF_;
    BufferResource verticesHFFRes_;
    std::vector<IndexBufferType> indicesWF_;
    BufferResource indexWFRes_;

    SurfaceCreateInfo info_;
};

static_assert(std::is_base_of<Particle::Buffer::Base, Buffer>::value,
              "If/when the base changes removed the assert condition \"NAME == \"Ocean Surface Buffer\"\
              from Particle::Buffer::Base constructor.");

}  // namespace Ocean

#endif  //! OCEAN_H