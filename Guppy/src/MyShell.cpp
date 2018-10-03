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

#include <cassert>
#include <array>
#include <iostream>
#include <string>
#include <sstream>
#include <set>

#include "EventHandlers.h"
#include "Game.h"
#include "Helpers.h"
#include "InputHandler.h"
#include "MyShell.h"
#include "util.hpp"

MyShell::MyShell(Game &game)
    : game_(game), settings_(game.settings()), ctx_(), game_tick_(1.0f / settings_.ticks_per_second), game_time_(game_tick_) {
    // require generic WSI extensions
    instance_extensions_.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    device_extensions_.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    // require "standard" validation layers
    if (settings_.validate) {
        instance_layers_.push_back("VK_LAYER_LUNARG_standard_validation");
        instance_extensions_.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
        instance_extensions_.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    InputHandler::get(this);
}

void MyShell::log(LogPriority priority, const char *msg) const {
    std::ostream &st = (priority >= LOG_ERR) ? std::cerr : std::cout;
    st << msg << "\n";
}

void MyShell::init_vk() {
    init_instance();

    init_debug_report();
    init_validation_messenger();

    init_physical_dev();
}

void MyShell::cleanup_vk() {
    if (settings_.validate) DestoryDebugReportCallbackEXT(ctx_.instance, ctx_.debug_report, nullptr);
    if (settings_.validate) DestroyDebugUtilsMessengerEXT(ctx_.instance, ctx_.debug_utils_messenger, nullptr);

    destroy_instance();
}

bool MyShell::debug_report_callback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT obj_type, uint64_t object,
                                    size_t location, int32_t msg_code, const char *layer_prefix, const char *msg) {
    LogPriority prio = LOG_WARN;
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
        prio = LOG_ERR;
    else if (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT))
        prio = LOG_WARN;
    else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)
        prio = LOG_INFO;
    else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
        prio = LOG_DEBUG;

    std::stringstream ss;
    ss << layer_prefix << ": " << msg;

    log(prio, ss.str().c_str());

    return false;
}

void MyShell::assert_all_instance_layers() const {
    // Check if all instance layers required are present
    for (const auto &layer_name : instance_layers_) {
        auto it = std::find_if(layerProps_.begin(), layerProps_.end(), [&layer_name](auto layer_prop) {
            return std::strcmp(layer_prop.properties.layerName, layer_name) == 0;
        });
        if (it == layerProps_.end()) {
            std::stringstream ss;
            ss << "instance layer " << layer_name << " is missing";
            throw std::runtime_error(ss.str());
        }
    }
}

void MyShell::assert_all_instance_extensions() const {
    // Check if all instance extensions required are present
    for (const auto &extensionName : instance_extensions_) {
        auto it = std::find_if(instExtProps_.begin(), instExtProps_.end(),
                               [&extensionName](auto ext) { return std::strcmp(extensionName, ext.extensionName) == 0; });
        if (it == instExtProps_.end()) {
            std::stringstream ss;
            ss << "instance extension " << extensionName << " is missing";
            throw std::runtime_error(ss.str());
        }
    }
}

bool MyShell::has_all_device_layers(VkPhysicalDevice phy) const {
    // Something here?

    return true;
}

bool MyShell::has_all_device_extensions(VkPhysicalDevice phy) const {
    //// enumerate device extensions
    // std::vector<VkExtensionProperties> exts;
    // vk::enumerate(phy, nullptr, exts);

    // std::set<std::string> ext_names;
    // for (const auto &ext : exts) ext_names.insert(ext.extensionName);

    //// all listed device extensions are required
    // for (const auto &name : device_extensions_) {
    //    if (ext_names.find(name) == ext_names.end()) return false;
    //}

    return true;
}

void MyShell::init_instance() {
    // enumerate instance layer & extension properties
    enumerate_instance_properties();

    assert_all_instance_layers();
    assert_all_instance_extensions();

    uint32_t version = VK_VERSION_1_0;
    determine_api_version(version);

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = settings_.name.c_str();
    app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.pEngineName = "MyShell";
    app_info.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    app_info.apiVersion = version;

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = nullptr;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledLayerCount = static_cast<uint32_t>(instance_layers_.size());
    inst_info.ppEnabledLayerNames = instance_layers_.data();
    inst_info.enabledExtensionCount = static_cast<uint32_t>(instance_extensions_.size());
    inst_info.ppEnabledExtensionNames = instance_extensions_.data();

    VkResult res = vkCreateInstance(&inst_info, nullptr, &ctx_.instance);
    assert(res == VK_SUCCESS);
}

