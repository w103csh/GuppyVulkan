/*
 * Vulkan Samples
 *
 * Copyright (C) 2015-2016 Valve Corporation
 * Copyright (C) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google, Inc.
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

/*
VULKAN_SAMPLE_DESCRIPTION
samples "init" utility functions
*/

#include <algorithm>
#include <array>
#include <cstdlib>
#include <assert.h>
#include <set>
#include <string.h>
#include "util_init.hpp"

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include <linux/input.h>
#endif

using namespace std;

/*
 * TODO: function description here
 */
VkResult init_global_extension_properties(layer_properties &layer_props) {
    VkExtensionProperties *instance_extensions;
    uint32_t instance_extension_count;
    VkResult res;
    char *layer_name = NULL;

    layer_name = layer_props.properties.layerName;

    do {
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, NULL);
        if (res) return res;

        if (instance_extension_count == 0) {
            return VK_SUCCESS;
        }

        layer_props.instance_extensions.resize(instance_extension_count);
        instance_extensions = layer_props.instance_extensions.data();
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, instance_extensions);
    } while (res == VK_INCOMPLETE);

    return res;
}

/*
 * TODO: function description here
 */
VkResult init_global_layer_properties(struct sample_info &info, const std::vector<const char *> &required_layers) {
    uint32_t instance_layer_count;
    VkLayerProperties *vk_props = NULL;
    VkResult res;
#ifdef __ANDROID__
    // This place is the first place for samples to use Vulkan APIs.
    // Here, we are going to open Vulkan.so on the device and retrieve function pointers using
    // vulkan_wrapper helper.
    if (!InitVulkan()) {
        LOGE("Failied initializing Vulkan APIs!");
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    LOGI("Loaded Vulkan APIs.");
#endif

    /*
     * It's possible, though very rare, that the number of
     * instance layers could change. For example, installing something
     * could include new layers that the loader would pick up
     * between the initial query for the count and the
     * request for VkLayerProperties. The loader indicates that
     * by returning a VK_INCOMPLETE status and will update the
     * the count parameter.
     * The count parameter will be updated with the number of
     * entries loaded into the data pointer - in case the number
     * of layers went down or is smaller than the size given.
     */
    do {
        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, NULL);
        if (res) return res;

        if (instance_layer_count == 0) {
            return VK_SUCCESS;
        }

        vk_props = (VkLayerProperties *)realloc(vk_props, instance_layer_count * sizeof(VkLayerProperties));

        res = vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_props);
    } while (res == VK_INCOMPLETE);

    /*
     * Now gather the extension list for each instance layer.
     */
    bool foundRequired = false;
    for (uint32_t i = 0; i < instance_layer_count; i++) {
        layer_properties layer_props;
        layer_props.properties = vk_props[i];

        auto it = std::find_if(required_layers.begin(), required_layers.end(), [&layer_props](auto layerName) {
            return std::strcmp(layer_props.properties.layerName, layerName) == 0;
        });
        foundRequired |= (it != required_layers.end());

        res = init_global_extension_properties(layer_props);
        if (res) return res;
        info.instance_layer_properties.push_back(layer_props);
    }
    free(vk_props);

    assert(foundRequired);

    return res;
}

VkResult init_device_layer_extension_properties(struct sample_info &info, physical_device_properties &props,
                                          layer_properties &layer_props) {
    VkResult res;
    uint32_t extensionCount;
    std::vector<VkExtensionProperties> extensions;
    char *layer_name = layer_props.properties.layerName;
    do {
        res = vkEnumerateDeviceExtensionProperties(props.device, layer_name, &extensionCount, NULL);
        if (res) return res;

        if (extensionCount == 0) {
            return VK_SUCCESS;
        }

        extensions.resize(extensionCount);
        res = vkEnumerateDeviceExtensionProperties(props.device, layer_name, &extensionCount, extensions.data());
    } while (res == VK_INCOMPLETE);

    if (!extensions.empty())
        props.layer_extension_map.insert(std::pair<const char *, std::vector<VkExtensionProperties>>(layer_name, extensions));

    return res;
}

void init_instance_layer_names(struct sample_info &info, const std::vector<const char *> &layer_names) {
    info.instance_layer_names.insert(info.instance_layer_names.begin(), layer_names.begin(), layer_names.end());
}

