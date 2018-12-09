#ifndef IMGUI_H
#define IMGUI_H

#include <vulkan/vulkan.h>

#include "UIHandler.h"

class ImGuiUI : public UI {
   public:
    void draw(VkCommandBuffer cmd, uint8_t frameIndex) override;
    void reset() override;
};

#endif  // !IMGUI_H