void MyShell::init_debug_report() {
    if (!settings_.validate) return;

    VkDebugReportCallbackCreateInfoEXT debug_report_info = {};
    debug_report_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;

    debug_report_info.flags =
        VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
    if (settings_.validate_verbose) {
        debug_report_info.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
    }

    debug_report_info.pfnCallback = debug_report_callback;
    debug_report_info.pUserData = reinterpret_cast<void *>(this);

    vk::assert_success(CreateDebugReportCallbackEXT(ctx_.instance, &debug_report_info, nullptr, &ctx_.debug_report));
}

void MyShell::init_validation_messenger() {
    if (!settings_.validate) return;

    VkDebugUtilsMessengerCreateInfoEXT debugInfo = {};
    debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;  // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
    debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugInfo.pfnUserCallback = debug_callback;
    debugInfo.pUserData = nullptr;  // Optional

    vk::assert_success(CreateDebugUtilsMessengerEXT(ctx_.instance, &debugInfo, nullptr, &ctx_.debug_utils_messenger));
}

void MyShell::init_physical_dev() {
    enumerate_physical_devs();

    ctx_.physical_dev = VK_NULL_HANDLE;
    pick_physical_dev();

    if (ctx_.physical_dev == VK_NULL_HANDLE) throw std::runtime_error("failed to find any capable Vulkan physical device");
}

void MyShell::create_context() {
    create_dev();

    init_dev_queues();

    create_back_buffers();

    // TODO: should this do this?
    // initialize ctx_.{surface,format} before attach_shell
    create_swapchain();

    game_.attach_shell(*this);
}

void MyShell::destroy_context() {
    if (ctx_.dev == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(ctx_.dev);

    destroy_swapchain();

    game_.detach_shell();

    destroy_back_buffers();

    // ctx_.game_queue = VK_NULL_HANDLE;
    // ctx_.present_queue = VK_NULL_HANDLE;

    vkDeviceWaitIdle(ctx_.dev);
    vkDestroyDevice(ctx_.dev, nullptr);
    ctx_.dev = VK_NULL_HANDLE;
}

void MyShell::create_dev() {
    // queue create info
    std::set<uint32_t> unique_queue_families = {
        ctx_.graphics_index,
        ctx_.present_index,
        ctx_.transfer_index,
    };
    float queue_priorities[1] = {0.0f};  // only one queue per family atm
    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    for (auto &index : unique_queue_families) {
        VkDeviceQueueCreateInfo queue_info = {};
        queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info.pNext = nullptr;
        queue_info.queueCount = 1;  // only one queue per family atm
        queue_info.queueFamilyIndex = index;
        queue_info.pQueuePriorities = queue_priorities;
        queue_infos.push_back(queue_info);
    }

    // features
    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = ctx_.sampler_anisotropy_enabled_ ? VK_TRUE : VK_FALSE;
    deviceFeatures.sampleRateShading = ctx_.sample_rate_shading_enabled_ ? VK_TRUE : VK_FALSE;

    VkDeviceCreateInfo dev_info = {};
    dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_info.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
    dev_info.pQueueCreateInfos = queue_infos.data();
    dev_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions_.size());
    dev_info.ppEnabledExtensionNames = device_extensions_.data();
    dev_info.pEnabledFeatures = &deviceFeatures;

    vk::assert_success(vkCreateDevice(ctx_.physical_dev, &dev_info, nullptr, &ctx_.dev));
}

