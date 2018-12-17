#ifndef IMGUI_H
#define IMGUI_H

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "UIHandler.h"

class ImGuiUI : public UI {
   public:
    ImGuiUI(GLFWwindow* window);

    void draw(VkCommandBuffer cmd, uint8_t frameIndex) override;
    void reset() override;

    // Application main menu
    void appMainMenuBar();
    void menuFile();
    void menuShowWindows();
    void showSelectionInfoWindow(bool* p_open);

   private:
    GLFWwindow* window_;

    bool showDemoWindow_;
    bool showSelectionInfoWindow_;
};

#endif  // !IMGUI_H
