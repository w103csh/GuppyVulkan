/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef RENDER_PASS_SCREEN_SPACE_H
#define RENDER_PASS_SCREEN_SPACE_H

#include <vulkan/vulkan.h>

#include "ConstantsAll.h"
#include "RenderPass.h"

namespace RenderPass {

struct CreateInfo;
class Handler;

namespace ScreenSpace {

extern const CreateInfo CREATE_INFO;

// BASE
class Base : public RenderPass::Base {
   public:
    Base(Handler& handler, const index&& offset, const CreateInfo* pCreateInfo);

    virtual void init() override;
    void update(const std::vector<Descriptor::Base*> pDynamicItems = {}) override;
    void record(const uint8_t frameIndex) override;

   private:
};

// BRIGHT
class Bright : public RenderPass::ScreenSpace::Base {
   public:
    Bright(Handler& handler, const index&& offset);
    void record(const uint8_t frameIndex) override {}
};

// BLUR
class BlurA : public RenderPass::ScreenSpace::Base {
   public:
    BlurA(Handler& handler, const index&& offset);
    void record(const uint8_t frameIndex) override {}
};

class BlurB : public RenderPass::ScreenSpace::Base {
   public:
    BlurB(Handler& handler, const index&& offset);
    void record(const uint8_t frameIndex) override {}
};

// HDR LOG
class HdrLog : public RenderPass::ScreenSpace::Base {
   public:
    HdrLog(Handler& handler, const index&& offset);

    void record(const uint8_t frameIndex) override {}
    void downSample(const VkCommandBuffer& priCmd, const uint8_t frameIndex);

   private:
    std::map<uint8_t, bool> passFlags_;
    std::vector<VkCommandBuffer> transfCmds_;
};

}  // namespace ScreenSpace

}  // namespace RenderPass

#endif  // !RENDER_PASS_SCREEN_SPACE_H
