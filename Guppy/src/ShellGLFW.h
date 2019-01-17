#ifndef SHELL_GLFW_H
#define SHELL_GLFW_H

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <type_traits>
#include <vulkan/vulkan.h>

#include "CmdBufHandler.h"
#include "Helpers.h"
#include "ImGuiUI.h"
#include "Shell.h"
#include "PipelineHandler.h"

void glfw_error_callback(int error, const char* description);
void glfw_resize_callback(GLFWwindow* window, int w, int h);
void glfw_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);
void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

template <class T>
class ShellGLFW : public T {
    static_assert(std::is_base_of<Shell, T>::value, "T must be derived from of Shell");

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
        // After window is created instantiate the UI class
        pUI_ = std::make_shared<ImGuiUI>(window_);

        // Setup Vulkan
        if (!glfwVulkanSupported()) {
            log(LOG_ERR, "GLFW: Vulkan Not Supported\n");
            assert(false);
        }

        setPlatformSpecificExtensions();
        init_vk();

        // input listeners (set before imgui init because it installs it own callbacks)
        glfwSetCursorPosCallback(window_, glfw_cursor_pos_callback);
        glfwSetMouseButtonCallback(window_, glfw_mouse_button_callback);
        glfwSetKeyCallback(window_, glfw_key_callback);

        setupImGui();

        double currentTime = glfwGetTime();

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

            if (settings_.enable_directory_listener) checkDirectories();

            acquireBackBuffer();

            double now = glfwGetTime();
            double elapsed = now - currentTime;

            InputHandler::updateInput(static_cast<float>(elapsed));
            onMouse(InputHandler::getMouseInput());  // TODO: this stuff is all out of whack

            addGameTime(static_cast<float>(elapsed));

            presentBackBuffer();

            currentTime = now;

#ifdef LIMIT_FRAMERATE
            // TODO: this is crude and inaccurate.
            DWORD Hz = static_cast<DWORD>(1000 / 10);  // Hz
            if (settings_.enable_directory_listener) asyncAlert(Hz);
#else
            if (settings_.enable_directory_listener) asyncAlert(0);
#endif

            InputHandler::clear();
        }

        vkDeviceWaitIdle(context().dev);

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        destroy_context();

        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    void initUI(VkRenderPass pass) override {
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = context().instance;
        init_info.PhysicalDevice = context().physical_dev;
        init_info.Device = context().dev;
        init_info.QueueFamily = context().graphics_index;
        init_info.Queue = context().queues[context().graphics_index];
        init_info.PipelineCache = Pipeline::Handler::getPipelineCache();
        init_info.DescriptorPool = Pipeline::Handler::getDescriptorPool();
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = (void (*)(VkResult))vk::assert_success;
        init_info.Subpass = 2;
        init_info.RasterizationSamples = context().samples;
        init_info.SampleShadingEnable = settings_.enable_sample_shading;
        init_info.MinSampleShading = settings_.enable_sample_shading ? MIN_SAMPLE_SHADING : 0.0f;

        ImGui_ImplVulkan_Init(&init_info, pass);
    }

    std::shared_ptr<UI> getUI() const override { return static_cast<std::shared_ptr<UI>>(pUI_); }

    void updateRenderPass() override {
        //
        assert(false);
    }

   private:
    void setupImGui() {
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        create_context();

        int w, h;
        glfwGetFramebufferSize(window_, &w, &h);
        glfwSetFramebufferSizeCallback(window_, glfw_resize_callback);

        resizeSwapchain(w, h, false);

        ImGui_ImplVulkan_SetFrameCount(context().imageCount);  // Remove this?

        const auto& ctx = context();
        // getUI()->getRenderPass()->init(ctx, settings_);

        // ImGui_ImplVulkanH_CreateWindowDataSwapChainAndFramebuffer(ctx.physical_dev, ctx.dev, &getUI()->getRenderPass(),
        //                                                          nullptr, w, h);

        windowData_.Surface = context().surface;
        windowData_.SurfaceFormat = context().surfaceFormat;
        windowData_.PresentMode = context().mode;
        windowData_.PresentMode = context().mode;

        // Setup Platform/Renderer bindings
        ImGui_ImplGlfw_InitForVulkan(window_, true);

        // Setup Style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsClassic();

        uploadFonts();
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
        window_ = glfwCreateWindow(settings_.initial_width, settings_.initial_height, settings_.name.c_str(), NULL, NULL);
        glfwSetWindowUserPointer(window_, this);
    }

    void uploadFonts() {
        ImGui_ImplVulkan_CreateFontsTexture(CmdBufHandler::graphics_cmd());
        CmdBufHandler::endCmd(CmdBufHandler::graphics_cmd());

        VkSubmitInfo end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &CmdBufHandler::graphics_cmd();
        vk::assert_success(vkQueueSubmit(CmdBufHandler::graphics_queue(), 1, &end_info, VK_NULL_HANDLE));

        vk::assert_success(vkDeviceWaitIdle(context().dev));
        ImGui_ImplVulkan_InvalidateFontUploadObjects();
    }

    VkSurfaceKHR createSurface(VkInstance instance) override {
        VkSurfaceKHR surface;
        vk::assert_success(glfwCreateWindowSurface(instance, window_, nullptr, &surface));
        return surface;
    };

    void ShellGLFW::setPlatformSpecificExtensions() {
        uint32_t extensions_count = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);

        for (uint32_t i = 0; i < extensions_count; i++) instance_extensions_.push_back(extensions[i]);

        T::setPlatformSpecificExtensions();
    }

    GLFWwindow* window_;
    ImGui_ImplVulkanH_WindowData windowData_;
    std::shared_ptr<ImGuiUI> pUI_;
};

#endif  // !SHELL_GLFW_H
