/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef RENDER_PASS_CUBE_MAP_H
#define RENDER_PASS_CUBE_MAP_H

#include <vector>
#include <vulkan/vulkan.h>

#include "RenderPass.h"

namespace RenderPass {
struct CreateInfo;
class Handler;
namespace CubeMap {

// BASE
class Base : public RenderPass::Base {
   protected:
    Base(Handler& handler, const index&& offset, const CreateInfo* pCreateInfo);

   private:
    void createAttachments() override;
    void createFramebuffers() override;
};

// SKYBOX NIGHT
class SkyboxNight : public Base {
   public:
    SkyboxNight(Handler& handler, const index&& offset);
    void record(const uint8_t frameIndex, const VkCommandBuffer& priCmd);
};

}  // namespace CubeMap
}  // namespace RenderPass

#endif  // !RENDER_PASS_SHADOW_H
