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
#include "UIHandler.h"
#include "Shell.h"
#include "PipelineHandler.h"

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);  // TODO: shell logging
}

static void glfw_resize_callback(GLFWwindow* window, int w, int h) {
    fprintf(stderr, "Glfw resize\n");  //
    auto pShell = reinterpret_cast<Shell*>(glfwGetWindowUserPointer(window));
    pShell->resize_swapchain(w, h);
}

template <class T>
class ShellGLFW : public T {
    static_assert(std::is_base_of<Shell, T>::value, "T must be a derived from of Shell");

   public:
    ShellGLFW(Game& game) : T(game){};
    ~ShellGLFW(){};

    void run() override {
        setupImGui();

        double current_time = glfwGetTime();

        // Main loop
        while (!glfwWindowShouldClose(window_)) {
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            acquire_back_buffer();

            double now = glfwGetTime();
            double elapsed = now - current_time;

            InputHandler::updateInput(static_cast<float>(elapsed));
            add_game_time(static_cast<float>(elapsed));

            present_back_buffer();

            current_time = now;
        }

        vkDeviceWaitIdle(context().dev);

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        destroy_context();

        glfwDestroyWindow(window_);
        glfwTerminate();
    }

    void updateUIResources(DescriptorResources& desRes, PipelineResources& plRes) override {
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = context().instance;
        init_info.PhysicalDevice = context().physical_dev;
        init_info.Device = context().dev;
        init_info.QueueFamily = context().graphics_index;
        init_info.Queue = context().queues[context().graphics_index];
        init_info.PipelineCache = PipelineHandler::getPipelineCache();
        init_info.DescriptorPool = desRes.pool;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = (void (*)(VkResult))vk::assert_success;
        init_info.Subpass = static_cast<uint32_t>(PIPELINE_TYPE::UI);
        init_info.RasterizationSamples = context().num_samples;
        init_info.SampleShadingEnable = settings_.enable_sample_shading;
        init_info.MinSampleShading = settings_.enable_sample_shading ? MIN_SAMPLE_SHADING : 0.0f;

        ImGui_ImplVulkan_Init(&init_info, plRes.renderPass);
    }

   private:
    void setupImGui() {
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

        resize_swapchain(w, h, false);

        ImGui_ImplVulkan_SetFrameCount(context().image_count);

        windowData_.Surface = context().surface;
        windowData_.SurfaceFormat = context().surface_format;
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
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window_ = glfwCreateWindow(settings_.initial_width, settings_.initial_height, "Dear ImGui GLFW+Vulkan example", NULL, NULL);
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
};

#endif  // !SHELL_GLFW_H