void MyShell::create_back_buffers() {
    VkSemaphoreCreateInfo sem_info = {};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // BackBuffer is used to track which swapchain image and its associated
    // sync primitives are busy.  Having more BackBuffer's than swapchain
    // images may allow us to replace CPU wait on present_fence by GPU wait
    // on acquire_semaphore.
    const int count = settings_.back_buffer_count + 1;
    for (int i = 0; i < count; i++) {
        BackBuffer buf = {};
        vk::assert_success(vkCreateSemaphore(ctx_.dev, &sem_info, nullptr, &buf.acquire_semaphore));
        vk::assert_success(vkCreateSemaphore(ctx_.dev, &sem_info, nullptr, &buf.render_semaphore));
        vk::assert_success(vkCreateFence(ctx_.dev, &fence_info, nullptr, &buf.present_fence));

        ctx_.back_buffers.push(buf);
    }
}

void MyShell::destroy_back_buffers() {
    while (!ctx_.back_buffers.empty()) {
        const auto &buf = ctx_.back_buffers.front();

        vkDestroySemaphore(ctx_.dev, buf.acquire_semaphore, nullptr);
        vkDestroySemaphore(ctx_.dev, buf.render_semaphore, nullptr);
        vkDestroyFence(ctx_.dev, buf.present_fence, nullptr);

        ctx_.back_buffers.pop();
    }
}

// TODO: this doesn't create the swapchain
void MyShell::create_swapchain() {
    ctx_.surface = create_surface(ctx_.instance);

    VkBool32 supported;
    vk::assert_success(
        vkGetPhysicalDeviceSurfaceSupportKHR(ctx_.physical_dev, ctx_.present_queue_family, ctx_.surface, &supported));
    // this should be guaranteed by the platform-specific can_present call
    assert(supported);

    enumerate_surface_properties();
    determine_swapchain_surface_format();
    determine_swapchain_present_mode();
    determine_swapchain_image_count();
    determine_depth_format();

    // defer to resize_swapchain()
    ctx_.swapchain = VK_NULL_HANDLE;
    // ctx_.extent.width = UINT32_MAX;
    // ctx_.extent.height = UINT32_MAX;
}

void MyShell::destroy_swapchain() {
    if (ctx_.swapchain != VK_NULL_HANDLE) {
        game_.detach_swapchain();

        vkDestroySwapchainKHR(ctx_.dev, ctx_.swapchain, nullptr);
        ctx_.swapchain = VK_NULL_HANDLE;
    }

    vkDestroySurfaceKHR(ctx_.instance, ctx_.surface, nullptr);
    ctx_.surface = VK_NULL_HANDLE;
}

