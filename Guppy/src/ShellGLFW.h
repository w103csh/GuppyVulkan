#ifndef SHELL_GLFW_H
#define SHELL_GLFW_H

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "MyShell.h"

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);  // TODO: shell logging
}

static void glfw_resize_callback(GLFWwindow*, int w, int h) {
    fprintf(stderr, "Glfw resize\n");  //
}

template <class T>
class ShellGLFW : public T {
   public:
    ShellGLFW(Game& game) : T(game){};
    ~ShellGLFW(){};

    void run() override {
        // Setup window
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) {
            log(LOG_ERR, "GLFW: Failed to initialize\n");
            assert(false);
        }

        createWindow();

        // Setup Vulkan
        if (!glfwVulkanSupported()) {
            log(LOG_ERR, "GLFW: Vulkan Not Supported\n");
            assert(false);
        }

        setPlatformSpecificExtensions();
        init_vk();

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;

        create_context();

        int w, h;
        glfwGetFramebufferSize(window_, &w, &h);
        glfwSetFramebufferSizeCallback(window_, glfw_resize_callback);

        resize_swapchain(settings_.initial_width, settings_.initial_height, false);

        // SetupVulkanWindowData(wd, surface, w, h);

        // Win32Timer timer;
        // double current_time = timer.get();
        //
        //        while (true) {
        //            bool quit = false;
        //
        //            assert(settings_.animate);
        //
        //            // process all messages
        //            MSG msg;
        //            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        //                if (msg.message == WM_QUIT) {
        //                    quit = true;
        //                    break;
        //                }
        //
        //                TranslateMessage(&msg);
        //                DispatchMessage(&msg);
        //            }
        //
        //            if (settings_.enable_directory_listener) CheckDirectories();
        //
        //            if (quit) {
        //                break;
        //            } else if (minimized_) {
        //                // TODO: somehow pause...
        //                // std::unique_lock<std::mutex> lock(mtx_);
        //                // while (minimized_) pause_.wait(lock);
        //            } else {
        //                acquire_back_buffer();
        //
        //                // TODO: simplify this?
        //                double now = timer.get();
        //                double elapsed = now - current_time;
        //                current_time = now;
        //
        //                InputHandler::updateInput(static_cast<float>(elapsed));
        //                add_game_time(static_cast<float>(elapsed));
        //
        //                present_back_buffer();
        //            }
        //
        //#ifdef LIMIT_FRAMERATE
        //            // TODO: this is crude and inaccurate.
        //            DWORD Hz = static_cast<DWORD>(1000 / 10);  // 30Hz
        //            if (settings_.enable_directory_listener) AsyncAlert(Hz);
        //#else
        //            if (settings_.enable_directory_listener) AsyncAlert(0);
        //#endif
        //        }
        //
        //        // Free any directory listening handles
        //        for (auto& dirInst : dirInsts_) {
        //            CloseHandle(dirInst.hDir);
        //        }
        //
        //        destroy_context();
        //
        //        DestroyWindow(hwnd_);
    }

   private:
    // VkBool32 canPresent(VkPhysicalDevice phy, uint32_t queue_family) override { return sh_.canPresent(phy, queue_family); };
    VkSurfaceKHR createSurface(VkInstance instance) override {
        VkSurfaceKHR surface;
        vk::assert_success(glfwCreateWindowSurface(instance, window_, nullptr, &surface));
        return surface;
    };
    // PFN_vkGetInstanceProcAddr load_vk() override { return sh_.load_vk(); };
    // void quit() override { sh_.run(); };
    // void watchDirectory(std::string dir, std::function<void(std::string)> callback) override { sh_.watchDirectory(dir, callback);
    // };

    void ShellGLFW::setPlatformSpecificExtensions() {
        uint32_t extensions_count = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);

        for (uint32_t i = 0; i < extensions_count; i++) instance_extensions_.push_back(extensions[i]);

        T::setPlatformSpecificExtensions();
    }

    void createWindow() override {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window_ = glfwCreateWindow(settings_.initial_width, settings_.initial_height, "Dear ImGui GLFW+Vulkan example", NULL, NULL);
    }

    GLFWwindow* window_;
    ImGui_ImplVulkanH_WindowData* windowData_;
};

#endif  // SHELL_GLFW_H
