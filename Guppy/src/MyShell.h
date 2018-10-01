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

#ifndef MY_SHELL_H
#define MY_SHELL_H

#include <queue>
#include <map>
#include <vector>
#include <stdexcept>
#include <vulkan/vulkan.h>

#include "Extensions.h"
#include "Game.h"

class Game;

class MyShell {
   public:
    MyShell(const MyShell &sh) = delete;
    MyShell &operator=(const MyShell &sh) = delete;
    virtual ~MyShell() {}

    struct BackBuffer {
        uint32_t image_index;
        VkSemaphore acquire_semaphore;
        VkSemaphore render_semaphore;
        // signaled when this struct is ready for reuse
        VkFence present_fence;
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
        VkInstance instance;
        VkDebugReportCallbackEXT debug_report;
        VkDebugUtilsMessengerEXT debug_utils_messenger;

        bool sampler_anisotropy_enabled_;   // *
        bool sample_rate_shading_enabled_;  // *

        VkPhysicalDevice physical_dev;
        std::vector<PhysicalDeviceProperties> physical_dev_props;  // *
        uint32_t physical_dev_index;                               // *
        std::vector<VkQueue> queues;                               // *
        uint32_t graphics_index;                                   // *
        uint32_t present_index;                                    // *
        uint32_t transfer_index;                                   // *
        uint32_t game_queue_family;
        uint32_t present_queue_family;

        VkDevice dev;
        //VkQueue game_queue;
        //VkQueue present_queue;

        std::queue<BackBuffer> back_buffers;

        SurfaceProperties surface_props;  // *
        VkSurfaceKHR surface;
        VkSurfaceFormatKHR format;
        VkPresentModeKHR mode;   // *
        uint32_t image_count;  // *
        VkFormat depth_format;   // *

        VkSwapchainKHR swapchain;
        VkExtent2D extent;

        BackBuffer acquired_back_buffer;
    };
    const Context &context() const { return ctx_; }

    enum LogPriority {
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARN,
        LOG_ERR,
    };
    virtual void log(LogPriority priority, const char *msg) const;

    virtual void run() = 0;
    virtual void quit() = 0;

   protected:
    MyShell(Game &game);

    void init_vk();
    void cleanup_vk();

    void create_context();
    void destroy_context();

    void resize_swapchain(uint32_t width_hint, uint32_t height_hint, bool refresh_capabilities = true);

    void add_game_time(float time);

    void acquire_back_buffer();
    void present_back_buffer();

    Game &game_;
    const Game::Settings &settings_;

    std::vector<const char *> instance_layers_;
    std::vector<const char *> instance_extensions_;

    std::vector<const char *> device_extensions_;

    // NEW
    std::vector<LayerProperties> layerProps_;
    std::vector<VkExtensionProperties> instExtProps_;

   private:
    bool debug_report_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT obj_type, uint64_t object, size_t location,
                               int32_t msg_code, const char *layer_prefix, const char *msg);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT obj_type,
                                                                uint64_t object, size_t location, int32_t msg_code,
                                                                const char *layer_prefix, const char *msg, void *user_data) {
        MyShell *shell = reinterpret_cast<MyShell *>(user_data);
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
    virtual bool can_present(VkPhysicalDevice phy, uint32_t queue_family) = 0;
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
    bool is_dev_suitable(const PhysicalDeviceProperties &props, uint32_t &graphics_queue_index, uint32_t &present_queue_index,
                         uint32_t &transfer_queue_index);  // *

    // called by is_dev_suitable
    bool MyShell::determine_queue_families_support(const PhysicalDeviceProperties &props, uint32_t &graphics_queue_family_index,
                                                   uint32_t &present_queue_family_index,
                                                   uint32_t &transfer_queue_family_index);    // *
    bool MyShell::determine_device_extension_support(const PhysicalDeviceProperties &props);  // *
    void MyShell::determine_device_feature_support(const PhysicalDeviceProperties &props);    // *

    // called by create_context
    void init_dev_queues();  // *
    void create_dev();
    void create_back_buffers();
    void destroy_back_buffers();
    virtual VkSurfaceKHR create_surface(VkInstance instance) = 0;
    void create_swapchain();
    void destroy_swapchain();

    // called by create_swapchain
    void enumerate_surface_properties();        // *
    void determine_depth_format();              // *
    void determine_swapchain_surface_format();  // *
    void determine_swapchain_present_mode();    // *
    void determine_swapchain_image_count();     // *

    // called by resize_swapchain
    bool MyShell::determine_swapchain_extent(uint32_t width_hint, uint32_t height_hint, bool refresh_capabilities);  // *

    // called by cleanup_vk
    void destroy_instance();  // *

    void fake_present();

    Context ctx_;

    const float game_tick_;
    float game_time_;
};

#endif  // SHELL_H