void MyShell::resize_swapchain(uint32_t width_hint, uint32_t height_hint, bool refresh_capabilities) {
    if (determine_swapchain_extent(width_hint, height_hint, refresh_capabilities)) return;

    auto &caps = ctx_.surface_props.capabilities;

    VkSurfaceTransformFlagBitsKHR preTransform;  // ex: rotate 90
    if (caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        preTransform = caps.currentTransform;
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
        if (caps.supportedCompositeAlpha & compositeAlphaFlags[i]) {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapchain_info = {};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = ctx_.surface;
    swapchain_info.minImageCount = ctx_.image_count;
    swapchain_info.imageFormat = ctx_.format.format;
    swapchain_info.imageColorSpace = ctx_.format.colorSpace;
    swapchain_info.imageExtent = ctx_.extent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.preTransform = caps.currentTransform;
    swapchain_info.compositeAlpha = compositeAlpha;
    swapchain_info.presentMode = ctx_.mode;
#ifndef __ANDROID__
    // Clip obscured pixels (by other windows for example). Only needed if the pixel info is reused.
    swapchain_info.clipped = true;
#else
    swapchain_info.clipped = false;
#endif
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.queueFamilyIndexCount = 0;
    swapchain_info.pQueueFamilyIndices = nullptr;

    if (ctx_.graphics_index != ctx_.present_index) {
        // If the graphics and present queues are from different queue families,
        // we either have to explicitly transfer ownership of images between the
        // queues, or we have to create the swapchain with imageSharingMode
        // as VK_SHARING_MODE_CONCURRENT
        uint32_t queue_families[2] = {ctx_.graphics_index, ctx_.present_index};
        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = 2;
        swapchain_info.pQueueFamilyIndices = queue_families;
    }

    swapchain_info.oldSwapchain = ctx_.swapchain;

    vk::assert_success(vkCreateSwapchainKHR(ctx_.dev, &swapchain_info, nullptr, &ctx_.swapchain));

    // destroy the old swapchain
    if (swapchain_info.oldSwapchain != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(ctx_.dev);

        game_.detach_swapchain();
        vkDestroySwapchainKHR(ctx_.dev, swapchain_info.oldSwapchain, nullptr);
    }

    game_.attach_swapchain();
}

void MyShell::add_game_time(float time) {
    int max_ticks = 3;

    if (!settings_.no_tick) game_time_ += time;

    while (game_time_ >= game_tick_ && max_ticks--) {
        game_.on_tick();
        game_time_ -= game_tick_;
    }
}

void MyShell::acquire_back_buffer() {
    // acquire just once when not presenting
    if (settings_.no_present && ctx_.acquired_back_buffer.acquire_semaphore != VK_NULL_HANDLE) return;

    auto &buf = ctx_.back_buffers.front();

    // wait until acquire and render semaphores are waited/unsignaled
    vk::assert_success(vkWaitForFences(ctx_.dev, 1, &buf.present_fence, true, UINT64_MAX));

    VkResult res = VK_TIMEOUT;  // Anything but VK_SUCCESS
    while (res != VK_SUCCESS) {
        res = vkAcquireNextImageKHR(ctx_.dev, ctx_.swapchain, UINT64_MAX, buf.acquire_semaphore, VK_NULL_HANDLE, &buf.image_index);
        if (res == VK_ERROR_OUT_OF_DATE_KHR) {
            // Swapchain is out of date (e.g. the window was resized) and
            // must be recreated:
            resize_swapchain(0, 0);  // width and height hints should be ignored
        } else {
            assert(!res);
        }
    }

    // reset the fence (AFTER POTENTIAL RECREATION OF SWAP CHAIN)
    vk::assert_success(vkResetFences(ctx_.dev, 1, &buf.present_fence));  // *

    ctx_.acquired_back_buffer = buf;
    ctx_.back_buffers.pop();
}

void MyShell::present_back_buffer() {
    const auto &buf = ctx_.acquired_back_buffer;

    if (!settings_.no_render) game_.on_frame(game_time_ / game_tick_);

    if (settings_.no_present) {
        fake_present();
        return;
    }

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = (settings_.no_render) ? &buf.acquire_semaphore : &buf.render_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &ctx_.swapchain;
    present_info.pImageIndices = &buf.image_index;

    VkResult res = vkQueuePresentKHR(ctx_.queues[ctx_.present_queue_family], &present_info);
    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        // Swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        resize_swapchain(0, 0);  // width and height hints should be ignored
    } else {
        assert(!res);
    }

    vk::assert_success(vkQueueSubmit(ctx_.queues[ctx_.present_queue_family], 0, nullptr, buf.present_fence));
    ctx_.back_buffers.push(buf);
}

// TODO: need this???
void MyShell::fake_present() {
    // const auto &buf = ctx_.acquired_back_buffer;

    // assert(settings_.no_present);

    //// wait render semaphore and signal acquire semaphore
    // if (!settings_.no_render) {
    //    VkPipelineStageFlags stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    //    VkSubmitInfo submit_info = {};
    //    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    //    submit_info.waitSemaphoreCount = 1;
    //    submit_info.pWaitSemaphores = &buf.render_semaphore;
    //    submit_info.pWaitDstStageMask = &stage;
    //    submit_info.signalSemaphoreCount = 1;
    //    submit_info.pSignalSemaphores = &buf.acquire_semaphore;
    //    vk::assert_success(vk::QueueSubmit(ctx_.game_queue, 1, &submit_info, VK_NULL_HANDLE));
    //}

    //// push the buffer back just once for MyShell::cleanup_vk
    // if (buf.acquire_semaphore != ctx_.back_buffers.back().acquire_semaphore) ctx_.back_buffers.push(buf);
}

// ********
// NEW FUNCTIONS FROM MY OTHER INIT CODE
// ********

void MyShell::init_dev_queues() {
    std::set<uint32_t> unique_queue_families = {
        ctx_.graphics_index,
        ctx_.present_index,
        ctx_.transfer_index,
    };
    ctx_.queues.resize(unique_queue_families.size());
    for (size_t i = 0; i < unique_queue_families.size(); i++) {
        vkGetDeviceQueue(ctx_.dev, i, 0, &ctx_.queues[i]);
    }
}

void MyShell::destroy_instance() { vkDestroyInstance(ctx_.instance, nullptr); }

void MyShell::enumerate_instance_properties() {
    uint32_t instance_layer_count;
    VkLayerProperties *vk_props = nullptr;
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
    vk::assert_success(vkEnumerateInstanceLayerProperties(&instance_layer_count, nullptr));

    vk_props = (VkLayerProperties *)realloc(vk_props, instance_layer_count * sizeof(VkLayerProperties));
    vk::assert_success(vkEnumerateInstanceLayerProperties(&instance_layer_count, vk_props));

    /*
     * Now gather the extension list for each instance layer.
     */
    for (uint32_t i = 0; i < instance_layer_count; i++) {
        LayerProperties layer_props;
        layer_props.properties = vk_props[i];
        enumerate_instance_layer_extension_properties(layer_props);
        layerProps_.push_back(layer_props);
    }

    // Get instance extension properties
    enumerate_instance_extension_properties();

    free(vk_props);
}

void MyShell::enumerate_instance_layer_extension_properties(LayerProperties &layer_props) {
    VkExtensionProperties *instance_extensions;
    uint32_t instance_extension_count;
    VkResult res;
    char *layer_name = layer_props.properties.layerName;

    do {
        // This could be cleaner obviously
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, nullptr);
        if (res) return;
        if (instance_extension_count == 0) return;

        layer_props.extensionProps.resize(instance_extension_count);
        instance_extensions = layer_props.extensionProps.data();
        res = vkEnumerateInstanceExtensionProperties(layer_name, &instance_extension_count, instance_extensions);
    } while (res == VK_INCOMPLETE);
}

