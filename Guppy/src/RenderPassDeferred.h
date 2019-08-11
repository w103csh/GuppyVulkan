#ifndef RENDER_PASS_DEFERRED_H
#define RENDER_PASS_DEFERRED_H

#include <vulkan/vulkan.h>

#include "RenderPass.h"

namespace RenderPass {
struct CreateInfo;
class Handler;
namespace Deferred {

// MRT
class Deferred : public RenderPass::Base {
   public:
    Deferred(Handler& handler, const index&& offset);

    void record(const uint8_t frameIndex) override;
    void update() override;

   private:
    void createAttachmentsAndSubpasses() override;
    void createDependencies() override;
    void updateClearValues() override;
    void createFramebuffers() override;
};

}  // namespace Deferred
}  // namespace RenderPass

#endif  // !RENDER_PASS_DEFERRED_H
