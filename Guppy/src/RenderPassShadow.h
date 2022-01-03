/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef RENDER_PASS_SHADOW_H
#define RENDER_PASS_SHADOW_H

#include <memory>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "RenderPass.h"

// clang-format off
namespace Pass { class Handler; }
// clang-format on

namespace RenderPass {
struct CreateInfo;
namespace Shadow {

// BASE
class Base : public RenderPass::Base {
   protected:
    Base(Pass::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo);

   private:
    void createAttachments() override;
    void createDependencies() override;
    void createImageResources() override {}
    void createDepthResource() override {}
    void createFramebuffers() override;
    void record(const uint8_t) override { assert(false); }

   public:
    virtual void record(const uint8_t frameIndex, const RENDER_PASS& surrogatePassType,
                        std::vector<PIPELINE>& surrogatePipelineTypes, const vk::CommandBuffer& priCmd) = 0;
};

// DEFAULT
class Default : public RenderPass::Shadow::Base {
   public:
    Default(Pass::Handler& handler, const index&& offset);

    void record(const uint8_t frameIndex, const RENDER_PASS& surrogatePassType,
                std::vector<PIPELINE>& surrogatePipelineTypes, const vk::CommandBuffer& priCmd) override;

    void update(const std::vector<Descriptor::Base*> pDynamicItems = {}) override;
};

// CUBEMAP
class Cube : public RenderPass::Shadow::Base {
   public:
    Cube(Pass::Handler& handler, const index&& offset);

    void record(const uint8_t frameIndex, const RENDER_PASS& surrogatePassType,
                std::vector<PIPELINE>& surrogatePipelineTypes, const vk::CommandBuffer& priCmd) override;
};

}  // namespace Shadow
}  // namespace RenderPass

#endif  // !RENDER_PASS_SHADOW_H
