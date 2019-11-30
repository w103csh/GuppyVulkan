/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef UI_IMGUI_HANDLER_H
#define UI_IMGUI_HANDLER_H

#include <vulkan/vulkan.h>

#include "RenderPassImGui.h"
#include "UIHandler.h"

class Face;

namespace UI {

class ImGuiHandler : public UI::Handler {
   public:
    ImGuiHandler(Game* pGame)
        : UI::Handler(pGame),  //
          showDemoWindow_(false),
          showSelectionInfoWindow_(false) {}

    void frame() override;
    void draw(const VkCommandBuffer& cmd, const uint8_t frameIndex) override;

    // Application main menu (TODO: move this to a UI class)
    void appMainMenuBar();
    void menuFile();
    void menuShowWindows();
    void showSelectionInfoWindow(bool* p_open);

   private:
    void showFaceSelectionInfoText(const std::unique_ptr<Face>& pFace);

    bool showDemoWindow_;
    bool showSelectionInfoWindow_;
};

}  // namespace UI

#endif  // !UI_IMGUI_HANDLER_H
