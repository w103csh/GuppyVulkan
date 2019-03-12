
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <glm/glm.hpp>

#include "ImGuiHandler.h"

#include "CommandHandler.h"
#include "DescriptorHandler.h"
#include "Face.h"
#include "Helpers.h"
#include "PipelineHandler.h"
#include "SceneHandler.h"

void UI::ImGuiHandler::init() {
    // if (pRenderPass != nullptr) pRenderPass->destroy(dev);
    // pRenderPass = std::move(pPass);

    // RENDER PASS
    RenderPass::InitInfo initInfo = {};
    initInfo = {};
    initInfo.format = shell().context().surfaceFormat.format;
    initInfo.commandCount = shell().context().imageCount;
    initInfo.waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    pPass_->init(shell().context(), settings(), &initInfo);

    // VULKAN INIT
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = shell().context().instance;
    init_info.PhysicalDevice = shell().context().physical_dev;
    init_info.Device = shell().context().dev;
    init_info.QueueFamily = shell().context().graphics_index;
    init_info.Queue = shell().context().queues[shell().context().graphics_index];
    init_info.PipelineCache = pipelineHandler().getPipelineCache();
    init_info.DescriptorPool = descriptorHandler().getPool();
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = (void (*)(VkResult))vk::assert_success;

    ImGui_ImplVulkan_Init(&init_info, pPass_->pass);

    // FONTS
    ImGui_ImplVulkan_CreateFontsTexture(commandHandler().graphicsCmd());
    vkEndCommandBuffer(commandHandler().graphicsCmd());

    VkSubmitInfo end_info = {};
    end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    end_info.commandBufferCount = 1;
    end_info.pCommandBuffers = &commandHandler().graphicsCmd();
    vk::assert_success(
        vkQueueSubmit(shell().context().queues[shell().context().graphics_index], 1, &end_info, VK_NULL_HANDLE));

    vk::assert_success(vkDeviceWaitIdle(shell().context().dev));

    // RESET GRAPHICS DEFAULT COMMAND
    vk::assert_success(vkResetCommandBuffer(commandHandler().graphicsCmd(), 0));
    commandHandler().beginCmd(commandHandler().graphicsCmd());

    ImGui_ImplVulkan_InvalidateFontUploadObjects();
}

void UI::ImGuiHandler::updateRenderPass(RenderPass::FrameInfo* pFrameInfo) {
    pPass_->createTarget(shell().context(), settings(), commandHandler(), pFrameInfo);
}

void UI::ImGuiHandler::reset() { pPass_->destroyTargetResources(shell().context().dev, commandHandler()); }

void UI::ImGuiHandler::destroy() { pPass_->destroy(shell().context().dev); }

void UI::ImGuiHandler::draw(uint8_t frameIndex) {
    // if (pPass->data.tests[frameIndex] == 0) {
    //    return;
    //}

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    appMainMenuBar();
    if (showDemoWindow_) ImGui::ShowDemoWindow(&showDemoWindow_);
    if (showSelectionInfoWindow_) showSelectionInfoWindow(&showSelectionInfoWindow_);

    // Rendering
    ImGui::Render();

    auto& cmd = pPass_->data.priCmds[frameIndex];
    pPass_->beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

    // Record Imgui Draw Data and draw funcs into command buffer
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd, frameIndex);

    pPass_->endPass(frameIndex);
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
        if (ImGui::BeginMenu("Show Windows")) {
            menuShowWindows();
            ImGui::EndMenu();
        }
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

    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::Separator();

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
