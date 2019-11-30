/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef RENDER_PASS_DEFERRED_H
#define RENDER_PASS_DEFERRED_H

#include <vulkan/vulkan.h>

#include "RenderPass.h"

namespace RenderPass {
struct CreateInfo;
class Handler;
namespace Deferred {

class Base : public RenderPass::Base {
   public:
    Base(Handler& handler, const index&& offset);

    void init() override;
    void record(const uint8_t frameIndex) override;
    void update() override;

   private:
    void createAttachments() override;
    void createSubpassDescriptions() override;
    void createDependencies() override;
    void updateClearValues() override;
    void createFramebuffers() override;

    uint32_t inputAttachmentOffset_;
    uint32_t inputAttachmentCount_;
    uint32_t combinePassIndex_;
    bool doSSAO_;
};

}  // namespace Deferred
}  // namespace RenderPass

#endif  // !RENDER_PASS_DEFERRED_H
