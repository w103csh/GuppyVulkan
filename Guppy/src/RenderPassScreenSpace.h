#ifndef RENDER_PASS_SCREEN_SPACE_H
#define RENDER_PASS_SCREEN_SPACE_H

#include <vulkan/vulkan.h>

#include "ConstantsAll.h"
#include "Mesh.h"
#include "RenderPass.h"

namespace RenderPass {

struct CreateInfo;
class Handler;

namespace ScreenSpace {

extern const CreateInfo CREATE_INFO;
extern const CreateInfo SAMPLER_CREATE_INFO;

class Base : public RenderPass::Base {
   public:
    Base(Handler& handler, const uint32_t&& offset, const CreateInfo* pCreateInfo);

    virtual void init(bool isFinal = false) override;

    void record(const uint8_t frameIndex) override;

   private:
    Mesh::INDEX defaultRenderPlaneIndex_;
};

}  // namespace ScreenSpace

}  // namespace RenderPass

#endif  // !RENDER_PASS_SCREEN_SPACE_H
