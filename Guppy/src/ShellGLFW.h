#ifndef SHELL_GLFW_H
#define SHELL_GLFW_H

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <type_traits>
#include <utility>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "Shell.h"

void glfw_error_callback(int error, const char* description);
void glfw_resize_callback(GLFWwindow* window, int w, int h);
void glfw_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

template <class TShell>
class ShellGLFW : public TShell {
    static_assert(std::is_base_of<Shell, TShell>::value, "T must be derived from of Shell");

   public:
    ShellGLFW(Game& game) : TShell(game), window_(nullptr) {}
    ~ShellGLFW() {}

    void run() override {
        // Setup window
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) {
            TShell::log(TShell::LogPriority::LOG_ERR, "GLFW: Failed to initialize\n");
            assert(false);
        }
        createWindow();

        // Setup Vulkan
        if (!glfwVulkanSupported()) {
            TShell::log(TShell::LogPriority::LOG_ERR, "GLFW: Vulkan Not Supported\n");
            assert(false);
        }

        setPlatformSpecificExtensions();
        TShell::initVk();

        TShell::init();

        // input listeners (set before imgui init because it installs it own callbacks)
        glfwSetCursorPosCallback(window_, glfw_cursor_pos_callback);
        glfwSetMouseButtonCallback(window_, glfw_mouse_button_callback);
        glfwSetKeyCallback(window_, glfw_key_callback);
        setupImGui();

        TShell::currentTime_ = glfwGetTime();

        // Main loop
        while (!glfwWindowShouldClose(window_)) {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your
            // inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those
            // two flags.
            glfwPollEvents();

            if (TShell::settings_.enable_directory_listener) TShell::checkDirectories();

            TShell::acquireBackBuffer();

            double now = glfwGetTime();
            double elapsed = now - TShell::currentTime_;
            TShell::currentTime_ = now;

            TShell::update(elapsed);

            TShell::onMouse(TShell::handlers_.pInput->getMouseInput());  // TODO: this stuff is all out of whack

            TShell::addGameTime(static_cast<float>(elapsed));

            TShell::presentBackBuffer();

            if (TShell::limitFramerate) {
                // TODO: this is inaccurate.
                auto Hz = static_cast<uint64_t>(1000 / TShell::framesPerSecondLimit);  // Hz
                if (TShell::settings_.enable_directory_listener) TShell::asyncAlert(Hz);
            } else {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
                if (TShell::settings_.enable_directory_listener) TShell::asyncAlert(0);
#endif
            }

            TShell::handlers_.pInput->clear();
        }

        vkDeviceWaitIdle(TShell::context().dev);

        TShell::destroy();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window_);

        TShell::destroyContext();

        glfwTerminate();
    }

   private:
    void setupImGui() {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        TShell::createContext();

        int w, h;
        // glfwGetFramebufferSize(window_, &w, &h); // Why is this so different on mac???
        glfwGetWindowSize(window_, &w, &h);
        glfwSetFramebufferSizeCallback(window_, glfw_resize_callback);

        TShell::resizeSwapchain(w, h, false);

        ImGui_ImplVulkan_SetFrameCount(TShell::context().imageCount);  // Remove this?
        // getUI()->getRenderPass()->init(ctx, settings_);

        // ImGui_ImplVulkanH_CreateWindowDataSwapChainAndFramebuffer(ctx.physical_dev, ctx.dev, &getUI()->getRenderPass(),
        //                                                          nullptr, w, h);

        windowData_.Surface = TShell::context().surface;
        windowData_.SurfaceFormat = TShell::context().surfaceFormat;
        windowData_.PresentMode = TShell::context().mode;
        windowData_.PresentMode = TShell::context().mode;

        // Setup Platform/Renderer bindings
        ImGui_ImplGlfw_InitForVulkan(window_, true);

        // Setup Style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsClassic();
    }

    void createWindow() override {
        // GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        // if (!PRIMARY_MONITOR) {
        //    int count;
        //    GLFWmonitor** monitors = glfwGetMonitors(&count);
        //    for (int i = 0; i < count; i++) {
        //        if (monitors[i] != monitor) {
        //            monitor = monitors[i];
        //            break;
        //        }
        //    }
        //}

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window_ = glfwCreateWindow(TShell::settings_.initial_width, TShell::settings_.initial_height,
                                   TShell::settings_.name.c_str(), NULL, NULL);
        glfwSetWindowUserPointer(window_, this);
    }

    VkSurfaceKHR createSurface(VkInstance instance) override {
        VkSurfaceKHR surface;
        vk::assert_success(glfwCreateWindowSurface(instance, window_, nullptr, &surface));
        return surface;
    };

    void setPlatformSpecificExtensions() override {
        uint32_t extensions_count = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);

        for (uint32_t i = 0; i < extensions_count; i++) TShell::instanceExtensions_.push_back(extensions[i]);

        TShell::setPlatformSpecificExtensions();
    }

    void quit() const override { glfwSetWindowShouldClose(window_, GLFW_TRUE); }

    GLFWwindow* window_;
    ImGui_ImplVulkanH_WindowData windowData_;
};

#endif  // !SHELL_GLFW_H
