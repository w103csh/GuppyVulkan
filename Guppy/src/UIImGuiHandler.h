/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef UI_IMGUI_HANDLER_H
#define UI_IMGUI_HANDLER_H

#include <vulkan/vulkan.hpp>

#include "RenderPassImGui.h"
#include "UIHandler.h"

class Face;
class Game;

namespace UI {

class ImGuiHandler : public UI::Handler {
   public:
    ImGuiHandler(Game* pGame);

    void frame() override;
    void draw(const vk::CommandBuffer& cmd, const uint8_t frameIndex) override;

    // Application main menu (TODO: move this to a UI class)
    void appMainMenuBar();
    void menuFile();
    void menuWater();
    void menuOcean();
    void menuShowWindows();
    void showSelectionInfoWindow(bool* p_open);

   private:
    void showFaceSelectionInfoText(const std::unique_ptr<Face>& pFace);

    bool showDemoWindow_;
    bool showSelectionInfoWindow_;
    struct {
        bool columns;
        bool wireframe;
        bool flat;
    } water_;
    struct {
        bool wireframe;
        bool wireframeTess;
        bool deferred;
        bool deferredTess;
    } ocean_;
};

}  // namespace UI

#endif  // !UI_IMGUI_HANDLER_H
