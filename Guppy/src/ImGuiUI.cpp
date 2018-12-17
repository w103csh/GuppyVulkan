

#include "ImGuiUI.h"

ImGuiUI::ImGuiUI(GLFWwindow* window) : window_(window), showDemoWindow_(false), showSelectionInfoWindow_(false) {}

void ImGuiUI::draw(VkCommandBuffer cmd, uint8_t frameIndex) {
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    appMainMenuBar();
    if (showDemoWindow_) ImGui::ShowDemoWindow(&showDemoWindow_);
    if (showSelectionInfoWindow_) showSelectionInfoWindow(&showSelectionInfoWindow_);

    // Rendering
    ImGui::Render();

    // Record Imgui Draw Data and draw funcs into command buffer
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd, frameIndex);
}

void ImGuiUI::reset() {
    // vkFreeCommandBuffers(ctx_.dev, CmdBufHandler::present_cmd_pool(), 1, &inst_.cmd_);
}

void ImGuiUI::appMainMenuBar() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            menuFile();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit")) {
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {
            }
            if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {
            }  // Disabled item
            ImGui::Separator();
            if (ImGui::MenuItem("Cut", "CTRL+X")) {
            }
            if (ImGui::MenuItem("Copy", "CTRL+C")) {
            }
            if (ImGui::MenuItem("Paste", "CTRL+V")) {
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Show Windows")) {
            menuShowWindows();
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}

void ImGuiUI::menuFile() {
    // ImGui::MenuItem("(dummy menu)", NULL, false, false);
    if (ImGui::MenuItem("New")) {
    }
    if (ImGui::MenuItem("Open", "Ctrl+O")) {
    }
    if (ImGui::BeginMenu("Open Recent")) {
        // ImGui::MenuItem("fish_hat.c");
        // ImGui::MenuItem("fish_hat.inl");
        // ImGui::MenuItem("fish_hat.h");
        // if (ImGui::BeginMenu("More..")) {
        //    ImGui::MenuItem("Hello");
        //    ImGui::MenuItem("Sailor");
        //    if (ImGui::BeginMenu("Recurse..")) {
        //        ShowExampleMenuFile();
        //        ImGui::EndMenu();
        //    }
        //    ImGui::EndMenu();
        //}
        ImGui::EndMenu();
    }
    if (ImGui::MenuItem("Save", "Ctrl+S")) {
    }
    if (ImGui::MenuItem("Save As..")) {
    }
    if (ImGui::MenuItem("Quit", "Alt+F4")) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }
}

void ImGuiUI::menuShowWindows() {
    if (ImGui::MenuItem("Selection Information", nullptr, &showSelectionInfoWindow_)) {
    }
    if (ImGui::MenuItem("Demo Window", nullptr, &showDemoWindow_)) {
    }
}

void ImGuiUI::showSelectionInfoWindow(bool* p_open) {

    // We specify a default position/size in case there's no data in the .ini file. Typically this isn't required! We only do
    // it to make the Demo applications a little more welcoming.
    ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Selection Information", p_open, 0)) {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }

    // End of showSelectionInfoWindow()
    ImGui::End();
}