void MyShell::enumerate_device_layer_extension_properties(PhysicalDeviceProperties &props, LayerProperties &layer_props) {
    uint32_t extensionCount;
    std::vector<VkExtensionProperties> extensions;
    char *layer_name = layer_props.properties.layerName;

    vk::assert_success(vkEnumerateDeviceExtensionProperties(props.device, layer_name, &extensionCount, nullptr));

    extensions.resize(extensionCount);
    vk::assert_success(vkEnumerateDeviceExtensionProperties(props.device, layer_name, &extensionCount, extensions.data()));

    if (!extensions.empty())
        for (auto &ext : extensions)
            props.layer_extension_map.insert(std::pair<const char *, VkExtensionProperties>(layer_name, ext));
}

void MyShell::enumerate_instance_extension_properties() {
    uint32_t ext_count;
    vk::assert_success(vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr));
    instExtProps_.resize(ext_count);
    vk::assert_success(vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, instExtProps_.data()));
}

void MyShell::enumerate_surface_properties() {
    // capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx_.physical_dev, ctx_.surface, &ctx_.surface_props.capabilities);

    // surface formats
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(ctx_.physical_dev, ctx_.surface, &formatCount, NULL);
    if (formatCount != 0) {
        ctx_.surface_props.surf_formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(ctx_.physical_dev, ctx_.surface, &formatCount, ctx_.surface_props.surf_formats.data());
    }

    // present modes
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(ctx_.physical_dev, ctx_.surface, &presentModeCount, NULL);
    if (presentModeCount != 0) {
        ctx_.surface_props.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(ctx_.physical_dev, ctx_.surface, &presentModeCount,
                                                  ctx_.surface_props.presentModes.data());
    }
}

void MyShell::enumerate_physical_devs(uint32_t physical_dev_count) {
    std::vector<VkPhysicalDevice> devs;
    uint32_t const req_count = physical_dev_count;

    VkResult res = vkEnumeratePhysicalDevices(ctx_.instance, &physical_dev_count, NULL);
    assert(physical_dev_count >= req_count);
    devs.resize(physical_dev_count);

    vk::assert_success(vkEnumeratePhysicalDevices(ctx_.instance, &physical_dev_count, devs.data()));

    for (auto &dev : devs) {
        PhysicalDeviceProperties props;
        props.device = std::move(dev);

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
        vkEnumerateDeviceExtensionProperties(dev, NULL, &extensionCount, props.extensions.data());

        // layer extensions
        for (auto &layer_prop : layerProps_) {
            enumerate_device_layer_extension_properties(props, layer_prop);
        }

        // features
        vkGetPhysicalDeviceFeatures(props.device, &props.features);

        ctx_.physical_dev_props.push_back(props);
    }

    devs.erase(devs.begin(), devs.end());
}

