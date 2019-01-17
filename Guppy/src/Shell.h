/*
 * Copyright (C) 2016 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef SHELL_H
#define SHELL_H

#include <functional>
#include <map>
#include <queue>
#include <vector>
#include <stdexcept>
#include <vulkan/vulkan.h>

#include "Game.h"

namespace RenderPass {
class Base;
}

class UI {
   public:
    virtual std::unique_ptr<RenderPass::Base> &getRenderPass() = 0;
    virtual void draw(VkCommandBuffer cmd, uint8_t frameIndex) = 0;
    virtual void reset() = 0;
};

// structure for comparing char arrays
struct less_str {
    bool operator()(char const *a, char const *b) const { return std::strcmp(a, b) < 0; }
};

class Shell {
   public:
    Shell(const Shell &sh) = delete;
    Shell &operator=(const Shell &sh) = delete;
    virtual ~Shell() {}

    struct BackBuffer {
        uint32_t imageIndex;
        VkSemaphore acquireSemaphore;
        VkSemaphore renderSemaphore;
        // signaled when this struct is ready for reuse
        VkFence presentFence;
    };

    struct LayerProperties {
        VkLayerProperties properties;
        std::vector<VkExtensionProperties> extensionProps;
    };  // *

    struct SurfaceProperties {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> surf_formats;
        std::vector<VkPresentModeKHR> presentModes;
    };  // *

    struct PhysicalDeviceProperties {
        VkPhysicalDevice device;
        uint32_t queue_family_count;
        std::vector<VkQueueFamilyProperties> queue_props;
        VkPhysicalDeviceMemoryProperties memory_properties;
        VkPhysicalDeviceProperties properties;
        std::vector<VkExtensionProperties> extensions;
        std::multimap<const char *, VkExtensionProperties, less_str> layer_extension_map = {};
        VkPhysicalDeviceFeatures features;
    };  // *

    struct Context {
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugReportCallbackEXT debug_report = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debug_utils_messenger = VK_NULL_HANDLE;

        bool sampler_anisotropy_enabled_ = false;   // *
        bool sample_rate_shading_enabled_ = false;  // *
        bool linear_blitting_supported_ = false;    // *

        VkPhysicalDevice physical_dev = VK_NULL_HANDLE;
        std::vector<PhysicalDeviceProperties> physical_dev_props;  // *
        uint32_t physical_dev_index = 0;                           // *
        VkPhysicalDeviceMemoryProperties mem_props = {};           // *
        std::vector<VkQueue> queues;                               // *
        uint32_t graphics_index = 0;                               // *
        uint32_t present_index = 0;                                // *
        uint32_t transfer_index = 0;                               // *
        uint32_t game_queue_family = 0;
        uint32_t present_queue_family = 0;

        VkDevice dev = VK_NULL_HANDLE;
        // VkQueue game_queue;
        // VkQueue present_queue;

        // SURFACE (TODO: figure out what is what)
        SurfaceProperties surfaceProps = {};
        VkSurfaceFormatKHR surfaceFormat = {};
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        VkPresentModeKHR mode = {};
        VkSampleCountFlagBits samples = {};
        uint32_t imageCount = 0;
        VkExtent2D extent = {};

        // DEPTH
        VkFormat depthFormat = {};

        // SWAPCHAIN (TODO: figure out what is what)
        VkSwapchainKHR swapchain = VK_NULL_HANDLE;
        std::queue<BackBuffer> backBuffers;
        BackBuffer acquiredBackBuffer = {};
    };

    const Context &context() const { return ctx_; }
    const Game &game() const { return game_; }

    // LOGGING
    enum LogPriority {
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARN,
        LOG_ERR,
    };
    virtual void log(LogPriority priority, const char *msg) const;

    virtual void run() = 0;
    virtual void quit() = 0;

    // SHADER RECOMPILING
    virtual void asyncAlert(uint64_t milliseconds) = 0;                                           // TODO: think this through
    virtual void checkDirectories() = 0;                                                          // TODO: think this through
    virtual void watchDirectory(std::string dir, std::function<void(std::string)> callback) = 0;  // TODO: think this through

    inline void onKey(GAME_KEY key) { game_.onKey(key); }                   // TODO: think this through
    inline void onMouse(const MouseInput &input) { game_.onMouse(input); }  // TODO: think this through

    // SWAPCHAIN
    void resizeSwapchain(uint32_t width_hint, uint32_t height_hint, bool refresh_capabilities = true);

    // UI
    virtual std::shared_ptr<UI> getUI() const { return nullptr; };  // TODO: think this through
    virtual void initUI(VkRenderPass pass){};                       // TODO: think this through
    virtual void updateRenderPass(){};                              // TODO: think this through

   protected:
    Shell(Game &game);

    virtual void setPlatformSpecificExtensions() = 0;
    void init_vk();
    void cleanup_vk();

    void create_context();
    void destroy_context();

    void addGameTime(float time);

    // SWAPCHAIN
    void acquireBackBuffer();
    void presentBackBuffer();

    Game &game_;
    const Game::Settings &settings_;

    std::vector<const char *> instance_layers_;
    std::vector<const char *> instance_extensions_;

    std::vector<const char *> device_extensions_;

    std::vector<LayerProperties> layerProps_;          // *
    std::vector<VkExtensionProperties> instExtProps_;  // *

   private:
    bool debug_report_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT obj_type, uint64_t object,
                               size_t location, int32_t msg_code, const char *layer_prefix, const char *msg);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report_callback(VkDebugReportFlagsEXT flags,
                                                                VkDebugReportObjectTypeEXT obj_type, uint64_t object,
                                                                size_t location, int32_t msg_code, const char *layer_prefix,
                                                                const char *msg, void *user_data) {
        Shell *shell = reinterpret_cast<Shell *>(user_data);
        return shell->debug_report_callback(flags, obj_type, object, location, msg_code, layer_prefix, msg);
    }

    void assert_all_instance_layers() const;
    void assert_all_instance_extensions() const;

    bool has_all_device_layers(VkPhysicalDevice phy) const;
    bool has_all_device_extensions(VkPhysicalDevice phy) const;

    // called by init_instance
    void enumerate_instance_properties();           // *
    void determine_api_version(uint32_t &version);  // *

    // called by init_vk
    void enumerate_instance_layer_extension_properties(LayerProperties &layer_props);  // *
    void init_validation_messenger();                                                  // *
    virtual PFN_vkGetInstanceProcAddr load_vk() = 0;
    virtual VkBool32 canPresent(VkPhysicalDevice phy, uint32_t queue_family) = 0;
    void init_instance();
    void init_debug_report();
    void init_physical_dev();

    // called by enumerate_instance_layer_extension_properties
    void enumerate_instance_extension_properties();                                                                   // *
    void enumerate_device_layer_extension_properties(PhysicalDeviceProperties &props, LayerProperties &layer_props);  // *

    // called by init_physical_dev
    void enumerate_physical_devs(uint32_t physical_dev_count = 1);  // *
    void pick_physical_dev();                                       // *

    // called by pick_device
    bool is_dev_suitable(const PhysicalDeviceProperties &props, uint32_t &graphics_queue_index,
                         uint32_t &present_queue_index,
                         uint32_t &transfer_queue_index);  // *

    // called by is_dev_suitable
    bool determine_queue_families_support(const PhysicalDeviceProperties &props, uint32_t &graphics_queue_family_index,
                                          uint32_t &present_queue_family_index,
                                          uint32_t &transfer_queue_family_index);    // *
    bool determine_device_extension_support(const PhysicalDeviceProperties &props);  // *
    void determine_device_feature_support(const PhysicalDeviceProperties &props);    // *
    void determine_sample_count(const PhysicalDeviceProperties &props);              // *

    // called by create_context
    void init_dev_queues();  // *
    void create_dev();
    void create_back_buffers();
    void destroy_back_buffers();
    virtual void createWindow() = 0;
    virtual VkSurfaceKHR createSurface(VkInstance instance) = 0;
    void create_swapchain();
    void destroy_swapchain();

    // called by create_swapchain
    void enumerate_surface_properties();        // *
    void determine_depth_format();              // *
    void determine_swapchain_surface_format();  // *
    void determine_swapchain_present_mode();    // *
    void determine_swapchain_image_count();     // *

    // called by resizeSwapchain
    bool Shell::determine_swapchain_extent(uint32_t width_hint, uint32_t height_hint, bool refresh_capabilities);  // *

    // called by cleanup_vk
    void destroy_instance();  // *

    void fake_present();

    Context ctx_;

    const float game_tick_;
    float game_time_;
};

#endif  // SHELL_H
