/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef RENDER_PASS_SCREEN_SPACE_H
#define RENDER_PASS_SCREEN_SPACE_H

#include <vulkan/vulkan.hpp>

#include "ConstantsAll.h"
#include "RenderPass.h"

// clang-format off
namespace Pass { class Handler; }
// clang-format on

namespace RenderPass {
struct CreateInfo;
namespace ScreenSpace {

extern const CreateInfo CREATE_INFO;

// BASE
class Base : public RenderPass::Base {
   public:
    Base(Pass::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo);

    virtual void init() override;
    void update(const std::vector<Descriptor::Base*> pDynamicItems = {}) override;
    void record(const uint8_t frameIndex) override;

   private:
};

// BRIGHT
class Bright : public RenderPass::ScreenSpace::Base {
   public:
    Bright(Pass::Handler& handler, const index&& offset);
    void record(const uint8_t frameIndex) override {}
};

// BLUR
class BlurA : public RenderPass::ScreenSpace::Base {
   public:
    BlurA(Pass::Handler& handler, const index&& offset);
    void record(const uint8_t frameIndex) override {}
};

class BlurB : public RenderPass::ScreenSpace::Base {
   public:
    BlurB(Pass::Handler& handler, const index&& offset);
    void record(const uint8_t frameIndex) override {}
};

// HDR LOG
class HdrLog : public RenderPass::ScreenSpace::Base {
   public:
    HdrLog(Pass::Handler& handler, const index&& offset);

    void record(const uint8_t frameIndex) override {}
    void downSample(const vk::CommandBuffer& priCmd, const uint8_t frameIndex);

   private:
    std::map<uint8_t, bool> passFlags_;
    std::vector<vk::CommandBuffer> transfCmds_;
};

}  // namespace ScreenSpace
}  // namespace RenderPass

#endif  // !RENDER_PASS_SCREEN_SPACE_H