void init_instance_extension_names(struct sample_info &info, const std::vector<const char *> &extension_names) {
    info.instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef __ANDROID__
    info.instance_extension_names.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#elif defined(_WIN32)
    info.instance_extension_names.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
    info.instance_extension_names.push_back(VK_MVK_IOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    info.instance_extension_names.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    info.instance_extension_names.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#else
    info.instance_extension_names.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
    info.instance_extension_names.insert(info.instance_extension_names.begin(), extension_names.begin(), extension_names.end());
}

VkResult init_instance(struct sample_info &info, char const *const app_short_name) {
    uint32_t version = VK_VERSION_1_0;
    determine_api_version(version);

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = app_short_name;
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = app_short_name;
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = version;

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledLayerCount = info.instance_layer_names.size();
    inst_info.ppEnabledLayerNames = info.instance_layer_names.size() ? info.instance_layer_names.data() : NULL;
    inst_info.enabledExtensionCount = info.instance_extension_names.size();
    inst_info.ppEnabledExtensionNames = info.instance_extension_names.data();

    VkResult res = vkCreateInstance(&inst_info, NULL, &info.inst);
    assert(res == VK_SUCCESS);

    return res;
}

void determine_api_version(uint32_t &version) {
    version = VK_API_VERSION_1_0;
// Android build not at 1.1 yet
#ifndef ANDROID
    // Keep track of the major/minor version we can actually use
    uint16_t using_major_version = 1;
    uint16_t using_minor_version = 0;
    std::string using_version_string = "";

    // Set the desired version we want
    uint16_t desired_major_version = 1;
    uint16_t desired_minor_version = 1;
    uint32_t desired_version = VK_MAKE_VERSION(desired_major_version, desired_minor_version, 0);
    std::string desired_version_string = "";
    desired_version_string += std::to_string(desired_major_version);
    desired_version_string += ".";
    desired_version_string += std::to_string(desired_minor_version);
    VkInstance instance = VK_NULL_HANDLE;
    std::vector<VkPhysicalDevice> physical_devices_desired;

    // Determine what API version is available
    uint32_t api_version;
    if (VK_SUCCESS == vkEnumerateInstanceVersion(&api_version)) {
        // Translate the version into major/minor for easier comparison
        uint32_t loader_major_version = VK_VERSION_MAJOR(api_version);
        uint32_t loader_minor_version = VK_VERSION_MINOR(api_version);
        std::cout << "Loader/Runtime support detected for Vulkan " << loader_major_version << "." << loader_minor_version << "\n";

        // Check current version against what we want to run
        if (loader_major_version > desired_major_version ||
            (loader_major_version == desired_major_version && loader_minor_version >= desired_minor_version)) {
            // Initialize the VkApplicationInfo structure with the version of the API we're intending to use
            VkApplicationInfo app_info = {};
            app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            app_info.pNext = NULL;
            app_info.pApplicationName = "";
            app_info.applicationVersion = 1;
            app_info.pEngineName = "";
            app_info.engineVersion = 1;
            app_info.apiVersion = desired_version;

            // Initialize the VkInstanceCreateInfo structure
            VkInstanceCreateInfo inst_info = {};
            inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            inst_info.pNext = NULL;
            inst_info.flags = 0;
            inst_info.pApplicationInfo = &app_info;
            inst_info.enabledExtensionCount = 0;
            inst_info.ppEnabledExtensionNames = NULL;
            inst_info.enabledLayerCount = 0;
            inst_info.ppEnabledLayerNames = NULL;

            // Attempt to create the instance
            if (VK_SUCCESS != vkCreateInstance(&inst_info, NULL, &instance)) {
                std::cout << "Unknown error creating " << desired_version_string << " Instance\n";
                exit(-1);
            }

            // Get the list of physical devices
            uint32_t phys_dev_count = 1;
            if (VK_SUCCESS != vkEnumeratePhysicalDevices(instance, &phys_dev_count, NULL) || phys_dev_count == 0) {
                std::cout << "Failed searching for Vulkan physical devices\n";
                exit(-1);
            }
            std::vector<VkPhysicalDevice> physical_devices;
            physical_devices.resize(phys_dev_count);
            if (VK_SUCCESS != vkEnumeratePhysicalDevices(instance, &phys_dev_count, physical_devices.data()) ||
                phys_dev_count == 0) {
                std::cout << "Failed enumerating Vulkan physical devices\n";
                exit(-1);
            }

            // Go through the list of physical devices and select only those that are capable of running the API version we want.
            for (uint32_t dev = 0; dev < physical_devices.size(); ++dev) {
                VkPhysicalDeviceProperties physical_device_props = {};
                vkGetPhysicalDeviceProperties(physical_devices[dev], &physical_device_props);
                if (physical_device_props.apiVersion >= desired_version) {
                    physical_devices_desired.push_back(physical_devices[dev]);
                }
            }

            // If we have something in the desired version physical device list, we're good
            if (physical_devices_desired.size() > 0) {
                using_major_version = desired_major_version;
                using_minor_version = desired_minor_version;
            }
        }
    }

    using_version_string += std::to_string(using_major_version);
    using_version_string += ".";
    using_version_string += std::to_string(using_minor_version);

    if (using_minor_version != desired_minor_version) {
        std::cout << "Determined that this system can only use Vulkan API version " << using_version_string
                  << " instead of desired version " << desired_version_string << std::endl;
    } else {
        version = desired_version;
        std::cout << "Determined that this system can run desired Vulkan API version " << desired_version_string << std::endl;
    }

    // Destroy the instance if it was created
    if (VK_NULL_HANDLE == instance) {
        vkDestroyInstance(instance, NULL);
    }
#endif
}

void init_device_extension_names(struct sample_info &info) {
    info.device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
}

VkResult init_device(struct sample_info &info) {
    /* DEPENDS pick_device() */

    VkResult res;

    // queue create info
    std::set<uint32_t> unique_queue_families = {
        info.graphics_queue_family_index,
        info.present_queue_family_index,
        info.transfer_queue_family_index,
    };
    float queue_priorities[1] = { 0.0 }; // only one queue per family atm
    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    for (auto& index : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.pNext = NULL;
        queue_info.queueCount = 1; // only one queue per family atm
        queue_info.queueFamilyIndex = index;
        queue_info.pQueuePriorities = queue_priorities;
        queue_infos.push_back(queue_info);
    }

    // features
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = info.enableSampleShading ? VK_TRUE : VK_FALSE; // TODO: OPTION (FEATURE BASED (its based off below))
    deviceFeatures.sampleRateShading = info.enableSampleShading ? VK_TRUE : VK_FALSE;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
    device_info.pQueueCreateInfos = queue_infos.data();
    device_info.enabledExtensionCount = info.device_extension_names.size();
    device_info.ppEnabledExtensionNames = device_info.enabledExtensionCount ? info.device_extension_names.data() : NULL;
    device_info.pEnabledFeatures = &deviceFeatures;

    res = vkCreateDevice(info.physical_device, &device_info, NULL, &info.device);
    assert(res == VK_SUCCESS);

    return res;
}

VkResult init_enumerate_devices(struct sample_info &info, uint32_t gpu_count) {
    /* depends on init_surface() */

    std::vector<VkPhysicalDevice> gpus;
    uint32_t const U_ASSERT_ONLY req_count = gpu_count;
    VkResult res = vkEnumeratePhysicalDevices(info.inst, &gpu_count, NULL);
    assert(gpu_count);
    gpus.resize(gpu_count);

    res = vkEnumeratePhysicalDevices(info.inst, &gpu_count, gpus.data());
    assert(!res && gpu_count >= req_count);

    for (auto &device : gpus) {
        physical_device_properties props;
        props.device = std::move(device);

        // queue family properties
        vkGetPhysicalDeviceQueueFamilyProperties(props.device, &props.queue_family_count, NULL);
        assert(props.queue_family_count >= 1);

        props.queue_props.resize(props.queue_family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(props.device, &props.queue_family_count, props.queue_props.data());
        assert(props.queue_family_count >= 1);

        // memory properties
        vkGetPhysicalDeviceMemoryProperties(props.device, &props.memory_properties);

        // properties
        vkGetPhysicalDeviceProperties(props.device, &props.properties);

        // extensions
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(props.device, NULL, &extensionCount, NULL);
        props.extensions.resize(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, props.extensions.data());

        // layer extensions
        for (auto &layer_props : info.instance_layer_properties) {
            init_device_layer_extension_properties(info, props, layer_props);
        }

        // capabilities
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(props.device, info.surface, &props.surf_props.capabilities);

        // surface formats
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(props.device, info.surface, &formatCount, NULL);
        if (formatCount != 0) {
            props.surf_props.surf_formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(props.device, info.surface, &formatCount, props.surf_props.surf_formats.data());
        }

        // present modes
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(props.device, info.surface, &presentModeCount, NULL);
        if (presentModeCount != 0) {
            props.surf_props.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(props.device, info.surface, &presentModeCount,
                                                      props.surf_props.presentModes.data());
        }

        // features
        vkGetPhysicalDeviceFeatures(props.device, &props.features);

        info.physical_device_properties.push_back(props);
    }

    gpus.erase(gpus.begin(), gpus.end());

    return res;
}

VkResult init_debug_report_callback(struct sample_info &info, PFN_vkDebugReportCallbackEXT dbgFunc) {
    VkResult res;
    VkDebugReportCallbackEXT debug_report_callback;

    info.dbgCreateDebugReportCallback =
        (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(info.inst, "vkCreateDebugReportCallbackEXT");
    if (!info.dbgCreateDebugReportCallback) {
        std::cout << "GetInstanceProcAddr: Unable to find "
                     "vkCreateDebugReportCallbackEXT function."
                  << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    std::cout << "Got dbgCreateDebugReportCallback function\n";

    info.dbgDestroyDebugReportCallback =
        (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(info.inst, "vkDestroyDebugReportCallbackEXT");
    if (!info.dbgDestroyDebugReportCallback) {
        std::cout << "GetInstanceProcAddr: Unable to find "
                     "vkDestroyDebugReportCallbackEXT function."
                  << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    std::cout << "Got dbgDestroyDebugReportCallback function\n";

    VkDebugReportCallbackCreateInfoEXT create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    create_info.pNext = NULL;
    create_info.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
    create_info.pfnCallback = dbgFunc;
    create_info.pUserData = NULL;

    res = info.dbgCreateDebugReportCallback(info.inst, &create_info, NULL, &debug_report_callback);
    switch (res) {
        case VK_SUCCESS:
            std::cout << "Successfully created debug report callback object\n";
            info.debug_report_callbacks.push_back(debug_report_callback);
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            std::cout << "dbgCreateDebugReportCallback: out of host memory pointer\n" << std::endl;
            return VK_ERROR_INITIALIZATION_FAILED;
            break;
        default:
            std::cout << "dbgCreateDebugReportCallback: unknown failure\n" << std::endl;
            return VK_ERROR_INITIALIZATION_FAILED;
            break;
    }
    return res;
}

void destroy_debug_report_callback(struct sample_info &info) {
    while (info.debug_report_callbacks.size() > 0) {
        info.dbgDestroyDebugReportCallback(info.inst, info.debug_report_callbacks.back(), NULL);
        info.debug_report_callbacks.pop_back();
    }
}

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)

static void handle_ping(void *data, wl_shell_surface *shell_surface, uint32_t serial) {
    wl_shell_surface_pong(shell_surface, serial);
}

static void handle_configure(void *data, wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height) {}

static void handle_popup_done(void *data, wl_shell_surface *shell_surface) {}

static const wl_shell_surface_listener shell_surface_listener = {handle_ping, handle_configure, handle_popup_done};

static void registry_handle_global(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    sample_info *info = (sample_info *)data;
    // pickup wayland objects when they appear
    if (strcmp(interface, "wl_compositor") == 0) {
        info->compositor = (wl_compositor *)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    } else if (strcmp(interface, "wl_shell") == 0) {
        info->shell = (wl_shell *)wl_registry_bind(registry, id, &wl_shell_interface, 1);
    }
}

static void registry_handle_global_remove(void *data, wl_registry *registry, uint32_t name) {}

static const wl_registry_listener registry_listener = {registry_handle_global, registry_handle_global_remove};

#endif

void init_connection(struct sample_info &info) {
#if defined(VK_USE_PLATFORM_XCB_KHR)
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    info.connection = xcb_connect(NULL, &scr);
    if (info.connection == NULL || xcb_connection_has_error(info.connection)) {
        std::cout << "Unable to make an XCB connection\n";
        exit(-1);
    }

    setup = xcb_get_setup(info.connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0) xcb_screen_next(&iter);

    info.screen = iter.data;
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    info.display = wl_display_connect(nullptr);

    if (info.display == nullptr) {
        printf(
            "Cannot find a compatible Vulkan installable client driver "
            "(ICD).\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    info.registry = wl_display_get_registry(info.display);
    wl_registry_add_listener(info.registry, &registry_listener, &info);
    wl_display_dispatch(info.display);
#endif
}
#ifdef _WIN32
static void run(struct sample_info *info) { /* Placeholder for samples that want to show dynamic content */
}

// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    struct sample_info *info = reinterpret_cast<struct sample_info *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            break;
        case WM_PAINT:
            run(info);
            return 0;
        default:
            break;
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

void init_window(struct sample_info &info) {
    WNDCLASSEX win_class;
    assert(info.width > 0);
    assert(info.height > 0);

    info.connection = GetModuleHandle(NULL);
    sprintf(info.name, "Guppy");

    // Initialize the window class structure:
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WndProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = info.connection;  // hInstance
    win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = info.name;
    win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    // Register window class:
    if (!RegisterClassEx(&win_class)) {
        // It didn't work, so try to give a useful error:
        printf("Unexpected error trying to start the application!\n");
        fflush(stdout);
        exit(1);
    }
    // Create window with the registered class:
    RECT wr = {0, 0, info.width, info.height};
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    info.window = CreateWindowEx(0,
                                 info.name,             // class name
                                 info.name,             // app name
                                 WS_OVERLAPPEDWINDOW |  // window style
                                     WS_VISIBLE | WS_SYSMENU,
                                 100, 100,            // x/y coords
                                 wr.right - wr.left,  // width
                                 wr.bottom - wr.top,  // height
                                 NULL,                // handle to parent
                                 NULL,                // handle to menu
                                 info.connection,     // hInstance
                                 NULL);               // no extra parameters
    if (!info.window) {
        // It didn't work, so try to give a useful error:
        printf("Cannot create a window in which to draw!\n");
        fflush(stdout);
        exit(1);
    }
    SetWindowLongPtr(info.window, GWLP_USERDATA, (LONG_PTR)&info);
}

void destroy_window(struct sample_info &info) {
    vkDestroySurfaceKHR(info.inst, info.surface, NULL);
    DestroyWindow(info.window);
}

#elif defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK)

// iOS & macOS: init_window() implemented externally to allow access to Objective-C components

void destroy_window(struct sample_info &info) { info.window = NULL; }

#elif defined(__ANDROID__)
// Android implementation.
void init_window(struct sample_info &info) {}

void destroy_window(struct sample_info &info) {}

#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)

void init_window(struct sample_info &info) {
    assert(info.width > 0);
    assert(info.height > 0);

    info.window = wl_compositor_create_surface(info.compositor);
    if (!info.window) {
        printf("Can not create wayland_surface from compositor!\n");
        fflush(stdout);
        exit(1);
    }

    info.shell_surface = wl_shell_get_shell_surface(info.shell, info.window);
    if (!info.shell_surface) {
        printf("Can not get shell_surface from wayland_surface!\n");
        fflush(stdout);
        exit(1);
    }

    wl_shell_surface_add_listener(info.shell_surface, &shell_surface_listener, &info);
    wl_shell_surface_set_toplevel(info.shell_surface);
}

void destroy_window(struct sample_info &info) {
    wl_shell_surface_destroy(info.shell_surface);
    wl_surface_destroy(info.window);
    wl_shell_destroy(info.shell);
    wl_compositor_destroy(info.compositor);
    wl_registry_destroy(info.registry);
    wl_display_disconnect(info.display);
}

#else

void init_window(struct sample_info &info) {
    assert(info.width > 0);
    assert(info.height > 0);

    uint32_t value_mask, value_list[32];

    info.window = xcb_generate_id(info.connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = info.screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE;

    xcb_create_window(info.connection, XCB_COPY_FROM_PARENT, info.window, info.screen->root, 0, 0, info.width, info.height, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, info.screen->root_visual, value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(info.connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(info.connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(info.connection, 0, 16, "WM_DELETE_WINDOW");
    info.atom_wm_delete_window = xcb_intern_atom_reply(info.connection, cookie2, 0);

    xcb_change_property(info.connection, XCB_PROP_MODE_REPLACE, info.window, (*reply).atom, 4, 32, 1,
                        &(*info.atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(info.connection, info.window);

    // Force the x/y coordinates to 100,100 results are identical in consecutive
    // runs
    const uint32_t coords[] = {100, 100};
    xcb_configure_window(info.connection, info.window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
    xcb_flush(info.connection);

    xcb_generic_event_t *e;
    while ((e = xcb_wait_for_event(info.connection))) {
        if ((e->response_type & ~0x80) == XCB_EXPOSE) break;
    }
}

void destroy_window(struct sample_info &info) {
    vkDestroySurfaceKHR(info.inst, info.surface, NULL);
    xcb_destroy_window(info.connection, info.window);
    xcb_disconnect(info.connection);
}

#endif  // _WIN32

void init_window_size(struct sample_info &info, int32_t default_width, int32_t default_height) {
#ifdef __ANDROID__
    AndroidGetWindowSize(&info.width, &info.height);
#else
    info.width = default_width;
    info.height = default_height;
#endif
}

void init_depth_buffer(struct sample_info &info) {
    /* allow custom depth formats */
#ifdef __ANDROID__
    // Depth format needs to be VK_FORMAT_D24_UNORM_S8_UINT on Android.
    info.depth.format = VK_FORMAT_D24_UNORM_S8_UINT;
#elif defined(VK_USE_PLATFORM_IOS_MVK)
    if (info.depth.format == VK_FORMAT_UNDEFINED) info.depth.format = VK_FORMAT_D32_SFLOAT;
#else
    if (info.depth.format == VK_FORMAT_UNDEFINED) info.depth.format = find_depth_format(info);
#endif

    VkFormatProperties props;
    VkImageTiling tiling;
    vkGetPhysicalDeviceFormatProperties(info.physical_device_properties[0].device, info.depth.format, &props);
    if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        tiling = VK_IMAGE_TILING_LINEAR;
    } else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        tiling = VK_IMAGE_TILING_OPTIMAL;
    } else {
        /* Try other depth formats? */
        throw std::runtime_error(("depth_format Unsupported.\n"));
    }

    create_image(info, info.swapchain_extent.width, info.swapchain_extent.height, 1, info.num_samples, info.depth.format, tiling,
                 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, info.depth.image,
                 info.depth.mem);

    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (has_stencil_component(info.depth.format)) {
        aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    create_image_view(info.device, info.depth.image, 1, info.depth.format, aspectFlags, VK_IMAGE_VIEW_TYPE_2D,
                      info.depth.view);

    transition_image_layout(info.cmds[info.graphics_queue_family_index], info.depth.image, 1, info.depth.format,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

}

 void init_color_buffer(struct sample_info &info) {
    create_image(info, info.swapchain_extent.width, info.swapchain_extent.height, 1, info.num_samples, info.swapchain_format.format,
                 VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, info.color.image, info.color.mem);

    create_image_view(info.device, info.color.image, 1, info.swapchain_format.format, VK_IMAGE_ASPECT_COLOR_BIT,
                      VK_IMAGE_VIEW_TYPE_2D, info.color.view);

    transition_image_layout(info.cmds[info.graphics_queue_family_index], info.color.image, 1, info.swapchain_format.format,
                            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void init_surface(struct sample_info &info) {
    /* DEPENDS on init_connection() and init_window() */

    VkResult U_ASSERT_ONLY res;

    // Construct the surface description:
#ifdef _WIN32
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.hinstance = info.connection;
    createInfo.hwnd = info.window;
    res = vkCreateWin32SurfaceKHR(info.inst, &createInfo, NULL, &info.surface);
#elif defined(__ANDROID__)
    GET_INSTANCE_PROC_ADDR(info.inst, CreateAndroidSurfaceKHR);

    VkAndroidSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.window = AndroidGetApplicationWindow();
    res = info.fpCreateAndroidSurfaceKHR(info.inst, &createInfo, nullptr, &info.surface);
#elif defined(VK_USE_PLATFORM_IOS_MVK)
    VkIOSSurfaceCreateInfoMVK createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_IOS_SURFACE_CREATE_INFO_MVK;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.pView = info.window;
    res = vkCreateIOSSurfaceMVK(info.inst, &createInfo, NULL, &info.surface);
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    VkMacOSSurfaceCreateInfoMVK createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.pView = info.window;
    res = vkCreateMacOSSurfaceMVK(info.inst, &createInfo, NULL, &info.surface);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    VkWaylandSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.display = info.display;
    createInfo.surface = info.window;
    res = vkCreateWaylandSurfaceKHR(info.inst, &createInfo, NULL, &info.surface);
#else
    VkXcbSurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.connection = info.connection;
    createInfo.window = info.window;
    res = vkCreateXcbSurfaceKHR(info.inst, &createInfo, NULL, &info.surface);
#endif  // __ANDROID__  && _WIN32
    assert(res == VK_SUCCESS);
}

bool determine_queue_families_support(struct sample_info &info, const physical_device_properties props,
                                      uint32_t &graphics_queue_family_index, uint32_t &present_queue_family_index,
                                      uint32_t &transfer_queue_family_index) {
    VkResult res;

    // Determine graphics and present queues ...

    // Iterate over each queue to learn whether it supports presenting:
    VkBool32 *pSupportsPresent = (VkBool32 *)malloc(props.queue_family_count * sizeof(VkBool32));
    for (uint32_t i = 0; i < props.queue_family_count; i++) {
        res = vkGetPhysicalDeviceSurfaceSupportKHR(props.device, i, info.surface, &pSupportsPresent[i]);
        assert(res == VK_SUCCESS);

        // Search for a graphics and a present queue in the array of queue
        // families, try to find one that supports both
        if (props.queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            if (graphics_queue_family_index == UINT32_MAX) graphics_queue_family_index = i;

            if (pSupportsPresent[i] == VK_TRUE) {
                graphics_queue_family_index = i;
                present_queue_family_index = i;
                break;
            }
        }
    }
    if (present_queue_family_index == UINT32_MAX) {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        // TODO: is this the best thing to do?
        for (uint32_t i = 0; i < props.queue_family_count; ++i) {
            if (pSupportsPresent[i] == VK_TRUE) {
                present_queue_family_index = i;
                break;
            }
        }
    }
    free(pSupportsPresent);

    // Determine transfer queue ...

    for (uint32_t i = 0; i < props.queue_family_count; ++i) {
        if ((props.queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 &&
            props.queue_props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            transfer_queue_family_index = i;
            break;
        } else if (props.queue_props[i].queueFlags & VK_QUEUE_TRANSFER_BIT) {
            transfer_queue_family_index = i;
        }
    }

    // Do any other selection of device logic here...

    // Just pick the fist one that supports both
    return (graphics_queue_family_index != UINT32_MAX && present_queue_family_index != UINT32_MAX &&
            transfer_queue_family_index != UINT32_MAX);
}

bool determine_device_extension_support(struct sample_info &info, const physical_device_properties props) {
    // TODO: This won't work right if either list is not unique, but I don't feel like chaing it atm.
    size_t count = 0;
    size_t req_ext_count = info.device_extension_names.size();
    for (const auto &extension : props.extensions) {
        for (const auto& req_ext_name : info.device_extension_names) {
            if (std::strcmp(req_ext_name, extension.extensionName) == 0) count++;
            if (count == req_ext_count) return true;
        }
    }
    return false;
}

bool determine_swapchain_support(struct sample_info &info, const physical_device_properties props) {
    //// TODO: OPTION (FEATURE BASED)
    return !props.surf_props.surf_formats.empty() && !props.surf_props.presentModes.empty();
}

bool determine_device_feature_support(struct sample_info &info, const physical_device_properties props) {
    //// TODO: OPTION (FEATURE BASED)
    if (info.enableSampleShading && !props.features.samplerAnisotropy) return false;
    return true;
}

bool determine_sample_count(struct sample_info &info, const physical_device_properties props) {
    //// TODO: OPTION (FEATURE BASED)
    VkSampleCountFlags counts = std::min(props.properties.limits.framebufferColorSampleCounts,
                                         props.properties.limits.framebufferDepthSampleCounts);

    info.num_samples = VK_SAMPLE_COUNT_1_BIT;
    // return the highest possible one for now
    if (counts & VK_SAMPLE_COUNT_64_BIT)
        info.num_samples = VK_SAMPLE_COUNT_64_BIT;
    else if (counts & VK_SAMPLE_COUNT_32_BIT)
        info.num_samples = VK_SAMPLE_COUNT_32_BIT;
    else if (counts & VK_SAMPLE_COUNT_16_BIT)
        info.num_samples = VK_SAMPLE_COUNT_16_BIT;
    else if (counts & VK_SAMPLE_COUNT_8_BIT)
        info.num_samples = VK_SAMPLE_COUNT_8_BIT;
    else if (counts & VK_SAMPLE_COUNT_4_BIT)
        info.num_samples = VK_SAMPLE_COUNT_4_BIT;
    else if (counts & VK_SAMPLE_COUNT_2_BIT)
        info.num_samples = VK_SAMPLE_COUNT_2_BIT;

    return true;
}

bool is_device_suitable(struct sample_info &info, const physical_device_properties props, uint32_t &graphics_queue_index,
                        uint32_t &present_queue_index, uint32_t &transfer_queue_index) {
    // contingent determinations
    if (!determine_queue_families_support(info, props, graphics_queue_index, present_queue_index, transfer_queue_index))
        return false;
    if (!determine_device_extension_support(info, props)) return false;
    if (!determine_swapchain_support(info, props)) return false;
    if (!determine_device_feature_support(info, props)) return false;
    // non-contigent determinations
    determine_sample_count(info, props);
    return true;
}

void pick_device(struct sample_info &info) {
    /* depends on init_surface(), init_enumerate_devices(), and possibly more */

    // Iterate over each enumerated physical device
    uint32_t graphics_queue_family_index, present_queue_family_index, transfer_queue_family_index;
    for (size_t i = 0; i < info.physical_device_properties.size(); i++) {
        auto &props = info.physical_device_properties[i];

        info.physical_device_property_index = graphics_queue_family_index = present_queue_family_index =
            transfer_queue_family_index = UINT32_MAX;

        if (is_device_suitable(info, props, graphics_queue_family_index, present_queue_family_index, transfer_queue_family_index)) {
            // We found a suitable physical device so set relevant data.
            info.physical_device_property_index = i;
            info.physical_device = props.device;
            info.graphics_queue_family_index = graphics_queue_family_index;
            info.present_queue_family_index = present_queue_family_index;
            info.transfer_queue_family_index = transfer_queue_family_index;
            break;
        }
    }
    assert(info.physical_device_property_index != UINT32_MAX);
}

void determine_swapchain_surface_format(struct sample_info &info, const surface_properties& props) {
    /* DEPENDS on init_enumerate_devices() */

    // no preferred type
    if (props.surf_formats.size() == 1 && props.surf_formats[0].format == VK_FORMAT_UNDEFINED) {
        info.swapchain_format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        return;
    } else {
        for (const auto &surfFormat : props.surf_formats) {
            if (surfFormat.format == VK_FORMAT_B8G8R8A8_UNORM && surfFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                info.swapchain_format = surfFormat;
                return;
            }
        }
    }
    std::cout << "choosing first available swap surface format!";
    info.swapchain_format = props.surf_formats[0];
}

void determine_swapchain_present_mode(struct sample_info &info, const surface_properties& props) {
    // guaranteed to be present (vert sync)
    VkPresentModeKHR swapchain_presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto &mode : props.presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {  // triple buffer
            swapchain_presentMode = mode;
        } else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {  // no sync
            swapchain_presentMode = mode;
        }
    }

    info.swapchain_presentMode = swapchain_presentMode;
}

 void determine_swapchain_extent(struct sample_info &info, const surface_properties& props) {
    VkExtent2D swapchainExtent;
    // width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (props.capabilities.currentExtent.width == std::numeric_limits<uint32_t>::max()) {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapchainExtent.width = info.width;
        swapchainExtent.height = info.height;

        swapchainExtent.width = std::max(props.capabilities.minImageExtent.width,
                                         std::min(props.capabilities.maxImageExtent.width, swapchainExtent.width));
        swapchainExtent.height = std::max(props.capabilities.minImageExtent.height,
                                          std::min(props.capabilities.maxImageExtent.height, swapchainExtent.height));
    } else {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = props.capabilities.currentExtent;
    }

    info.swapchain_extent = swapchainExtent;
}

void init_swapchain_extension(struct sample_info &info) {
    const auto& surf_props = info.physical_device_properties[info.physical_device_property_index].surf_props;
    determine_swapchain_surface_format(info, surf_props);
    determine_swapchain_present_mode(info, surf_props);
    determine_swapchain_extent(info, surf_props);
}

void init_presentable_image(struct sample_info &info) {
    /* DEPENDS on init_swap_chain() */

    VkResult U_ASSERT_ONLY res;
    VkSemaphoreCreateInfo imageAcquiredSemaphoreCreateInfo;
    imageAcquiredSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    imageAcquiredSemaphoreCreateInfo.pNext = NULL;
    imageAcquiredSemaphoreCreateInfo.flags = 0;

    res = vkCreateSemaphore(info.device, &imageAcquiredSemaphoreCreateInfo, NULL, &info.imageAcquiredSemaphore);
    assert(!res);

    // Get the index of the next available swapchain image:
    res = vkAcquireNextImageKHR(info.device, info.swap_chain, UINT64_MAX, info.imageAcquiredSemaphore, VK_NULL_HANDLE,
                                &info.current_buffer);
    // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
    // return codes
    assert(!res);
}

void execute_queue_cmdbuf(struct sample_info &info, const VkCommandBuffer *cmd_bufs, VkFence &fence) {
    VkResult U_ASSERT_ONLY res;

    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info[1] = {};
    submit_info[0].pNext = NULL;
    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[0].waitSemaphoreCount = 1;
    submit_info[0].pWaitSemaphores = &info.imageAcquiredSemaphore;
    submit_info[0].pWaitDstStageMask = NULL;
    submit_info[0].commandBufferCount = 1;
    submit_info[0].pCommandBuffers = cmd_bufs;
    submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
    submit_info[0].signalSemaphoreCount = 0;
    submit_info[0].pSignalSemaphores = NULL;

    /* Queue the command buffer for execution (graphics) */
    res = vkQueueSubmit(info.queues[0], 1, submit_info, fence);
    assert(!res);
}

void execute_pre_present_barrier(struct sample_info &info) {
    /* DEPENDS on init_swap_chain() */
    /* Add mem barrier to change layout to present */

    VkImageMemoryBarrier prePresentBarrier = {};
    prePresentBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    prePresentBarrier.pNext = NULL;
    prePresentBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    prePresentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    prePresentBarrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    prePresentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    prePresentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    prePresentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    prePresentBarrier.subresourceRange.baseMipLevel = 0;
    prePresentBarrier.subresourceRange.levelCount = 1;
    prePresentBarrier.subresourceRange.baseArrayLayer = 0;
    prePresentBarrier.subresourceRange.layerCount = 1;
    prePresentBarrier.image = info.buffers[info.current_buffer].image;
    vkCmdPipelineBarrier(info.cmds[info.present_queue_family_index], VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &prePresentBarrier);
}

void execute_present_image(struct sample_info &info) {
    /* DEPENDS on init_presentable_image() and init_swap_chain()*/
    /* Present the image in the window */

    VkResult U_ASSERT_ONLY res;
    VkPresentInfoKHR present;
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.pSwapchains = &info.swap_chain;
    present.pImageIndices = &info.current_buffer;
    present.pWaitSemaphores = NULL;
    present.waitSemaphoreCount = 0;
    present.pResults = NULL;

    res = vkQueuePresentKHR(info.queues[0], &present); // present
    // TODO: Deal with the VK_SUBOPTIMAL_WSI and VK_ERROR_OUT_OF_DATE_WSI
    // return codes
    assert(!res);
}

void init_swapchain(struct sample_info &info, VkImageUsageFlags usageFlags) {
    /* DEPENDS on pick_device() */
    VkResult U_ASSERT_ONLY res;

    // TODO: move below that depend on this to more determine functions
    auto &surf_props = info.physical_device_properties[info.physical_device_property_index].surf_props;

    // Determine the number of VkImage's to use in the swap chain.
    // We need to acquire only 1 presentable image at at time.
    // Asking for minImageCount images ensures that we can acquire
    // 1 presentable image as long as we present it before attempting
    // to acquire another.
    uint32_t desiredNumberOfSwapChainImages = surf_props.capabilities.minImageCount + 1;
    if (surf_props.capabilities.maxImageCount > 0 && desiredNumberOfSwapChainImages > surf_props.capabilities.maxImageCount) {
        desiredNumberOfSwapChainImages = surf_props.capabilities.maxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR preTransform;  // ex: rotate 90
    if (surf_props.capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = surf_props.capabilities.currentTransform;
    }

    // Find a supported composite alpha mode - one of these is guaranteed to be set
    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };
    for (uint32_t i = 0; i < sizeof(compositeAlphaFlags); i++) {
        if (surf_props.capabilities.supportedCompositeAlpha & compositeAlphaFlags[i]) {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapchain_ci = {};
    swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_ci.pNext = NULL;
    swapchain_ci.surface = info.surface;
    swapchain_ci.minImageCount = desiredNumberOfSwapChainImages;
    swapchain_ci.imageFormat = info.swapchain_format.format;
    swapchain_ci.imageExtent.width = info.swapchain_extent.width;
    swapchain_ci.imageExtent.height = info.swapchain_extent.height;
    swapchain_ci.preTransform = preTransform;
    swapchain_ci.compositeAlpha = compositeAlpha;
    swapchain_ci.imageArrayLayers = 1;
    swapchain_ci.presentMode = info.swapchain_presentMode;
    swapchain_ci.oldSwapchain = VK_NULL_HANDLE;
#ifndef __ANDROID__
    // Clip obscured pixels (by other windows for example). Only needed if the pixel info is reused.
    swapchain_ci.clipped = true;
#else
    swapchain_ci.clipped = false;
#endif
    swapchain_ci.imageColorSpace = info.swapchain_format.colorSpace;
    swapchain_ci.imageUsage = usageFlags;
    swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_ci.queueFamilyIndexCount = 0;
    swapchain_ci.pQueueFamilyIndices = NULL;
    if (info.graphics_queue_family_index != info.present_queue_family_index) {
        // If the graphics and present queues are from different queue families,
        // we either have to explicitly transfer ownership of images between the
        // queues, or we have to create the swapchain with imageSharingMode
        // as VK_SHARING_MODE_CONCURRENT
        uint32_t queueFamilyIndices[2] = {(uint32_t)info.graphics_queue_family_index, (uint32_t)info.present_queue_family_index};
        swapchain_ci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_ci.queueFamilyIndexCount = 2;
        swapchain_ci.pQueueFamilyIndices = queueFamilyIndices;
    }

    res = vkCreateSwapchainKHR(info.device, &swapchain_ci, NULL, &info.swap_chain);
    assert(res == VK_SUCCESS);

    res = vkGetSwapchainImagesKHR(info.device, info.swap_chain, &info.swapchainImageCount, NULL);
    assert(res == VK_SUCCESS);

    VkImage *swapchainImages = (VkImage *)malloc(info.swapchainImageCount * sizeof(VkImage));
    assert(swapchainImages);
    res = vkGetSwapchainImagesKHR(info.device, info.swap_chain, &info.swapchainImageCount, swapchainImages);
    assert(res == VK_SUCCESS);

    for (uint32_t i = 0; i < info.swapchainImageCount; i++) {
        swap_chain_buffer sc_buffer;
        sc_buffer.image = swapchainImages[i];
        create_image_view(info.device, sc_buffer.image, 1, info.swapchain_format.format, VK_IMAGE_ASPECT_COLOR_BIT,
                          VK_IMAGE_VIEW_TYPE_2D, sc_buffer.view);
        info.buffers.push_back(sc_buffer);
    }
    free(swapchainImages);
    info.current_buffer = 0;
}

void init_uniform_buffer(struct sample_info &info) {
    //VkResult U_ASSERT_ONLY res;

    float fov = glm::radians(45.0f);
    if (info.width > info.height) {
        fov *= static_cast<float>(info.height) / static_cast<float>(info.width);
    }
    info.Projection = glm::perspective(fov, static_cast<float>(info.width) / static_cast<float>(info.height), 0.1f, 100.0f);
    info.View = glm::lookAt(glm::vec3(-5, 3, -10),  // Camera is at (-5,3,-10), in World Space
                            glm::vec3(0, 0, 0),     // and looks at the origin
                            glm::vec3(0, -1, 0)     // Head is up (set to 0,-1,0 to look upside-down)
    );
    info.Model = glm::mat4(1.0f);
    // Vulkan clip space has inverted Y and half Z.
    info.Clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f);

    info.MVP = info.Clip * info.Projection * info.View * info.Model;

    auto device_size = create_buffer(info, sizeof(info.MVP), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                     info.uniform_data.buf, info.uniform_data.mem);

    // TODO: This mapping is in an update function in Guppy because their is a render loop
    //uint8_t *pData;
    //res = vkMapMemory(info.device, info.uniform_data.mem, 0, device_size, 0, (void **)&pData);
    //assert(res == VK_SUCCESS);
    //memcpy(pData, &info.MVP, sizeof(info.MVP));
    //vkUnmapMemory(info.device, info.uniform_data.mem);

    info.uniform_data.buffer_info.buffer = info.uniform_data.buf;
    info.uniform_data.buffer_info.offset = 0;
    info.uniform_data.buffer_info.range = sizeof(info.MVP);
}

void init_descriptor_set_layout(struct sample_info &info, const VkDescriptorSetLayoutCreateFlags descSetLayoutCreateFlags) {
    // UNIFORM BUFFER
    VkDescriptorSetLayoutBinding ubo_binding = {};
    ubo_binding.binding = 0;
    ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_binding.descriptorCount = 1;
    ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_binding.pImmutableSamplers = NULL;  // Optional

    // TODO: move texture things into a texture place

    // TEXTURE
    VkDescriptorSetLayoutBinding tex_binding = {};
    tex_binding.binding = 1;
    tex_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    tex_binding.descriptorCount = 1;
    tex_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    tex_binding.pImmutableSamplers = NULL;  // Optional

    // LAYOUT
    std::array<VkDescriptorSetLayoutBinding, 2> layout_bindings = {ubo_binding, tex_binding};
    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout.pNext = NULL;
    descriptor_layout.flags = descSetLayoutCreateFlags;
    descriptor_layout.bindingCount = static_cast<uint32_t>(layout_bindings.size());
    descriptor_layout.pBindings = layout_bindings.data();

    VkResult U_ASSERT_ONLY res;

    info.desc_layout.resize(NUM_DESCRIPTOR_SETS);
    res = vkCreateDescriptorSetLayout(info.device, &descriptor_layout, NULL, info.desc_layout.data());
    assert(res == VK_SUCCESS);
}

void init_pipeline_layout(struct sample_info &info, const VkDescriptorSetLayoutCreateFlags descSetLayoutCreateFlags) {
    /* DEPENDS on inti_descriptor_set_layout() */
    VkResult U_ASSERT_ONLY res;

    /* Now use the descriptor layout to create a pipeline layout */
    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext = NULL;
    pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
    pPipelineLayoutCreateInfo.setLayoutCount = NUM_DESCRIPTOR_SETS;
    pPipelineLayoutCreateInfo.pSetLayouts = info.desc_layout.data();

    res = vkCreatePipelineLayout(info.device, &pPipelineLayoutCreateInfo, NULL, &info.pipeline_layout);
    assert(res == VK_SUCCESS);
}

void init_renderpass(struct sample_info &info, bool include_depth, bool include_color, bool clear, VkImageLayout finalLayout) {
    /* DEPENDS on init_swap_chain() and init_depth_buffer() */

    VkResult U_ASSERT_ONLY res;
    std::vector<VkAttachmentDescription> attachments;

    // COLOR ATTACHMENT (SWAP-CHAIN)
    VkAttachmentReference color_reference = {};
    VkAttachmentDescription attachemnt = {};
    attachemnt = {};
    attachemnt.format = info.swapchain_format.format;
    attachemnt.samples = VK_SAMPLE_COUNT_1_BIT;
    attachemnt.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachemnt.finalLayout = finalLayout;
    attachemnt.flags = 0;
    attachments.push_back(attachemnt);
    // RESOLVE
    color_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // COLOR ATTACHMENT RESOLVE (MULTI-SAMPLE)
    VkAttachmentReference color_resolve_reference = {};
    if (include_color) {
        attachemnt = {};
        attachemnt.format = info.swapchain_format.format;
        attachemnt.samples = info.num_samples;
        attachemnt.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachemnt.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachemnt.flags = 0;
        // RESOLVE
        color_resolve_reference.attachment = static_cast<uint32_t>(attachments.size() - 1); // point to swapchain attachment
        color_resolve_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachments.push_back(attachemnt);
        color_reference.attachment = static_cast<uint32_t>(attachments.size() - 1); // point to multi-sample attachment
    }

    // DEPTH ATTACHMENT
    VkAttachmentReference depth_reference = {};
    if (include_depth) {
        attachemnt = {};
        attachemnt.format = info.depth.format;
        attachemnt.samples = info.num_samples;
        attachemnt.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        // This was "don't care" in the sample and that makes more sense to
        // me. This obvious is for some kind of stencil operation.
        attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachemnt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachemnt.flags = 0;
        attachments.push_back(attachemnt);
        // RESOLVE
        depth_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);
        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = NULL;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pResolveAttachments = include_color ? &color_resolve_reference : NULL;
    subpass.pDepthStencilAttachment = include_depth ? &depth_reference : NULL;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = NULL;

    // TODO: used for waiting in draw (figure this out)
    // VkSubpassDependency dependency = {};
    // dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    // dependency.dstSubpass = 0;
    // dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // dependency.srcAccessMask = 0;
    // dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    // dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rp_info = {};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.pNext = NULL;
    rp_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    rp_info.pAttachments = attachments.data();
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    rp_info.dependencyCount = 0;   // 1;
    rp_info.pDependencies = NULL;  // &dependency;

    res = vkCreateRenderPass(info.device, &rp_info, NULL, &info.render_pass);
    assert(res == VK_SUCCESS);
}

 void init_framebuffers(struct sample_info &info, bool include_depth, bool include_color) {
     /* DEPENDS on  */

     VkResult U_ASSERT_ONLY res;

     // There will always be a swapchain buffer
     std::vector<VkImageView> attachments;
     attachments.resize(1);

     // multi-sample (optional - needs to be same attachment index as in render pass)
     if (include_color) {
         attachments.resize(attachments.size() + 1);
         attachments[attachments.size() - 1] = info.color.view;
     }

     // depth
     if (include_color) {
         attachments.resize(attachments.size() + 1);
         attachments[attachments.size() - 1] = info.depth.view;
     }

    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = info.render_pass;
    fb_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    fb_info.pAttachments = attachments.data();
    fb_info.width = info.width;
    fb_info.height = info.height;
    fb_info.layers = 1;

    uint32_t i;
    info.framebuffers.resize(info.swapchainImageCount);
    for (i = 0; i < info.swapchainImageCount; i++) {
        attachments[0] = info.buffers[i].view;
        res = vkCreateFramebuffer(info.device, &fb_info, NULL, &info.framebuffers[i]);
        assert(res == VK_SUCCESS);
    }
}

void init_command_pools(struct sample_info &info) {
    VkResult U_ASSERT_ONLY res;
    std::set<uint32_t> unique_queue_families = {
        info.graphics_queue_family_index,
        info.present_queue_family_index,
        info.transfer_queue_family_index,
    };

    info.cmd_pools.resize(unique_queue_families.size());
    for (const auto& index : unique_queue_families) {
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.pNext = NULL;
        cmd_pool_info.queueFamilyIndex = index;
        cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        res = vkCreateCommandPool(info.device, &cmd_pool_info, NULL, &info.cmd_pools[index]);
        assert(res == VK_SUCCESS);
    };
}

void init_command_buffers(struct sample_info &info) {
    /* DEPENDS on init_command_pool() */
    VkResult U_ASSERT_ONLY res;
    std::set<uint32_t> unique_queue_families = {
        info.graphics_queue_family_index,
        info.present_queue_family_index,
        info.transfer_queue_family_index,
    };

    info.cmds.resize(unique_queue_families.size());
    for (const auto& index : unique_queue_families) {
        VkCommandBufferAllocateInfo cmd = {};
        cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd.pNext = NULL;
        cmd.commandPool = info.cmd_pools[index];
        cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd.commandBufferCount = 1;

        res = vkAllocateCommandBuffers(info.device, &cmd, &info.cmds[index]);
        assert(res == VK_SUCCESS);
    };
}

void execute_begin_command_buffer(VkCommandBuffer &cmd) {
    /* DEPENDS on init_command_buffer() */
    VkResult U_ASSERT_ONLY res;

    VkCommandBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = 0;
    cmd_buf_info.pInheritanceInfo = NULL;

    res = vkBeginCommandBuffer(cmd, &cmd_buf_info);
    assert(res == VK_SUCCESS);
}

void execute_end_command_buffer(VkCommandBuffer &cmd) {
    VkResult U_ASSERT_ONLY res;

    res = vkEndCommandBuffer(cmd);
    assert(res == VK_SUCCESS);
}

void execute_submit_queue_fenced(struct sample_info &info, const VkQueue &queue, const VkCommandBuffer &cmd_buf) {
    VkResult U_ASSERT_ONLY res;

    /* Queue the command buffer for execution */
    VkFenceCreateInfo fenceInfo;
    VkFence fence;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;
    vkCreateFence(info.device, &fenceInfo, NULL, &fence);

    VkSubmitInfo submit_info = {};
    submit_info.pNext = NULL;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 0;
    submit_info.pWaitSemaphores = NULL;
    submit_info.pWaitDstStageMask = 0;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buf;
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;

    res = vkQueueSubmit(queue, 1, &submit_info, fence);
    assert(res == VK_SUCCESS);

    do {
        res = vkWaitForFences(info.device, 1, &fence, VK_TRUE, FENCE_TIMEOUT);
    } while (res == VK_TIMEOUT);
    assert(res == VK_SUCCESS);

    vkDestroyFence(info.device, fence, NULL);
}

void execute_queue_command_buffer(struct sample_info &info, VkQueue &queue, VkCommandBuffer cmd_bufs[]) {
    VkResult U_ASSERT_ONLY res;

    /* Queue the command buffer for execution */
    VkFenceCreateInfo fenceInfo;
    VkFence drawFence;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;
    vkCreateFence(info.device, &fenceInfo, NULL, &drawFence);

    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submit_info[1] = {};
    submit_info[0].pNext = NULL;
    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info[0].waitSemaphoreCount = 0;
    submit_info[0].pWaitSemaphores = NULL;
    submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
    submit_info[0].commandBufferCount = 1;
    submit_info[0].pCommandBuffers = cmd_bufs;
    submit_info[0].signalSemaphoreCount = 0;
    submit_info[0].pSignalSemaphores = NULL;

    res = vkQueueSubmit(queue, 1, submit_info, drawFence);
    assert(res == VK_SUCCESS);

    do {
        res = vkWaitForFences(info.device, 1, &drawFence, VK_TRUE, FENCE_TIMEOUT);
    } while (res == VK_TIMEOUT);
    assert(res == VK_SUCCESS);

    vkDestroyFence(info.device, drawFence, NULL);
}

void init_device_queues(struct sample_info &info) {
    /* DEPENDS init_device() */

    std::set<uint32_t> unique_queue_families = {
        info.graphics_queue_family_index,
        info.present_queue_family_index,
        info.transfer_queue_family_index,
    };

    info.queues.resize(unique_queue_families.size());
    for(size_t i = 0; i < unique_queue_families.size(); i++) {
        vkGetDeviceQueue(info.device, i, 0, &info.queues[i]);
    }
}

void init_vertex_buffer(struct sample_info &info, const void *vertexData, uint32_t dataSize, uint32_t dataStride,
                        bool use_texture) {
    VkResult U_ASSERT_ONLY res;
    bool U_ASSERT_ONLY pass;

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = NULL;
    buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buf_info.size = dataSize;
    buf_info.queueFamilyIndexCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    res = vkCreateBuffer(info.device, &buf_info, NULL, &info.vertex_buffer.buf);
    assert(res == VK_SUCCESS);

    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(info.device, info.vertex_buffer.buf, &mem_reqs);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    alloc_info.allocationSize = mem_reqs.size;
    pass = memory_type_from_properties(info, mem_reqs.memoryTypeBits,
                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                       &alloc_info.memoryTypeIndex);
    assert(pass && "No mappable, coherent memory");

    res = vkAllocateMemory(info.device, &alloc_info, NULL, &(info.vertex_buffer.mem));
    assert(res == VK_SUCCESS);
    info.vertex_buffer.buffer_info.range = mem_reqs.size;
    info.vertex_buffer.buffer_info.offset = 0;

    uint8_t *pData;
    res = vkMapMemory(info.device, info.vertex_buffer.mem, 0, mem_reqs.size, 0, (void **)&pData);
    assert(res == VK_SUCCESS);

    memcpy(pData, vertexData, dataSize);

    vkUnmapMemory(info.device, info.vertex_buffer.mem);

    res = vkBindBufferMemory(info.device, info.vertex_buffer.buf, info.vertex_buffer.mem, 0);
    assert(res == VK_SUCCESS);

    info.vi_binding.binding = 0;
    info.vi_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    info.vi_binding.stride = dataStride;

    info.vi_attribs[0].binding = 0;
    info.vi_attribs[0].location = 0;
    info.vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[0].offset = 0;
    info.vi_attribs[1].binding = 0;
    info.vi_attribs[1].location = 1;
    info.vi_attribs[1].format = use_texture ? VK_FORMAT_R32G32_SFLOAT : VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[1].offset = 16;
}

void init_descriptor_pool(struct sample_info &info, bool use_texture) {
    /* DEPENDS on init_uniform_buffer() and
     * init_descriptor_and_pipeline_layouts() */

    VkResult U_ASSERT_ONLY res;
    VkDescriptorPoolSize type_count[2];
    type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_count[0].descriptorCount = 1;
    if (use_texture) {
        type_count[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        type_count[1].descriptorCount = 1;
    }

    VkDescriptorPoolCreateInfo descriptor_pool = {};
    descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool.pNext = NULL;
    descriptor_pool.maxSets = 1;
    descriptor_pool.poolSizeCount = use_texture ? 2 : 1;
    descriptor_pool.pPoolSizes = type_count;

    res = vkCreateDescriptorPool(info.device, &descriptor_pool, NULL, &info.desc_pool);
    assert(res == VK_SUCCESS);
}

void init_descriptor_set(struct sample_info &info, bool use_texture) {
    /* DEPENDS on init_descriptor_pool() */

    VkResult U_ASSERT_ONLY res;

    VkDescriptorSetAllocateInfo alloc_info[1];
    alloc_info[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info[0].pNext = NULL;
    alloc_info[0].descriptorPool = info.desc_pool;
    alloc_info[0].descriptorSetCount = NUM_DESCRIPTOR_SETS;
    alloc_info[0].pSetLayouts = info.desc_layout.data();

    info.desc_set.resize(NUM_DESCRIPTOR_SETS);
    res = vkAllocateDescriptorSets(info.device, alloc_info, info.desc_set.data());
    assert(res == VK_SUCCESS);

    VkWriteDescriptorSet writes[2];

    writes[0] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].pNext = NULL;
    writes[0].dstSet = info.desc_set[0];
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pBufferInfo = &info.uniform_data.buffer_info;

    if (use_texture) {
        writes[1] = {};
        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = info.desc_set[0];
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorCount = 1;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].pImageInfo = &info.textures[0].img_desc_info;
    }

    vkUpdateDescriptorSets(info.device, use_texture ? 2 : 1, writes, 0, NULL);
}

void init_shaders(struct sample_info &info, const char* vertShaderText, const char* fragShaderText) {
    VkResult U_ASSERT_ONLY res;
    bool U_ASSERT_ONLY retVal;

    const char *xxx =
        "#version 450\n"
        "#extension GL_ARB_separate_shader_objects : enable\n"
        "layout(binding = 0) uniform UniformBufferObject {\n"
        "    mat4 model;\n"
        "    mat4 view;\n"
        "    mat4 proj;\n"
        "} ubo;\n"
        "layout(location = 0) in vec3 inPosition;\n"
        "layout(location = 1) in vec3 inColor;\n"
        "layout(location = 2) in vec2 inTexCoord;\n"
        "layout(location = 0) out vec3 fragColor;\n"
        "layout(location = 1) out vec2 fragTexCoord;\n"
        "out gl_PerVertex {\n"
        "    vec4 gl_Position;\n"
        "};\n"
        "void main() {\n"
        "    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(inPosition, 1.0);\n"
        "    fragColor = inColor;\n"
        "    fragTexCoord = inTexCoord;\n"
        "}\n";

    // If no shaders were submitted, just return
    if (!(vertShaderText || fragShaderText)) return;

    init_glslang();
    VkShaderModuleCreateInfo moduleCreateInfo;
    info.shaderStages.resize(2); // TODO: this is a bit wonky methinks

    if (vertShaderText) {
        std::vector<unsigned int> vtx_spv;
        info.shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.shaderStages[0].pNext = NULL;
        info.shaderStages[0].pSpecializationInfo = NULL;
        info.shaderStages[0].flags = 0;
        info.shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        info.shaderStages[0].pName = "main";

        retVal = GLSLtoSPV(VK_SHADER_STAGE_VERTEX_BIT, vertShaderText, vtx_spv);
        assert(retVal);

        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.pNext = NULL;
        moduleCreateInfo.flags = 0;
        moduleCreateInfo.codeSize = vtx_spv.size() * sizeof(unsigned int);
        moduleCreateInfo.pCode = vtx_spv.data();
        res = vkCreateShaderModule(info.device, &moduleCreateInfo, NULL, &info.shaderStages[0].module);
        assert(res == VK_SUCCESS);
    }

    if (fragShaderText) {
        std::vector<unsigned int> frag_spv;
        info.shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        info.shaderStages[1].pNext = NULL;
        info.shaderStages[1].pSpecializationInfo = NULL;
        info.shaderStages[1].flags = 0;
        info.shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        info.shaderStages[1].pName = "main";

        retVal = GLSLtoSPV(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderText, frag_spv);
        assert(retVal);

        moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleCreateInfo.pNext = NULL;
        moduleCreateInfo.flags = 0;
        moduleCreateInfo.codeSize = frag_spv.size() * sizeof(unsigned int);
        moduleCreateInfo.pCode = frag_spv.data();
        res = vkCreateShaderModule(info.device, &moduleCreateInfo, NULL, &info.shaderStages[1].module);
        assert(res == VK_SUCCESS);
    }

    finalize_glslang();
}

void init_pipeline_cache(struct sample_info &info) {
    VkResult U_ASSERT_ONLY res;

    VkPipelineCacheCreateInfo pipeline_cache_info;
    pipeline_cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipeline_cache_info.pNext = NULL;
    pipeline_cache_info.initialDataSize = 0;
    pipeline_cache_info.pInitialData = NULL;
    pipeline_cache_info.flags = 0;

    res = vkCreatePipelineCache(info.device, &pipeline_cache_info, NULL, &info.pipelineCache);
    assert(res == VK_SUCCESS);
}

void init_sampler(struct sample_info &info, VkSampler &sampler) {
    VkResult U_ASSERT_ONLY res;

    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.minFilter = VK_FILTER_NEAREST;
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerCreateInfo.mipLodBias = 0.0;
    samplerCreateInfo.anisotropyEnable = VK_FALSE;
    samplerCreateInfo.maxAnisotropy = 1;
    samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerCreateInfo.minLod = 0.0;
    samplerCreateInfo.maxLod = 0.0;
    samplerCreateInfo.compareEnable = VK_FALSE;
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    /* create sampler */
    res = vkCreateSampler(info.device, &samplerCreateInfo, NULL, &sampler);
    assert(res == VK_SUCCESS);
}

//void init_image(struct sample_info &info, texture_object &texObj, const char *textureName, VkImageUsageFlags extraUsages,
//                VkFormatFeatureFlags extraFeatures) {
//    VkResult U_ASSERT_ONLY res;
//    bool U_ASSERT_ONLY pass;
//    std::string filename = get_base_data_dir();
//
//    if (textureName == nullptr)
//        filename.append("lunarg.ppm");
//    else
//        filename.append(textureName);
//
//    //if (!read_ppm(filename.c_str(), static_cast<int32_t>(texObj.tex_width), static_cast<int32_t>(texObj.tex_height), 0, NULL)) {
//    //    std::cout << "Try relative path\n";
//    //    filename = "../../API-Samples/data/";
//    //    if (textureName == nullptr)
//    //        filename.append("lunarg.ppm");
//    //    else
//    //        filename.append(textureName);
//    //    if (!read_ppm(filename.c_str(), texObj.tex_width, texObj.tex_height, 0, NULL)) {
//    //        std::cout << "Could not read texture file " << filename;
//    //        exit(-1);
//    //    }
//    //}
//
//    VkFormatProperties formatProps;
//    vkGetPhysicalDeviceFormatProperties(info.physical_device_properties[0].device, VK_FORMAT_R8G8B8A8_UNORM, &formatProps);
//
//    /* See if we can use a linear tiled image for a texture, if not, we will
//     * need a staging image for the texture data */
//    VkFormatFeatureFlags allFeatures = (VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | extraFeatures);
//    bool needStaging = ((formatProps.linearTilingFeatures & allFeatures) != allFeatures) ? true : false;
//
//    if (needStaging) {
//        assert((formatProps.optimalTilingFeatures & allFeatures) == allFeatures);
//    }
//
//    VkImageCreateInfo image_create_info = {};
//    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//    image_create_info.pNext = NULL;
//    image_create_info.imageType = VK_IMAGE_TYPE_2D;
//    image_create_info.format = VK_FORMAT_R8G8B8A8_UNORM;
//    image_create_info.extent.width = texObj.tex_width;
//    image_create_info.extent.height = texObj.tex_height;
//    image_create_info.extent.depth = 1;
//    image_create_info.mipLevels = 1;
//    image_create_info.arrayLayers = 1;
//    image_create_info.samples = info.num_samples; // NUM_SAMPLES;
//    image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
//    image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
//    image_create_info.usage =
//        needStaging ? (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | extraUsages) : (VK_IMAGE_USAGE_SAMPLED_BIT | extraUsages);
//    image_create_info.queueFamilyIndexCount = 0;
//    image_create_info.pQueueFamilyIndices = NULL;
//    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//    image_create_info.flags = 0;
//
//    VkMemoryAllocateInfo mem_alloc = {};
//    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//    mem_alloc.pNext = NULL;
//    mem_alloc.allocationSize = 0;
//    mem_alloc.memoryTypeIndex = 0;
//
//    VkImage mappableImage;
//    VkDeviceMemory mappableMemory;
//
//    VkMemoryRequirements mem_reqs;
//
//    /* Create a mappable image.  It will be the texture if linear images are ok
//     * to be textures or it will be the staging image if they are not. */
//    res = vkCreateImage(info.device, &image_create_info, NULL, &mappableImage);
//    assert(res == VK_SUCCESS);
//
//    vkGetImageMemoryRequirements(info.device, mappableImage, &mem_reqs);
//    assert(res == VK_SUCCESS);
//
//    mem_alloc.allocationSize = mem_reqs.size;
//
//    /* Find the memory type that is host mappable */
//    pass = memory_type_from_properties(info, mem_reqs.memoryTypeBits,
//                                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//                                       &mem_alloc.memoryTypeIndex);
//    assert(pass && "No mappable, coherent memory");
//
//    /* allocate memory */
//    res = vkAllocateMemory(info.device, &mem_alloc, NULL, &(mappableMemory));
//    assert(res == VK_SUCCESS);
//
//    /* bind memory */
//    res = vkBindImageMemory(info.device, mappableImage, mappableMemory, 0);
//    assert(res == VK_SUCCESS);
//
//    res = vkEndCommandBuffer(info.cmds[0]);
//    assert(res == VK_SUCCESS);
//    const VkCommandBuffer cmd_bufs[] = { info.cmds[0] };
//    VkFenceCreateInfo fenceInfo;
//    VkFence cmdFence;
//    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
//    fenceInfo.pNext = NULL;
//    fenceInfo.flags = 0;
//    vkCreateFence(info.device, &fenceInfo, NULL, &cmdFence);
//
//    VkSubmitInfo submit_info[1] = {};
//    submit_info[0].pNext = NULL;
//    submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//    submit_info[0].waitSemaphoreCount = 0;
//    submit_info[0].pWaitSemaphores = NULL;
//    submit_info[0].pWaitDstStageMask = NULL;
//    submit_info[0].commandBufferCount = 1;
//    submit_info[0].pCommandBuffers = cmd_bufs;
//    submit_info[0].signalSemaphoreCount = 0;
//    submit_info[0].pSignalSemaphores = NULL;
//
//    /* Queue the command buffer for execution (graphics) */
//    res = vkQueueSubmit(info.queues[0], 1, submit_info, cmdFence);
//    assert(res == VK_SUCCESS);
//
//    VkImageSubresource subres = {};
//    subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//    subres.mipLevel = 0;
//    subres.arrayLayer = 0;
//
//    VkSubresourceLayout layout;
//    void *data;
//
//    /* Get the subresource layout so we know what the row pitch is */
//    vkGetImageSubresourceLayout(info.device, mappableImage, &subres, &layout);
//
//    /* Make sure command buffer is finished before mapping */
//    do {
//        res = vkWaitForFences(info.device, 1, &cmdFence, VK_TRUE, FENCE_TIMEOUT);
//    } while (res == VK_TIMEOUT);
//    assert(res == VK_SUCCESS);
//
//    vkDestroyFence(info.device, cmdFence, NULL);
//
//    res = vkMapMemory(info.device, mappableMemory, 0, mem_reqs.size, 0, &data);
//    assert(res == VK_SUCCESS);
//
//    ///* Read the ppm file into the mappable image's memory */
//    //if (!read_ppm(filename.c_str(), texObj.tex_width, texObj.tex_height, layout.rowPitch, (unsigned char *)data)) {
//    //    std::cout << "Could not load texture file lunarg.ppm\n";
//    //    exit(-1);
//    //}
//
//    vkUnmapMemory(info.device, mappableMemory);
//
//    VkCommandBufferBeginInfo cmd_buf_info = {};
//    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//    cmd_buf_info.pNext = NULL;
//    cmd_buf_info.flags = 0;
//    cmd_buf_info.pInheritanceInfo = NULL;
//
//    res = vkResetCommandBuffer(info.cmds[0], 0);
//    res = vkBeginCommandBuffer(info.cmds[0], &cmd_buf_info);
//    assert(res == VK_SUCCESS);
//
//    if (!needStaging) {
//        /* If we can use the linear tiled image as a texture, just do it */
//        texObj.image = mappableImage;
//        texObj.mem = mappableMemory;
//        texObj.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//        set_image_layout(info, info.cmds[0], texObj.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED, texObj.imageLayout,
//                         VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
//        /* No staging resources to free later */
//        info.stagingImage = VK_NULL_HANDLE;
//        info.stagingMemory = VK_NULL_HANDLE;
//    } else {
//        /* The mappable image cannot be our texture, so create an optimally
//         * tiled image and blit to it */
//        image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
//        image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
//        image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//
//        res = vkCreateImage(info.device, &image_create_info, NULL, &texObj.image);
//        assert(res == VK_SUCCESS);
//
//        vkGetImageMemoryRequirements(info.device, texObj.image, &mem_reqs);
//
//        mem_alloc.allocationSize = mem_reqs.size;
//
//        /* Find memory type - dont specify any mapping requirements */
//        pass = memory_type_from_properties(info, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//                                           &mem_alloc.memoryTypeIndex);
//        assert(pass);
//
//        /* allocate memory */
//        res = vkAllocateMemory(info.device, &mem_alloc, NULL, &texObj.mem);
//        assert(res == VK_SUCCESS);
//
//        /* bind memory */
//        res = vkBindImageMemory(info.device, texObj.image, texObj.mem, 0);
//        assert(res == VK_SUCCESS);
//
//        /* Since we're going to blit from the mappable image, set its layout to
//         * SOURCE_OPTIMAL. Side effect is that this will create info.cmd */
//        set_image_layout(info, info.cmds[0], mappableImage, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PREINITIALIZED,
//                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
//
//        /* Since we're going to blit to the texture image, set its layout to
//         * DESTINATION_OPTIMAL */
//        set_image_layout(info, info.cmds[0], texObj.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_UNDEFINED,
//                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
//
//        VkImageCopy copy_region;
//        copy_region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//        copy_region.srcSubresource.mipLevel = 0;
//        copy_region.srcSubresource.baseArrayLayer = 0;
//        copy_region.srcSubresource.layerCount = 1;
//        copy_region.srcOffset.x = 0;
//        copy_region.srcOffset.y = 0;
//        copy_region.srcOffset.z = 0;
//        copy_region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//        copy_region.dstSubresource.mipLevel = 0;
//        copy_region.dstSubresource.baseArrayLayer = 0;
//        copy_region.dstSubresource.layerCount = 1;
//        copy_region.dstOffset.x = 0;
//        copy_region.dstOffset.y = 0;
//        copy_region.dstOffset.z = 0;
//        copy_region.extent.width = texObj.tex_width;
//        copy_region.extent.height = texObj.tex_height;
//        copy_region.extent.depth = 1;
//
//        /* Put the copy command into the command buffer */
//        vkCmdCopyImage(info.cmds[0], mappableImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, texObj.image,
//                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy_region);
//
//        /* Set the layout for the texture image from DESTINATION_OPTIMAL to
//         * SHADER_READ_ONLY */
//        texObj.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//        set_image_layout(info, info.cmds[0], texObj.image, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, texObj.imageLayout,
//                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
//
//        /* Remember staging resources to free later */
//        info.stagingImage = mappableImage;
//        info.stagingMemory = mappableMemory;
//    }
//
//    VkImageViewCreateInfo view_info = {};
//    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//    view_info.pNext = NULL;
//    view_info.image = VK_NULL_HANDLE;
//    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
//    view_info.format = VK_FORMAT_R8G8B8A8_UNORM;
//    view_info.components.r = VK_COMPONENT_SWIZZLE_R;
//    view_info.components.g = VK_COMPONENT_SWIZZLE_G;
//    view_info.components.b = VK_COMPONENT_SWIZZLE_B;
//    view_info.components.a = VK_COMPONENT_SWIZZLE_A;
//    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//    view_info.subresourceRange.baseMipLevel = 0;
//    view_info.subresourceRange.levelCount = 1;
//    view_info.subresourceRange.baseArrayLayer = 0;
//    view_info.subresourceRange.layerCount = 1;
//
//    /* create image view */
//    view_info.image = texObj.image;
//    res = vkCreateImageView(info.device, &view_info, NULL, &texObj.view);
//    assert(res == VK_SUCCESS);
//}

//void init_texture(struct sample_info &info, const char *textureName, VkImageUsageFlags extraUsages,
//                  VkFormatFeatureFlags extraFeatures) {
//    struct texture_object texObj;
//
//    /* create image */
//    init_image(info, texObj, textureName, extraUsages, extraFeatures);
//
//    /* create sampler */
//    init_sampler(info, texObj.sampler);
//
//    info.textures.push_back(texObj);
//
//    /* track a description of the texture */
//    info.texture_data.image_info.imageView = info.textures.back().view;
//    info.texture_data.image_info.sampler = info.textures.back().sampler;
//    info.texture_data.image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//}

void init_viewports(struct sample_info &info) {
#ifdef __ANDROID__
// Disable dynamic viewport on Android. Some drive has an issue with the dynamic viewport
// feature.
#else
    info.viewport.height = (float)info.height;
    info.viewport.width = (float)info.width;
    info.viewport.minDepth = (float)0.0f;
    info.viewport.maxDepth = (float)1.0f;
    info.viewport.x = 0;
    info.viewport.y = 0;
    // TODO: not sure if this should be graphics or present queue command.
    vkCmdSetViewport(info.cmds[info.graphics_queue_family_index], 0, NUM_VIEWPORTS, &info.viewport);
#endif
}

void init_scissors(struct sample_info &info) {
#ifdef __ANDROID__
// Disable dynamic viewport on Android. Some drive has an issue with the dynamic scissors
// feature.
#else
    info.scissor.extent.width = info.width;
    info.scissor.extent.height = info.height;
    info.scissor.offset.x = 0;
    info.scissor.offset.y = 0;
    // TODO: not sure if this should be graphics or present queue command.
    vkCmdSetScissor(info.cmds[info.graphics_queue_family_index], 0, NUM_SCISSORS, &info.scissor);
#endif
}

void init_fence(struct sample_info &info, VkFence &fence) {
    VkFenceCreateInfo fenceInfo;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;
    vkCreateFence(info.device, &fenceInfo, NULL, &fence);
}

void init_submit_info(struct sample_info &info, VkSubmitInfo &submit_info, VkPipelineStageFlags &pipe_stage_flags) {
    submit_info.pNext = NULL;
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &info.imageAcquiredSemaphore;
    submit_info.pWaitDstStageMask = &pipe_stage_flags;
    submit_info.commandBufferCount = 1;
    // TODO: I think this should always be the graphics queue?
    submit_info.pCommandBuffers = &info.cmds[info.graphics_queue_family_index];
    submit_info.signalSemaphoreCount = 0;
    submit_info.pSignalSemaphores = NULL;
}

void init_present_info(struct sample_info &info, VkPresentInfoKHR &present) {
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.pSwapchains = &info.swap_chain;
    present.pImageIndices = &info.current_buffer;
    present.pWaitSemaphores = NULL;
    present.waitSemaphoreCount = 0;
    present.pResults = NULL;
}

void init_clear_color_and_depth(struct sample_info &info, VkClearValue *clear_values) {
    clear_values[0].color.float32[0] = 0.2f;
    clear_values[0].color.float32[1] = 0.2f;
    clear_values[0].color.float32[2] = 0.2f;
    clear_values[0].color.float32[3] = 0.2f;
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;
}

void init_render_pass_begin_info(struct sample_info &info, VkRenderPassBeginInfo &rp_begin) {
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = info.render_pass;
    rp_begin.framebuffer = info.framebuffers[info.current_buffer];
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = info.width;
    rp_begin.renderArea.extent.height = info.height;
    rp_begin.clearValueCount = 0;
    rp_begin.pClearValues = nullptr;
}

void init_validation_layers(struct sample_info &info, PFN_vkDebugUtilsMessengerCallbackEXT callback,
                            PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessenger) {
    /* depends on init_instance_layer_names() */

    VkResult U_ASSERT_ONLY res;
    for (const char *layerName : info.instance_layer_names) {
        // TODO: this will only work for the one layer unless revisited

        VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
        debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;  // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugInfo.pfnUserCallback = callback;
        debugInfo.pUserData = nullptr;  // Optional

        res = createDebugUtilsMessenger(info.inst, &debugInfo, NULL, &info.callback);
        assert(res == VK_SUCCESS);  // , "failed to setup debug callback!");
    }
}

void destroy_pipeline(struct sample_info &info) { vkDestroyPipeline(info.device, info.pipeline, NULL); }

void destroy_pipeline_cache(struct sample_info &info) { vkDestroyPipelineCache(info.device, info.pipelineCache, NULL); }

void destroy_uniform_buffer(struct sample_info &info) {
    vkDestroyBuffer(info.device, info.uniform_data.buf, NULL);
    vkFreeMemory(info.device, info.uniform_data.mem, NULL);
}

void destroy_descriptor_and_pipeline_layouts(struct sample_info &info) {
    for (int i = 0; i < NUM_DESCRIPTOR_SETS; i++) vkDestroyDescriptorSetLayout(info.device, info.desc_layout[i], NULL);
    vkDestroyPipelineLayout(info.device, info.pipeline_layout, NULL);
}

void destroy_descriptor_pool(struct sample_info &info) { vkDestroyDescriptorPool(info.device, info.desc_pool, NULL); }

void destroy_shaders(struct sample_info &info) {
    for (auto &shaderStageInfo : info.shaderStages) vkDestroyShaderModule(info.device, shaderStageInfo.module, NULL);
}

void destroy_command_buffers(struct sample_info &info) {
    for (size_t i = 0; i < info.cmd_pools.size(); i++) {
        vkFreeCommandBuffers(info.device, info.cmd_pools[i], 1, &info.cmds[i]);
    }
}

void destroy_command_pools(struct sample_info &info) {
    for (auto &cmd_pool : info.cmd_pools) {
        vkDestroyCommandPool(info.device, cmd_pool, NULL);
    }
}

void destroy_depth_buffer(struct sample_info &info) {
    // TODO: what about the format member?
    vkDestroyImageView(info.device, info.depth.view, NULL);
    vkDestroyImage(info.device, info.depth.image, NULL);
    vkFreeMemory(info.device, info.depth.mem, NULL);
}

void destroy_color_buffer(struct sample_info &info) {
    // TODO: what about the format member?
    vkDestroyImageView(info.device, info.color.view, NULL);
    vkDestroyImage(info.device, info.color.image, NULL);
    vkFreeMemory(info.device, info.color.mem, NULL);
}

void destroy_staging_buffers(struct sample_info &info) {
    // TODO: some kind of mutex
    for (auto& staging_buf : info.staging_buffers) {
        vkDestroyBuffer(info.device, staging_buf.buf, NULL);
        vkFreeMemory(info.device, staging_buf.mem, NULL);
    }
    info.staging_buffers.clear();
}

void destroy_vertex_buffer(struct sample_info &info) {
    vkDestroyBuffer(info.device, info.vertex_buffer.buf, NULL);
    vkFreeMemory(info.device, info.vertex_buffer.mem, NULL);
}

void destroy_swapchain(struct sample_info &info) {
    for (uint32_t i = 0; i < info.swapchainImageCount; i++) {
        vkDestroyImageView(info.device, info.buffers[i].view, NULL);
    }
    vkDestroySwapchainKHR(info.device, info.swap_chain, NULL);
}

void destroy_framebuffers(struct sample_info &info) {
    for (uint32_t i = 0; i < info.swapchainImageCount; i++) vkDestroyFramebuffer(info.device, info.framebuffers[i], NULL);
}

void destroy_renderpass(struct sample_info &info) { vkDestroyRenderPass(info.device, info.render_pass, NULL); }

void destroy_device(struct sample_info &info) {
    vkDeviceWaitIdle(info.device);
    vkDestroyDevice(info.device, NULL);
}

void destroy_instance(struct sample_info &info) { vkDestroyInstance(info.inst, NULL); }

void destroy_textures(struct sample_info &info) {
    for (size_t i = 0; i < info.textures.size(); i++) {
        vkDestroySampler(info.device, info.textures[i].sampler, NULL);
        vkDestroyImageView(info.device, info.textures[i].view, NULL);
        vkDestroyImage(info.device, info.textures[i].image, NULL);
        vkFreeMemory(info.device, info.textures[i].mem, NULL);
    }
}

void destroy_validation_layers(struct sample_info &info, const VkAllocationCallbacks *pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(info.inst, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(info.inst, info.callback, pAllocator);
    }
}

// QueueFamilyIndices Guppy::findQueueFamilies(VkPhysicalDevice device)
//{
//    QueueFamilyIndices indices;
//
//    uint32_t queueFamilyCount = 0;
//    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
//
//    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
//    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
//
//
//    int i = 0;
//    for (const auto& queueFamily : queueFamilies)
//    {
//        // graphics
//        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
//        {
//            indices.graphicsFamily = i;
//        }
//
//        // present
//        VkBool32 presentSupport = false;
//        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
//        if (queueFamily.queueCount > 0 && presentSupport)
//        {
//            indices.presentFamily = i;
//        }
//
//        // transfer
//        if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 && queueFamily.queueFlags &
//        VK_QUEUE_TRANSFER_BIT)
//        {
//            indices.transferFamily = i;
//        }
//
//        if (indices.isComplete())
//        {
//            break;
//        }
//
//        i++;
//    }
//
//    return indices;
//}
