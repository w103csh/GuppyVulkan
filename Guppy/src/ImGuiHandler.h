#ifndef IMGUI_UI_HANDLER_H
#define IMGUI_UI_HANDLER_H

#include <vulkan/vulkan.h>

#include "ImGuiRenderPass.h"
#include "UIHandler.h"

class Face;

namespace UI {

class ImGuiHandler : public UI::Handler {
   public:
    ImGuiHandler(Game* pGame)
        : UI::Handler(pGame, std::make_unique<ImGuiRenderPass>()),  //
          showDemoWindow_(false),
          showSelectionInfoWindow_(true) {}

    void init() override;
    void updateRenderPass(RenderPass::FrameInfo* pFrameInfo);

    void draw(uint8_t frameIndex) override;

    // Application main menu (TODO: move this to a UI class)
    void appMainMenuBar();
    void menuFile();
    void menuShowWindows();
    void showSelectionInfoWindow(bool* p_open);

   private:
    void reset() override;

    void showFaceSelectionInfoText(const std::unique_ptr<Face>& pFace);

    bool showDemoWindow_;
    bool showSelectionInfoWindow_;
};

}  // namespace UI

#endif  // !IMGUI_UI_HANDLER_H
