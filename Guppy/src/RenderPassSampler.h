#ifndef SAMPLER_RENDER_PASS_H
#define SAMPLER_RENDER_PASS_H

#include "RenderPass.h"
#include "Shell.h"

namespace RenderPass {

class Handler;

class Sampler : public Base {
   public:
    Sampler(RenderPass::Handler& handler);

    void init() override;
    void record() override { assert(false); }

    void getSubmitResource(const uint8_t& frameIndex, SubmitResource& resource) const override;

   private:
    void createClearValues() override;
    void createAttachmentsAndSubpasses() override;
    void createDependencies() override;
    void createFramebuffers() override;
};

};  // namespace RenderPass

#endif  // !SAMPLER_RENDER_PASS_H
