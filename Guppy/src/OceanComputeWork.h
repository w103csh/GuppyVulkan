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
#include "Pipeline.h"

// SHADER
namespace Shader {
namespace Ocean {
extern const CreateInfo DISP_COMP_CREATE_INFO;
extern const CreateInfo FFT_COMP_CREATE_INFO;
}  // namespace Ocean
}  // namespace Shader

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
    void init() override;
    void copyDispRelImg(const vk::CommandBuffer cmd, const uint8_t frameIndex);

    // Convenience pointers
    const std::vector<vk::Semaphore>* pRenderSignalSemaphores_;
    Texture::Base* pDispRelTex_;
    std::array<Texture::Base*, 3> pDispRelTexCopies_;
};
}  // namespace ComputeWork

#endif  //! OCEAN_COMPUTE_WORK_H