#ifndef SAMPLER_RENDER_PASS_H
#define SAMPLER_RENDER_PASS_H

#include "RenderPassDefault.h"
#include "Shell.h"
#include "Texture.h"

namespace RenderPass {

class Handler;

class Sampler : public Default {
   public:
    Sampler(RenderPass::Handler& handler, const uint32_t&& offset);

    void init() override;

   private:
    void createSampler();
    void createFramebuffers() override;

    void setSwapchainInfo() override {}

    std::shared_ptr<Texture::Base> pTexture_;
};

};  // namespace RenderPass

#endif  // !SAMPLER_RENDER_PASS_H
