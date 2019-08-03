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
extern const CreateInfo SAMPLER_CREATE_INFO;

// BASE

class Base : public RenderPass::Base {
   public:
    Base(Handler& handler, const index&& offset, const CreateInfo* pCreateInfo);

    virtual void init() override;
    void update() override;
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

class HdrLog : public RenderPass::Base {
   public:
    HdrLog(Handler& handler, const index&& offset);

    virtual void init() override;

    void record(const uint8_t frameIndex) override;

    bool submitDownSample(const uint8_t frameIndex, RenderPass::SubmitResource& submitResource);
    void getSemaphore(const uint8_t frameIndex, RenderPass::SubmitResource& submitResource);
    void read(const uint8_t firstIndex);

   private:
    void prepareDownSample(const uint32_t imageCount);
    void downSample(const VkCommandBuffer& cmd, const uint8_t frameIndex);
    void downSample2(const VkCommandBuffer& cmd, const uint8_t frameIndex);

    bool hasTrasfCmd_;
    bool wasSubmitted_;
    int8_t firstThing_;
    std::vector<RenderPass::SubmitResource> transfSubmitResources_;
    std::vector<VkSemaphore> transfSemaphores_;
    VkPipelineStageFlags transfWaitDstStgMask_;
    std::vector<VkCommandBuffer> transfCmds_;
    std::vector<VkFence> transfFences_;
    VkImageLayout blitBCurrentLayout_;
};

}  // namespace ScreenSpace

}  // namespace RenderPass

#endif  // !RENDER_PASS_SCREEN_SPACE_H
