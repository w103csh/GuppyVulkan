/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef RENDER_PASS_SHADOW_H
#define RENDER_PASS_SHADOW_H

#include <memory>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#include "RenderPass.h"

namespace RenderPass {
struct CreateInfo;
class Handler;
namespace Shadow {

// BASE
class Base : public RenderPass::Base {
   protected:
    Base(Handler& handler, const index&& offset, const CreateInfo* pCreateInfo);

   private:
    void createAttachments() override;
    void createDependencies() override;
    void createImageResources() override {}
    void createDepthResource() override {}
    void createFramebuffers() override;
    void record(const uint8_t) override { assert(false); }

   public:
    void record(const uint8_t frameIndex, const PASS& surrogatePassType, std::vector<PIPELINE>& surrogatePipelineTypes,
                const vk::CommandBuffer& priCmd);
};

// DEFAULT
class Default : public RenderPass::Shadow::Base {
   public:
    Default(Handler& handler, const index&& offset);
};

}  // namespace Shadow
}  // namespace RenderPass

#endif  // !RENDER_PASS_SHADOW_H
