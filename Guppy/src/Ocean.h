/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef OCEAN_H
#define OCEAN_H

#include <glm/glm.hpp>
#include <string_view>
#include <vulkan/vulkan.hpp>

#include <CDLOD/Common.h>
#include <CDLOD/CDLODQuadTree.h>
#include <CDLOD/CDLODRenderer.h>

#include "ConstantsAll.h"
#include "DescriptorManager.h"
#include "HeightFieldFluid.h"
#include "OceanRenderer.h"
#include "ParticleBuffer.h"
#include "Pipeline.h"

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

struct SurfaceCreateInfo : public IHeightmapSource {
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
        // IHeightMapSource
        mapDims.SizeX = 40960.0f;
        mapDims.SizeY = 20480.0f;
        mapDims.SizeZ = 1200.0f;
        mapDims.MinX = -0.5f * mapDims.SizeX;  //-20480.0; CH
        mapDims.MinY = -0.5f * mapDims.SizeY;  //-10240.0; CH
        mapDims.MinZ = -0.5f * mapDims.SizeZ;  //-600.00;  CH
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

    // IHeightMapSource
    int GetSizeX() const override {
        // return static_cast<int>(Lx);
        return 4096;
    }
    int GetSizeY() const override {
        // return static_cast<int>(Lz);
        return 2048;
    }
    // I am not sure if I need to return meaningful values for below. Doing so will obviously be a challenge.
    unsigned short GetHeightAt(int x, int y) const override { return NormalizeForZ(0.0f); }
    void GetAreaMinMaxZ(int x, int y, int sizeX, int sizeY, unsigned short& minZ, unsigned short& maxZ) const override {
        // Just fix the values between +/-50.
        minZ = NormalizeForZ(-50.0f);
        maxZ = NormalizeForZ(50.0f);
    }
    // CDLOD wants to normalize dimension values to the range of an unsigned short.
    unsigned short NormalizeForX(float x) const {
        return static_cast<unsigned short>((x - mapDims.MinX) * 65535.0f / mapDims.SizeX);
    }
    unsigned short NormalizeForY(float y) const {
        return static_cast<unsigned short>((y - mapDims.MinY) * 65535.0f / mapDims.SizeY);
    }
    unsigned short NormalizeForZ(float z) const {
        return static_cast<unsigned short>((z - mapDims.MinZ) * 65535.0f / mapDims.SizeZ);
    }
    MapDimensions mapDims = {};
};

}  // namespace Ocean

// BUFFER VIEW
namespace BufferView {
namespace Ocean {
constexpr std::string_view FFT_BIT_REVERSAL_OFFSETS_N_ID = "Ocean FFT BRO N";
constexpr std::string_view FFT_BIT_REVERSAL_OFFSETS_M_ID = "Ocean FFT BRO M";
constexpr std::string_view FFT_TWIDDLE_FACTORS_ID = "Ocean FFT TF";
}  // namespace Ocean
}  // namespace BufferView

// TEXTURE
namespace Texture {
class Handler;
struct CreateInfo;
namespace Ocean {
constexpr std::string_view DATA_ID = "Ocean Data Texture";
void MakeTextures(Handler& handler, const ::Ocean::SurfaceCreateInfo& info);
}  // namespace Ocean
}  // namespace Texture

// SHADER
namespace Shader {
namespace Ocean {
extern const CreateInfo DISP_COMP_CREATE_INFO;
extern const CreateInfo FFT_COMP_CREATE_INFO;
extern const CreateInfo FFT_ROW_COMP_CREATE_INFO;
extern const CreateInfo FFT_COL_COMP_CREATE_INFO;
extern const CreateInfo VERT_CREATE_INFO;
extern const CreateInfo DEFERRED_MRT_FRAG_CREATE_INFO;
}  // namespace Ocean
}  // namespace Shader

// UNIFORM DYNAMIC
namespace UniformDynamic {
namespace Ocean {
namespace Simulation {
struct DATA {
    uint32_t nLog2, mLog2;  // log2 of discrete dimensions
    float lambda;           // horizontal displacement scale factor
    float t;                // time
};
struct CreateInfo : Buffer::CreateInfo {
    ::Ocean::SurfaceCreateInfo info;
};
class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);
    void updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) override;
};
}  // namespace Simulation
}  // namespace Ocean
}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
extern const CreateInfo OCEAN_DEFAULT_CREATE_INFO;
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {
class Handler;
namespace Ocean {

// DISPERSION
class Dispersion : public Compute {
   public:
    Dispersion(Handler& handler);

   private:
    void getShaderStageInfoResources(CreateInfoResources& createInfoRes);

    float omega0_;
};

// FFT
class FFT : public Compute {
   public:
    FFT(Handler& handler);
};

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

    void update(const float time, const float elapsed, const uint32_t frameIndex) override;

    virtual void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                      const uint8_t frameIndex) const override;
    void dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                  const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                  const uint8_t frameIndex) const override;

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
    Renderer::Basic renderer_;
};

}  // namespace Ocean

#endif  //! OCEAN_H