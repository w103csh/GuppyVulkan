/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "UIImGuiHandler.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <glm/glm.hpp>

#include <Common/Helpers.h>

#include "ComputeWorkManager.h"
#include "Face.h"
#include "HeightFieldFluid.h"
#include "Ocean.h"
#include "OceanComputeWork.h"
// HANDLERS
#include "CommandHandler.h"
#include "DescriptorHandler.h"
#include "ParticleHandler.h"
#include "PipelineHandler.h"
#include "PassHandler.h"
#include "SceneHandler.h"

UI::ImGuiHandler::ImGuiHandler(Game* pGame)
    : UI::Handler(pGame),  //
      showDemoWindow_(false),
      showSelectionInfoWindow_(false),
      water_(),
      ocean_() {}

void UI::ImGuiHandler::frame() {
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    appMainMenuBar();
    if (showDemoWindow_) ImGui::ShowDemoWindow(&showDemoWindow_);
    if (showSelectionInfoWindow_) showSelectionInfoWindow(&showSelectionInfoWindow_);

    // Rendering
    ImGui::Render();
}

void UI::ImGuiHandler::draw(const vk::CommandBuffer& cmd, const uint8_t frameIndex) {
    // Record Imgui Draw Data and draw funcs into command buffer
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

void UI::ImGuiHandler::appMainMenuBar() {
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
        // if (ImGui::BeginMenu("Water")) {
        //    menuWater();
        //    ImGui::EndMenu();
        //}
        if (ImGui::BeginMenu("Ocean Surface")) {
            menuOcean();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Show Windows")) {
            menuShowWindows();
            ImGui::EndMenu();
        }
        ImGui::Text("\t\t\t\t\tApplication average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);
        ImGui::EndMainMenuBar();
    }
}

void UI::ImGuiHandler::menuFile() {
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
        shell().quit();
    }
}

void UI::ImGuiHandler::menuWater() {
    auto pWater = static_cast<HeightFieldFluid::Buffer*>(particleHandler().getBuffer(particleHandler().waterOffset).get());
    water_.columns = false;
    water_.wireframe = false;
    water_.flat = false;
    switch (pWater->drawMode) {
        case GRAPHICS::HFF_CLMN_DEFERRED:
            water_.columns = true;
            break;
        case GRAPHICS::HFF_WF_DEFERRED:
            water_.wireframe = true;
            break;
        case GRAPHICS::HFF_OCEAN_DEFERRED:
            water_.flat = true;
            break;
        default:
            assert(false && "Unhandled or invalid case");
            exit(EXIT_FAILURE);
            break;
    }
    if (ImGui::MenuItem("Columns", nullptr, &water_.columns)) {
        pWater->drawMode = GRAPHICS::HFF_CLMN_DEFERRED;
    }
    if (ImGui::MenuItem("Wireframe", nullptr, &water_.wireframe)) {
        pWater->drawMode = GRAPHICS::HFF_WF_DEFERRED;
    }
    if (ImGui::MenuItem("Flat", nullptr, &water_.flat)) {
        pWater->drawMode = GRAPHICS::HFF_OCEAN_DEFERRED;
    }

    ImGui::Separator();

    static bool draw = pWater->getDraw();
    if (ImGui::Checkbox("Draw", &draw)) {
        pWater->toggleDraw();
    }
    static bool pause = pWater->getPaused();
    if (ImGui::Checkbox("Pause", &pause)) {
        pWater->togglePause();
    }
}

void UI::ImGuiHandler::menuOcean() {
    auto pBuffer = static_cast<Ocean::Buffer*>(particleHandler().getBuffer(particleHandler().waterOffset).get());

    ocean_.wireframe = false;
    ocean_.wireframeTess = false;
    ocean_.deferred = false;
    ocean_.deferredTess = false;

    switch (pBuffer->drawMode) {
        case GRAPHICS::OCEAN_WF_DEFERRED:
            ocean_.wireframe = true;
            break;
        case GRAPHICS::OCEAN_WF_TESS_DEFERRED:
            ocean_.wireframeTess = true;
            break;
        case GRAPHICS::OCEAN_SURFACE_DEFERRED:
            ocean_.deferred = true;
            break;
        case GRAPHICS::OCEAN_SURFACE_TESS_DEFERRED:
            ocean_.deferredTess = true;
            break;
        default:
            assert(false && "Unhandled or invalid case");
            exit(EXIT_FAILURE);
            break;
    }

    if (ImGui::MenuItem("Default", nullptr, &ocean_.deferred)) {
        pBuffer->drawMode = GRAPHICS::OCEAN_SURFACE_DEFERRED;
    }
    if (ImGui::MenuItem("Wireframe", nullptr, &ocean_.wireframe)) {
        pBuffer->drawMode = GRAPHICS::OCEAN_WF_DEFERRED;
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Default Tessellated", nullptr, &ocean_.deferredTess)) {
        pBuffer->drawMode = GRAPHICS::OCEAN_SURFACE_TESS_DEFERRED;
    }
    if (ImGui::MenuItem("Wireframe Tessellated", nullptr, &ocean_.wireframeTess)) {
        pBuffer->drawMode = GRAPHICS::OCEAN_WF_TESS_DEFERRED;
    }

    ImGui::Separator();

    // static bool draw = pOcean->getDraw();
    // if (ImGui::Checkbox("Draw", &draw)) {
    //    pOcean->toggleDraw();
    //}

    auto pWork = static_cast<ComputeWork::Ocean*>(passHandler().compWorkMgr().getWork(COMPUTE_WORK::OCEAN).get());

    static bool pause = pWork->getPaused();
    if (ImGui::Checkbox("Pause", &pause)) {
        pWork->onTogglePause();
    }
}