void MyShell::pick_physical_dev() {
    // Iterate over each enumerated physical device
    for (size_t i = 0; i < ctx_.physical_dev_props.size(); i++) {
        auto &props = ctx_.physical_dev_props[i];
        ctx_.physical_dev_index = ctx_.graphics_index = ctx_.present_index = ctx_.transfer_index = UINT32_MAX;

        if (is_dev_suitable(props, ctx_.graphics_index, ctx_.present_index, ctx_.transfer_index)) {
            // We found a suitable physical device so set relevant data.
            ctx_.physical_dev_index = i;
            ctx_.physical_dev = props.device;
            break;
        }
    }
}

bool MyShell::is_dev_suitable(const PhysicalDeviceProperties &props, uint32_t &graphics_queue_index, uint32_t &present_queue_index,
                              uint32_t &transfer_queue_index) {
    if (!determine_queue_families_support(props, graphics_queue_index, present_queue_index, transfer_queue_index)) return false;
    if (!determine_device_extension_support(props)) return false;
    // optional (warn but dont throw)
    determine_device_feature_support(props);
    return true;
}

bool MyShell::determine_queue_families_support(const PhysicalDeviceProperties &props, uint32_t &graphics_queue_family_index,
                                               uint32_t &present_queue_family_index, uint32_t &transfer_queue_family_index) {
    // Determine graphics and present queues ...

    // Iterate over each queue to learn whether it supports presenting:
    VkBool32 *pSupportsPresent = (VkBool32 *)malloc(props.queue_family_count * sizeof(VkBool32));
    // TODO: I don't feel like re-arranging the whole start up atm.
    auto surface = create_surface(ctx_.instance);
    for (uint32_t i = 0; i < props.queue_family_count; i++) {
        vk::assert_success(vkGetPhysicalDeviceSurfaceSupportKHR(props.device, i, surface, &pSupportsPresent[i]));

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
    vkDestroySurfaceKHR(ctx_.instance, surface, nullptr);
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

bool MyShell::determine_device_extension_support(const PhysicalDeviceProperties &props) {
    // TODO: This won't work right if either list is not unique, but I don't feel like chaing it atm.
    size_t count = 0;
    size_t req_ext_count = device_extensions_.size();
    for (const auto &extension : props.extensions) {
        for (const auto &req_ext_name : device_extensions_) {
            if (std::strcmp(req_ext_name, extension.extensionName) == 0) count++;
            if (count == req_ext_count) return true;
        }
    }
    return false;
}

void MyShell::determine_device_feature_support(const MyShell::PhysicalDeviceProperties &props) {
    // sampler anisotropy
    ctx_.sampler_anisotropy_enabled_ = props.features.samplerAnisotropy && settings_.try_sampler_anisotropy_;
    if (settings_.try_sampler_anisotropy_ && !ctx_.sampler_anisotropy_enabled_)
        log(MyShell::LOG_WARN, "cannot enable sampler anisotropy");
    // sample rate shading
    ctx_.sample_rate_shading_enabled_ = props.features.sampleRateShading && settings_.try_sample_rate_shading_;
    if (settings_.try_sample_rate_shading_ && !ctx_.sample_rate_shading_enabled_)
        log(MyShell::LOG_WARN, "cannot enable sample rate shading");
}

bool MyShell::determine_swapchain_extent(uint32_t width_hint, uint32_t height_hint, bool refresh_capabilities) {
    // 0, 0 for hints indicates calling from acquire_back_buffer... TODO: more robust solution???
    if (refresh_capabilities)
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx_.physical_dev, ctx_.surface, &ctx_.surface_props.capabilities);

    auto &caps = ctx_.surface_props.capabilities;

    VkExtent2D extent = caps.currentExtent;

    // use the hints
    if (extent.width == (uint32_t)-1) {
        extent.width = width_hint;
        extent.height = height_hint;
    }
    // clamp width; to protect us from broken hints?
    if (extent.width < caps.minImageExtent.width)
        extent.width = caps.minImageExtent.width;
    else if (extent.width > caps.maxImageExtent.width)
        extent.width = caps.maxImageExtent.width;
    // clamp height
    if (extent.height < caps.minImageExtent.height)
        extent.height = caps.minImageExtent.height;
    else if (extent.height > caps.maxImageExtent.height)
        extent.height = caps.maxImageExtent.height;

    if (ctx_.extent.width == extent.width && ctx_.extent.height == extent.height) {
        return true;
    } else {
        ctx_.extent = extent;
        return false;
    }
}

void MyShell::determine_depth_format() {
    /* allow custom depth formats */
#ifdef __ANDROID__
    // Depth format needs to be VK_FORMAT_D24_UNORM_S8_UINT on Android.
    ctx_.depth_format = VK_FORMAT_D24_UNORM_S8_UINT;
#elif defined(VK_USE_PLATFORM_IOS_MVK)
    if (ctx_.depth_format == VK_FORMAT_UNDEFINED) ctx_.depth_format = VK_FORMAT_D32_SFLOAT;
#else
    if (ctx_.depth_format == VK_FORMAT_UNDEFINED) ctx_.depth_format = helpers::find_depth_format(ctx_.physical_dev);
#endif
}

void MyShell::determine_swapchain_surface_format() {
    // no preferred type
    if (ctx_.surface_props.surf_formats.size() == 1 && ctx_.surface_props.surf_formats[0].format == VK_FORMAT_UNDEFINED) {
        ctx_.format = {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
        return;
    } else {
        for (const auto &surfFormat : ctx_.surface_props.surf_formats) {
            if (surfFormat.format == VK_FORMAT_B8G8R8A8_UNORM && surfFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                ctx_.format = surfFormat;
                return;
            }
        }
    }
    log(MyShell::LOG_INFO, "choosing first available swap surface format!");
    ctx_.format = ctx_.surface_props.surf_formats[0];
}

void MyShell::determine_swapchain_present_mode() {
    // guaranteed to be present (vert sync)
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto &mode : ctx_.surface_props.presentModes) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {  // triple buffer
            presentMode = mode;
        } else if (mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {  // no sync
            presentMode = mode;
        }
    }
    ctx_.mode = presentMode;
}

void MyShell::determine_swapchain_image_count() {
    auto &caps = ctx_.surface_props.capabilities;
    // Determine the number of VkImage's to use in the swap chain.
    // We need to acquire only 1 presentable image at at time.
    // Asking for minImageCount images ensures that we can acquire
    // 1 presentable image as long as we present it before attempting
    // to acquire another.
    ctx_.image_count = caps.minImageCount + 1;  // TODO: this "+ 1" is extremely arbitrary
    if (caps.maxImageCount > 0 && ctx_.image_count > caps.maxImageCount) {
        ctx_.image_count = caps.maxImageCount;
    }
}

void MyShell::determine_api_version(uint32_t &version) {
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
            app_info.pNext = nullptr;
            app_info.pApplicationName = "";
            app_info.applicationVersion = 1;
            app_info.pEngineName = "";
            app_info.engineVersion = 1;
            app_info.apiVersion = desired_version;

            // Initialize the VkInstanceCreateInfo structure
            VkInstanceCreateInfo inst_info = {};
            inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            inst_info.pNext = nullptr;
            inst_info.flags = 0;
            inst_info.pApplicationInfo = &app_info;
            inst_info.enabledExtensionCount = 0;
            inst_info.ppEnabledExtensionNames = nullptr;
            inst_info.enabledLayerCount = 0;
            inst_info.ppEnabledLayerNames = nullptr;

            // Attempt to create the instance
            if (VK_SUCCESS != vkCreateInstance(&inst_info, nullptr, &instance)) {
                std::cout << "Unknown error creating " << desired_version_string << " Instance\n";
                exit(-1);
            }

            // Get the list of physical devices
            uint32_t phys_dev_count = 1;
            if (VK_SUCCESS != vkEnumeratePhysicalDevices(instance, &phys_dev_count, nullptr) || phys_dev_count == 0) {
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

            // Go through the list of physical devices and select only those that are capable of running the API version we
            // want.
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
        vkDestroyInstance(instance, nullptr);
    }
#endif
}
