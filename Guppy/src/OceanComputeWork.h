/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef OCEAN_COMPUTE_WORK_H
#define OCEAN_COMPUTE_WORK_H

#include <array>
#include <memory>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "ComputeWork.h"
#include "ConstantsAll.h"
#include "DescriptorManager.h"
#include "Ocean.h"
#include "Pipeline.h"

// clang-format off
namespace Descriptor { class Base; }
// clang-format on

// SHADER
namespace Shader {
namespace Ocean {
extern const CreateInfo DISP_COMP_CREATE_INFO;
extern const CreateInfo FFT_COMP_CREATE_INFO;
extern const CreateInfo VERT_INPUT_COMP_CREATE_INFO;
}  // namespace Ocean
}  // namespace Shader

// UNIFORM DYNAMIC
namespace UniformDynamic {
namespace Ocean {
namespace SimulationDispatch {
struct DATA {
    glm::vec4 data0;   // [0] horizontal displacement scale factor
                       // [1] time
                       // [2] grid scale (Lx)
                       // [3] grid scale (Lz)
    glm::uvec2 data1;  // [0] log2 of discrete dimension N
                       // [1] log2 of discrete dimension M
};
struct CreateInfo : Buffer::CreateInfo {
    ::Ocean::SurfaceCreateInfo info;
};
class Base : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);
    void update(const float time);
};
using Manager = Descriptor::Manager<Descriptor::Base, Base, std::shared_ptr>;
}  // namespace SimulationDispatch
}  // namespace Ocean
}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
extern const CreateInfo OCEAN_DISPATCH_CREATE_INFO;
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
// VERTEX INPUT
class VertexInput : public Compute {
   public:
    VertexInput(Handler& handler);
};
}  // namespace Ocean
}  // namespace Pipeline

// COMPUTE WORK
namespace ComputeWork {
class Ocean : public Base {
   public:
    Ocean(Pass::Handler& handler, const index&& offset);

    void prepare() override;
    void record() override;
    const std::vector<Descriptor::Base*> getDynamicDataItems(const PIPELINE pipelineType) const override;

    static void dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                         const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                         const uint8_t frameIndex);

   private:
    void copyImage(const vk::CommandBuffer cmd, const uint8_t frameIndex);

    void init() override;
    void destroy() override;

    // Convenience pointers
    UniformDynamic::Ocean::SimulationDispatch::Base* pOcnSimDpch_;
    const std::vector<vk::Semaphore>* pRenderSignalSemaphores_;
    const Texture::Base* pVertInputTex_;
    std::array<const Texture::Base*, 3> pVertInputTexCopies_;
};
}  // namespace ComputeWork

#endif  //! OCEAN_COMPUTE_WORK_H