void UI::ImGuiHandler::menuShowWindows() {
    if (ImGui::MenuItem("Selection Information", nullptr, &showSelectionInfoWindow_)) {
    }
    if (ImGui::MenuItem("Demo Window", nullptr, &showDemoWindow_)) {
    }
}

void UI::ImGuiHandler::showSelectionInfoWindow(bool* p_open) {
    // We specify a default position/size in case there's no data in the .ini file. Typically this isn't required! We only do
    // it to make the Demo applications a little more welcoming.
    ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Selection Information", p_open, 0)) {
        // Early out if the window is collapsed, as an optimization.
        ImGui::End();
        return;
    }

    // ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
    //            ImGui::GetIO().Framerate);
    // ImGui::Separator();

    ImGui::Text("FACES:");
    ImGui::Separator();
    showFaceSelectionInfoText(sceneHandler().getActiveScene()->getFaceSelection());

    // End of showSelectionInfoWindow()
    ImGui::End();
}

void UI::ImGuiHandler::showFaceSelectionInfoText(const std::unique_ptr<Face>& pFace) {
    if (pFace == nullptr) {
        ImGui::Text("No face...");
        return;
    }

    // MISCELLANEOUS
    ImGui::Text("Miscellaneous:");
    ImGui::Columns(2, "vertex_misc_columns", false);  // 2-ways, no border
    ImGui::SetColumnWidth(0, 200.0f), ImGui::Indent();

    auto calcNormal = helpers::triangleNormal((*pFace)[0].position, (*pFace)[1].position, (*pFace)[2].position);
    ImGui::Text("Calculated normal:"), ImGui::NextColumn();
    ImGui::Text("(%.5f, %.5f, %.5f)", calcNormal.x, calcNormal.y, calcNormal.z), ImGui::NextColumn();

    ImGui::Unindent(), ImGui::Columns(1);

    // VERTEX
    ImGui::Text("Vertex:"), ImGui::Separator();
    ImGui::Columns(2, "vertex_columns", true);  // 2-ways, no border
    ImGui::SetColumnWidth(0, 200.0f), ImGui::Indent();
    for (uint8_t i = 0; i < 3; i++) {
        const auto& v = (*pFace)[i];

        ImGui::Text("Index:"), ImGui::NextColumn();
        ImGui::Text("%d", pFace->getIndex(i)), ImGui::NextColumn();

        ImGui::Text("Position:"), ImGui::NextColumn();
        ImGui::Text("(%.5f, %.5f, %.5f)", v.position.x, v.position.y, v.position.z), ImGui::NextColumn();

        ImGui::Text("Normal:"), ImGui::NextColumn();
        ImGui::Text("(%.5f, %.5f, %.5f)", v.normal.x, v.normal.y, v.normal.z), ImGui::NextColumn();

        ImGui::Text("Color:"), ImGui::NextColumn();
        ImGui::Text("(%.5f, % .5f, %.5f, %.5f)", v.color.x, v.color.y, v.color.z, v.color.w), ImGui::NextColumn();

        ImGui::Text("Tex coord:"), ImGui::NextColumn();
        ImGui::Text("(%.5f, %.5f)", v.texCoord.x, v.texCoord.y), ImGui::NextColumn();

        ImGui::Text("Tangent:"), ImGui::NextColumn();
        ImGui::Text("(%.5f, %.5f, %.5f)", v.tangent.x, v.tangent.y, v.tangent.z), ImGui::NextColumn();

        ImGui::Text("Binormal:"), ImGui::NextColumn();
        ImGui::Text("(%.5f, %.5f, %.5f)", v.binormal.x, v.binormal.y, v.binormal.z), ImGui::NextColumn();

        ImGui::Text("Smoothing group:"), ImGui::NextColumn();
        ImGui::Text("%d", v.smoothingGroupId), ImGui::NextColumn();

        ImGui::Separator();
    }
    ImGui::Unindent(), ImGui::Columns(1);
}
