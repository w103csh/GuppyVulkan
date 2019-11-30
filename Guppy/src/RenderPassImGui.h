/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef RENDER_PASS_IMGUI_H
#define RENDER_PASS_IMGUI_H

#include <vulkan/vulkan.h>

#include "ConstantsAll.h"
#include "RenderPass.h"

namespace RenderPass {

class Handler;

class ImGui : public Base {
   public:
    ImGui(Handler& handler, const uint32_t&& offset);

    void postCreate() override;
    void record(const uint8_t frameIndex) override;

   private:
    void createAttachments() override;
    void createSubpassDescriptions() override;
    void createDependencies() override;
    void updateClearValues() override;
};

}  // namespace RenderPass

#endif  // !RENDER_PASS_IMGUI_H
