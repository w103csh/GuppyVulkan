/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef RENDER_PASS_CUBE_MAP_H
#define RENDER_PASS_CUBE_MAP_H

#include <vector>
#include <vulkan/vulkan.hpp>

#include "RenderPass.h"

// clang-format off
namespace Pass { class Handler; }
// clang-format on

namespace RenderPass {
struct CreateInfo;
namespace CubeMap {

// BASE
class Base : public RenderPass::Base {
   protected:
    Base(Pass::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo);

   private:
    void createAttachments() override;
    void createFramebuffers() override;
};

// SKYBOX NIGHT
class SkyboxNight : public Base {
   public:
    SkyboxNight(Pass::Handler& handler, const index&& offset);
    void record(const uint8_t frameIndex, const vk::CommandBuffer& priCmd);
};

}  // namespace CubeMap
}  // namespace RenderPass

#endif  // !RENDER_PASS_SHADOW_H
