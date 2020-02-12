/*
 * Modifications copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 * -------------------------------
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

#include "Shell.h"

#include <cassert>
#include <array>
#include <iostream>
#include <stdio.h>
#include <string>
#include <sstream>
#include <set>

#include "EventHandlers.h"
#include "Game.h"
#include "Helpers.h"
#include "InputHandler.h"
// HANDLERS
#include "InputHandler.h"
#include "SoundHandler.h"

// See: https://github.com/KhronosGroup/Vulkan-Hpp#extensions--per-device-function-pointers
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;
static_assert(VK_HEADER_VERSION >= 126, "Vulkan version need 1.1.126.0 or greater");

Shell::Shell(Game &game, Handlers &&handlers)
    : limitFramerate(true),
      framesPerSecondLimit(10),
      game_(game),
      settings_(game.settings()),
      instanceExtensions_{
          VK_KHR_SURFACE_EXTENSION_NAME,  // require generic WSI extensions
      },
      /** Device extensions:
       *    If an extension is required to run flip that flag and the program will not start without it. If you want the
       * extension to be optional set the required flag to false. If you do not want the extension to try to be enabled set
       * the last flag and the extension will not be enabled even if its possible.
       */
      deviceExtensionInfo_{
          {VK_KHR_SWAPCHAIN_EXTENSION_NAME, true, true},
          {VK_EXT_DEBUG_MARKER_EXTENSION_NAME, false, settings_.tryDebugMarkers},
          {VK_EXT_DEBUG_MARKER_EXTENSION_NAME, false, settings_.tryDebugMarkers},
          {VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME, false, false},
          {VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME, false, false},
      },
      currentTime_(0.0),
      elapsedTime_(0.0),
      handlers_(std::move(handlers)),
      ctx_(),
      gameTick_(1.0f / settings_.ticksPerSecond),
      gameTime_(gameTick_),
      debugUtilsMessenger_() {
    /**
     * Leaving this validation layer code in but I am going to start using vkconfig.exe instead. It does everything
     * the layer code here already did but better. You can read about it here
     * https://vulkan.lunarg.com/doc/sdk/1.1.126.0/windows/vkconfig.html in case you forget how to use it. I feel like if
     * LunarG made the app then they are probably willing to maintain it which is easier than figuring out how the layers
     * will change over time. Its also configurable through the UI so it really is a nice little thing to use.
     *
     * Things to note:
     *
     * - They aparently removed the verbose oject tracking when they moved towards VK_LAYER_KHRONOS_validation from
     * VK_LAYER_LUNARG_standard_validation which is a huge bummer.
     *
     * - If I set the info flag for VK_LAYER_KHRONOS_validation I was not recieving the same messages as enabling it
     * programmatically. Fortunately, the message were only for device extension enabling which is not super useful, or has
     * not been thus far. It does make we wonder if there are other messages I am potentially missing though.
     */
    if (settings_.validateVerbose) assert(settings_.validate);
    if (settings_.validate) {
        instanceLayers_.push_back("VK_LAYER_KHRONOS_validation");
        instanceExtensions_.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
}

Shell::~Shell() = default;

void Shell::log(LogPriority priority, const char *msg) const {
    std::ostream &st = (priority >= LogPriority::LOG_ERR) ? std::cerr : std::cout;
    st << msg << std::endl;
}

void Shell::init() {
    initInstance();
    initDebugUtilsMessenger();
    initPhysicalDev();

    // TODO: Add init functions here... inline ???
    handlers_.pSound->init();
}

void Shell::update(const double elapsed) {
    // TODO: Add update functions here... inline ???
    handlers_.pSound->update(elapsed);
    handlers_.pInput->updateInput(static_cast<float>(elapsed));  // remove me !!!!!
    handlers_.pInput->update(elapsed);
}

void Shell::destroy() {
    // TODO: Add destroy functions here... inline ???
    handlers_.pSound->destroy();
}

void Shell::cleanup() {
    if (settings_.validate) ctx_.instance.destroyDebugUtilsMessengerEXT(debugUtilsMessenger_, ALLOC_PLACE_HOLDER);
    destroyInstance();
}

void Shell::assertAllInstanceLayers() const {
    // Check if all instance layers required are present
    for (const auto &layerName : instanceLayers_) {
        auto it = std::find_if(layerProps_.begin(), layerProps_.end(), [&layerName](auto layer_prop) {
            return std::strcmp(layer_prop.properties.layerName, layerName) == 0;
        });
        if (it == layerProps_.end()) {
            std::stringstream ss;
            ss << "instance layer " << layerName << " is missing";
            log(LogPriority::LOG_WARN, ss.str().c_str());
            assert(false);
        }
    }
}

void Shell::assertAllInstanceExtensions() const {
    // Check if all instance extensions required are present
    for (const auto &extensionName : instanceExtensions_) {
        auto it = std::find_if(instExtProps_.begin(), instExtProps_.end(),
                               [&extensionName](auto ext) { return std::strcmp(extensionName, ext.extensionName) == 0; });
        if (it == instExtProps_.end()) {
            std::stringstream ss;
            ss << "instance extension " << extensionName << " is missing";
            throw std::runtime_error(ss.str());
        }
    }
}

bool Shell::hasAllDeviceLayers(vk::PhysicalDevice phy) const {
    // Something here?

    return true;
}

bool Shell::hasAllDeviceExtensions(vk::PhysicalDevice phy) const {
    //// enumerate device extensions
    // std::vector<vk::ExtensionProperties> exts;
    // vk::enumerate(phy, nullptr, exts);

    // std::set<std::string> ext_names;
    // for (const auto &ext : exts) ext_names.insert(ext.extensionName);

    //// all listed device extensions are required
    // for (const auto &name : device_extensions_) {
    //    if (ext_names.find(name) == ext_names.end()) return false;
    //}

    return true;
}

void Shell::initInstance() {
    vk::DynamicLoader dl;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
    VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

    // enumerate instance layer & extension properties
    enumerateInstanceProperties();

    assertAllInstanceLayers();
    assertAllInstanceExtensions();

    vk::ApplicationInfo appInfo = {};
    appInfo.pApplicationName = settings_.name.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName = "Shell";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.apiVersion = vk::enumerateInstanceVersion();

    vk::InstanceCreateInfo instInfo = {};
    instInfo.pApplicationInfo = &appInfo;
    instInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers_.size());
    instInfo.ppEnabledLayerNames = instanceLayers_.data();
    instInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions_.size());
    instInfo.ppEnabledExtensionNames = instanceExtensions_.data();

    ctx_.instance = vk::createInstance(instInfo, ALLOC_PLACE_HOLDER);
    assert(ctx_.instance);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(ctx_.instance);

    // Moved asserts below from old Extensions.h. Not sure yet if there is a better place.

    if (settings_.validate) {
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkCreateDebugUtilsMessengerEXT);
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkDestroyDebugUtilsMessengerEXT);
    }
}

void Shell::initDebugUtilsMessenger() {
    if (!settings_.validate) return;

    vk::DebugUtilsMessengerCreateInfoEXT debugInfo = {};

    debugInfo.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;
    debugInfo.messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation;

    if (settings_.validateVerbose) {
        debugInfo.messageSeverity |=
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo;
        debugInfo.messageType |=
            vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;
    }

    debugInfo.pfnUserCallback = events::DebugUtilsMessenger;
    debugInfo.pUserData = this;

    debugUtilsMessenger_ = ctx_.instance.createDebugUtilsMessengerEXT(debugInfo, ALLOC_PLACE_HOLDER);
}

void Shell::initPhysicalDev() {
    enumeratePhysicalDevs();
    pickPhysicalDev();
    if (!ctx_.physicalDev) {
        printf("failed to find any capable Vulkan physical device");
        abort();
    }
}

void Shell::createContext() {
    createDev();

    initDevQueues();

    // TODO: should this do this?
    // initialize ctx_.{surface,format} before attach_shell
    createSwapchain();

    // need image count
    createBackBuffers();

    game_.attachShell(*this);
}

void Shell::destroyContext() {
    if (!ctx_.dev) return;

    ctx_.dev.waitIdle();

    destroySwapchain();

    game_.detachShell();

    destroyBackBuffers();

    ctx_.dev.waitIdle();
    ctx_.dev.destroy(ALLOC_PLACE_HOLDER);
}

void Shell::createDev() {
    // queue create info
    std::set<uint32_t> unique_queue_families = {
        ctx_.graphicsIndex,
        ctx_.presentIndex,
        ctx_.transferIndex,
        ctx_.computeIndex,
    };
    float queue_priorities[1] = {0.0f};  // only one queue per family atm
    std::vector<vk::DeviceQueueCreateInfo> queue_infos;
    for (auto &index : unique_queue_families) {
        vk::DeviceQueueCreateInfo queueInfo = {};
        queueInfo.queueCount = 1;  // only one queue per family atm
        queueInfo.queueFamilyIndex = index;
        queueInfo.pQueuePriorities = queue_priorities;
        queue_infos.push_back(queueInfo);
    }

    // features
    vk::PhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = ctx_.samplerAnisotropyEnabled;
    deviceFeatures.sampleRateShading = ctx_.sampleRateShadingEnabled;
    deviceFeatures.fragmentStoresAndAtomics = ctx_.computeShadingEnabled;
    deviceFeatures.tessellationShader = ctx_.tessellationShadingEnabled;
    deviceFeatures.geometryShader = ctx_.geometryShadingEnabled;
    deviceFeatures.fillModeNonSolid = ctx_.wireframeShadingEnabled;
    deviceFeatures.independentBlend = ctx_.independentBlendEnabled;
    deviceFeatures.imageCubeArray = ctx_.independentBlendEnabled;

    // Phyiscal device extensions names
    auto &phyDevProps = ctx_.physicalDevProps[ctx_.physicalDevIndex];
    std::vector<const char *> enabledExtensionNames;
    for (const auto &extInfo : phyDevProps.phyDevExtInfos)
        if (extInfo.valid) enabledExtensionNames.push_back(extInfo.name);

    vk::DeviceCreateInfo devInfo = {};
    devInfo.queueCreateInfoCount = static_cast<uint32_t>(queue_infos.size());
    devInfo.pQueueCreateInfos = queue_infos.data();
    devInfo.pEnabledFeatures = &deviceFeatures;
    devInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensionNames.size());
    devInfo.ppEnabledExtensionNames = enabledExtensionNames.data();

    // Physical device extension featrues (For some reason using the name above doesn't always work)
    // void **pNext = &(const_cast<void *>(devInfo.pNext));
    if (ctx_.vertexAttributeDivisorEnabled) {
        assert(false);  // Never tested. Was using the name above sufficient?
        // *pNext = &phyDevProps.featVertAttrDiv;
        // pNext = &phyDevProps.featVertAttrDiv.pNext;
    }
    if (ctx_.transformFeedbackEnabled) {
        assert(false);
        // *pNext = &phyDevProps.featTransFback;
        // pNext = &phyDevProps.featTransFback.pNext;
    }

    ctx_.dev = ctx_.physicalDev.createDevice(devInfo, ALLOC_PLACE_HOLDER);
    assert(ctx_.dev);

    VULKAN_HPP_DEFAULT_DISPATCHER.init(ctx_.dev);

    // Moved asserts below from old Extensions.h. Not sure yet if there is a better place.

    if (ctx_.debugMarkersEnabled) {
        // There are more of these and really there needs to be a more robust solution. As of now there is no way to enable
        // these.
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkDebugMarkerSetObjectTagEXT);
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkDebugMarkerSetObjectNameEXT);
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdDebugMarkerBeginEXT);
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdDebugMarkerEndEXT);
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdDebugMarkerInsertEXT);

        ctx_.dbg.init(ctx_.dev, true);
    }
    if (ctx_.transformFeedbackEnabled) {
        assert(false);  // You probably don't want to use transform feedback see below.
        /**
         * I started trying to implement a simple a vertex update on the gpu using transform feedback while following along
         * with the shader book I'm reading. As I was doing this it was becoming increasingly clear that this was going to be
         * difficult due to the lack of working examples I could find online. That is when I stumbled upon this blog post by
         * a Khronos employee: http://jason-blog.jlekstrand.net/2018/10/transform-feedback-is-terrible-so-why.html. After
         * reading this I will take his adivce and implement the update on the gpu using compute shaders, but I am going to
         * leave this code here because I liked the changes I made to the extensions code in this file, and the
         * Shell/Settings code elsewhere. Also, it should be a good reminder to not just following things blindly.
         */
        assert(VULKAN_HPP_DEFAULT_DISPATCHER.vkCmdBindTransformFeedbackBuffersEXT);
    }
}

void Shell::createBackBuffers() {
    vk::SemaphoreCreateInfo semaphoreInfo = {};
    vk::FenceCreateInfo fenceInfo = {vk::FenceCreateFlagBits::eSignaled};

    // BackBuffer is used to track which swapchain image and its associated
    // sync primitives are busy.  Having more BackBuffer's than swapchain
    // images may allow us to replace CPU wait on present_fence by GPU wait
    // on acquire_semaphore.
    const int count = ctx_.imageCount + 1;
    for (int i = 0; i < count; i++) {
        BackBuffer backBuffer = {};
        backBuffer.acquireSemaphore = ctx_.dev.createSemaphore(semaphoreInfo, ALLOC_PLACE_HOLDER);
        backBuffer.renderSemaphore = ctx_.dev.createSemaphore(semaphoreInfo, ALLOC_PLACE_HOLDER);
        backBuffer.presentFence = ctx_.dev.createFence(fenceInfo, ALLOC_PLACE_HOLDER);

        ctx_.backBuffers.push(backBuffer);
    }
}

void Shell::destroyBackBuffers() {
    while (!ctx_.backBuffers.empty()) {
        const auto &backBuffer = ctx_.backBuffers.front();

        ctx_.dev.destroySemaphore(backBuffer.acquireSemaphore, ALLOC_PLACE_HOLDER);
        ctx_.dev.destroySemaphore(backBuffer.renderSemaphore, ALLOC_PLACE_HOLDER);
        ctx_.dev.destroyFence(backBuffer.presentFence, ALLOC_PLACE_HOLDER);

        ctx_.backBuffers.pop();
    }
}

// TODO: this doesn't create the swapchain
void Shell::createSwapchain() {
    ctx_.surface = createSurface(ctx_.instance);

    vk::Bool32 supported = ctx_.physicalDev.getSurfaceSupportKHR(ctx_.presentIndex, ctx_.surface);
    assert(supported);
    // this should be guaranteed by the platform-specific can_present call
    supported = canPresent(ctx_.physicalDev, ctx_.presentIndex);
    assert(supported);

    enumerateSurfaceProperties();
    determineSwapchainSurfaceFormat();
    determineSwapchainPresentMode();
    determineSwapchainImageCount();
    determineDepthFormat();

    // suface dependent flags
    /*  TODO: Clean up what happens if the GPU doesn't handle linear blitting.
        Right now this flag just turns it off.

        There are two alternatives in this case. You could implement a function that searches common texture image
        formats for one that does support linear blitting, or you could implement the mipmap generation in software
        with a library like stb_image_resize. Each mip level can then be loaded into the image in the same way that
        you loaded the original image.
    */
    ctx_.linearBlittingSupported =
        helpers::findSupportedFormat(ctx_.physicalDev, {ctx_.surfaceFormat.format}, vk::ImageTiling::eOptimal,
                                     vk::FormatFeatureFlagBits::eSampledImageFilterLinear) != vk::Format::eUndefined;

    // defer to resizeSwapchain()
    ctx_.swapchain = vk::SwapchainKHR{};
    // ctx_.extent.width = UINT32_MAX;
    // ctx_.extent.height = UINT32_MAX;
}

void Shell::destroySwapchain() {
    if (ctx_.swapchain) {
        game_.detachSwapchain();
        ctx_.dev.destroySwapchainKHR(ctx_.swapchain, ALLOC_PLACE_HOLDER);
    }
    ctx_.instance.destroySurfaceKHR(ctx_.surface, ALLOC_PLACE_HOLDER);
}

void Shell::onButton(const GameButtonBits buttons) { game_.onButton(buttons); }

void Shell::onKey(const GAME_KEY key) {
    switch (key) {
        case GAME_KEY::KEY_MINUS: {
            if (framesPerSecondLimit > 1) framesPerSecondLimit--;
        } break;
        case GAME_KEY::KEY_EQUALS: {
            if (framesPerSecondLimit < UINT8_MAX) framesPerSecondLimit++;
        } break;
        case GAME_KEY::KEY_BACKSPACE: {
            limitFramerate = !limitFramerate;
        } break;
        default:;
    }
    game_.onKey(key);
}

void Shell::resizeSwapchain(uint32_t widthHint, uint32_t heightHint, bool refreshCapabilities) {
    if (determineSwapchainExtent(widthHint, heightHint, refreshCapabilities)) return;

    auto &caps = ctx_.surfaceProps.capabilities;

    vk::SurfaceTransformFlagBitsKHR preTransform;  // ex: rotate 90
    if (caps.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) {
        preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
    } else {
        preTransform = caps.currentTransform;
    }

    // Find a supported composite alpha mode - one of these is guaranteed to be set
    vk::CompositeAlphaFlagBitsKHR compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
    vk::CompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
        vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
        vk::CompositeAlphaFlagBitsKHR::eInherit,
    };
    for (uint32_t i = 0; i < sizeof(compositeAlphaFlags); i++) {
        if (caps.supportedCompositeAlpha & compositeAlphaFlags[i]) {
            compositeAlpha = compositeAlphaFlags[i];
            break;
        }
    }

    vk::SwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.surface = ctx_.surface;
    swapchainInfo.minImageCount = ctx_.imageCount;
    swapchainInfo.imageFormat = ctx_.surfaceFormat.format;
    swapchainInfo.imageColorSpace = ctx_.surfaceFormat.colorSpace;
    swapchainInfo.imageExtent = ctx_.extent;
    swapchainInfo.imageArrayLayers = 1;
    // TODO: Should these flags be pass setup dependent???
    swapchainInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;  // | vk::ImageUsageFlagBits::eStorage;
    swapchainInfo.preTransform = caps.currentTransform;
    swapchainInfo.compositeAlpha = compositeAlpha;
    swapchainInfo.presentMode = ctx_.mode;
#ifndef __ANDROID__
    // Clip obscured pixels (by other windows for example). Only needed if the pixel info is reused.
    swapchainInfo.clipped = true;
#else
    swapchain_info.clipped = false;
#endif
    swapchainInfo.imageSharingMode = vk::SharingMode::eExclusive;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.pQueueFamilyIndices = nullptr;

    if (ctx_.graphicsIndex != ctx_.presentIndex) {
        // If the graphics and present queues are from different queue families,
        // we either have to explicitly transfer ownership of images between the
        // queues, or we have to create the swapchain with imageSharingMode
        // as vk::SharingMode::eConcurrent
        uint32_t queue_families[2] = {ctx_.graphicsIndex, ctx_.presentIndex};
        swapchainInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        swapchainInfo.queueFamilyIndexCount = 2;
        swapchainInfo.pQueueFamilyIndices = queue_families;
    }

    swapchainInfo.oldSwapchain = ctx_.swapchain;

    ctx_.swapchain = ctx_.dev.createSwapchainKHR(swapchainInfo, ALLOC_PLACE_HOLDER);

    // destroy the old swapchain
    if (swapchainInfo.oldSwapchain) {
        ctx_.dev.waitIdle();

        game_.detachSwapchain();
        ctx_.dev.destroySwapchainKHR(swapchainInfo.oldSwapchain, ALLOC_PLACE_HOLDER);
    }

    game_.attachSwapchain();
}

void Shell::addGameTime(float time) {
    int max_ticks = 1;

    if (!settings_.noTick) gameTime_ += time;

    while (gameTime_ >= gameTick_ && max_ticks--) {
        game_.onTick();
        gameTime_ -= gameTick_;
    }
}

void Shell::acquireBackBuffer() {
    auto &backBuffer = ctx_.backBuffers.front();

    // wait until acquire and render semaphores are waited/unsignaled
    ctx_.dev.waitForFences({backBuffer.presentFence}, true, UINT64_MAX);

    vk::Result res = vk::Result::eTimeout;  // Anything but vk::Result::eSuccess
    while (res != vk::Result::eSuccess) {
        res = ctx_.dev.acquireNextImageKHR(ctx_.swapchain, UINT64_MAX, backBuffer.acquireSemaphore, {},
                                           &backBuffer.imageIndex);

        if (res == vk::Result::eErrorOutOfDateKHR) {
            // Swapchain is out of date (e.g. the window was resized) and
            // must be recreated:
            resizeSwapchain(0, 0);  // width and height hints should be ignored
        } else {
            helpers::checkVkResult(res);
        }
    }

    // reset the fence (AFTER POTENTIAL RECREATION OF SWAP CHAIN)
    ctx_.dev.resetFences({backBuffer.presentFence});

    ctx_.acquiredBackBuffer = backBuffer;
    ctx_.backBuffers.pop();
}

void Shell::presentBackBuffer() {
    const auto &backBuffer = ctx_.acquiredBackBuffer;

    if (!settings_.noRender) game_.onFrame(gameTime_ / gameTick_);

    vk::PresentInfoKHR presentInfo = {};
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = (settings_.noRender) ? &backBuffer.acquireSemaphore : &backBuffer.renderSemaphore;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &ctx_.swapchain;
    presentInfo.pImageIndices = &backBuffer.imageIndex;

    auto res = ctx_.queues[ctx_.presentIndex].presentKHR(presentInfo);
    if (res == vk::Result::eErrorOutOfDateKHR) {
        // Swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        resizeSwapchain(0, 0);  // width and height hints should be ignored
    } else {
        helpers::checkVkResult(res);
    }

    ctx_.queues[ctx_.presentIndex].submit({}, backBuffer.presentFence);
    ctx_.backBuffers.push(backBuffer);
}

// ********
// NEW FUNCTIONS FROM MY OTHER INIT CODE
// ********

void Shell::initDevQueues() {
    std::set<uint32_t> unique_queue_families = {
        ctx_.graphicsIndex,
        ctx_.presentIndex,
        ctx_.transferIndex,
        ctx_.computeIndex,
    };
    for (auto familtyIndex : unique_queue_families) {
        vk::Queue queue = ctx_.dev.getQueue(familtyIndex, 0);
        assert(queue);
        ctx_.queues[familtyIndex] = queue;
    }
}

void Shell::destroyInstance() { ctx_.instance.destroy(ALLOC_PLACE_HOLDER); }

void Shell::enumerateInstanceProperties() {
#ifdef __ANDROID__
    // This place is the first place for samples to use Vulkan APIs.
    // Here, we are going to open Vulkan.so on the device and retrieve function pointers using
    // vulkan_wrapper helper.
    if (!InitVulkan()) {
        LOGE("Failied initializing Vulkan APIs!");
        return vk::Result::eErrorInitializationFailed;
    }
    LOGI("Loaded Vulkan APIs.");
#endif

    /*
     * It's possible, though very rare, that the number of
     * instance layers could change. For example, installing something
     * could include new layers that the loader would pick up
     * between the initial query for the count and the
     * request for vk::LayerProperties. The loader indicates that
     * by returning a vk::Result::eIncomplete status and will update the
     * the count parameter.
     * The count parameter will be updated with the number of
     * entries loaded into the data pointer - in case the number
     * of layers went down or is smaller than the size given.
     */
    auto props = vk::enumerateInstanceLayerProperties();

    /*
     * Now gather the extension list for each instance layer.
     */
    for (auto &prop : props) {
        layerProps_.push_back({prop});
        layerProps_.back().extensionProps =
            vk::enumerateInstanceExtensionProperties(std::string(layerProps_.back().properties.layerName));
    }

    // Get instance extension properties
    instExtProps_ = vk::enumerateInstanceExtensionProperties();
}

void Shell::enumerateDeviceLayerExtensionProperties(PhysicalDeviceProperties &props, LayerProperties &layerProps) {
    auto extensions = props.device.enumerateDeviceExtensionProperties(std::string(layerProps.properties.layerName));
    if (!extensions.empty())
        for (auto &ext : extensions)
            props.layerExtensionMap.insert(
                std::pair<const char *, vk::ExtensionProperties>(layerProps.properties.layerName, ext));
}

void Shell::enumerateSurfaceProperties() {
    ctx_.surfaceProps.capabilities = ctx_.physicalDev.getSurfaceCapabilitiesKHR(ctx_.surface);
    ctx_.surfaceProps.formats = ctx_.physicalDev.getSurfaceFormatsKHR(ctx_.surface);
    ctx_.surfaceProps.presentModes = ctx_.physicalDev.getSurfacePresentModesKHR(ctx_.surface);
}

void Shell::enumeratePhysicalDevs(uint32_t physicalDevCount) {
    auto devs = ctx_.instance.enumeratePhysicalDevices();
    assert(devs.size() >= physicalDevCount);

    for (auto &dev : devs) {
        ctx_.physicalDevProps.push_back({});
        auto &props = ctx_.physicalDevProps.back();
        props.device = dev;

        // Queue family properties
        props.queueProps = props.device.getQueueFamilyProperties();
        assert(props.queueProps.size());
        // Memory properties
        props.memoryProperties = props.device.getMemoryProperties();
        // Extension properties
        props.extensionProperties = props.device.enumerateDeviceExtensionProperties();

        // This could all be faster, but I doubt it will make a significant difference at any point.
        for (const auto &extInfo : deviceExtensionInfo_) {
            auto it =
                std::find_if(props.extensionProperties.begin(), props.extensionProperties.end(),
                             [&extInfo](const auto &extProp) { return strcmp(extInfo.name, extProp.extensionName) == 0; });

            if (it == props.extensionProperties.end()) {
                // TODO: better logging
                if (extInfo.required) {
                    assert(false && "Couldn't find required extension");
                    exit(EXIT_FAILURE);
                }
                props.phyDevExtInfos.push_back(extInfo);
                props.phyDevExtInfos.back().valid = false;
            } else {
                props.phyDevExtInfos.push_back(extInfo);

                if (strcmp(extInfo.name, (char *)VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
                    props.phyDevExtInfos.back().valid = true;
                    continue;

                } else if (strcmp(extInfo.name, (char *)VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0) {
                    if (extInfo.tryToEnabled) {
                        assert(false);  // TODO: what should this be ?? There are a bunch of extensions functions.
                    }

                } else if (strcmp(extInfo.name, (char *)VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME) == 0) {
                    if (extInfo.tryToEnabled) {
                        assert(false);  // Not tested.
                        vk::PhysicalDeviceFeatures2 features = props.device.getFeatures2();
                        // Check features
                        if (props.featVertAttrDiv.vertexAttributeInstanceRateDivisor) {
                            props.featVertAttrDiv.vertexAttributeInstanceRateZeroDivisor = VK_FALSE;
                            props.phyDevExtInfos.back().valid = true;
                            continue;
                        }
                    }

                } else if (strcmp(extInfo.name, (char *)VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME) == 0) {
                    if (extInfo.tryToEnabled) {
                        vk::PhysicalDeviceFeatures2 features = props.device.getFeatures2();
                        // Check features
                        if (props.featTransFback.transformFeedback) {
                            props.featTransFback.geometryStreams = VK_FALSE;
                            props.phyDevExtInfos.back().valid = true;
                            continue;
                        }
                    }

                } else {
                    assert(false && "Unhandled physical device extension");
                    exit(EXIT_FAILURE);
                }

                props.phyDevExtInfos.back().valid = false;
            }
        }

        // Physical device features
        props.features = props.device.getFeatures();

        // Physical device properties
        props.properties = props.device.getProperties();

        // Layer extension properties
        for (auto &layerProp : layerProps_) {
            enumerateDeviceLayerExtensionProperties(props, layerProp);
        }
    }

    devs.erase(devs.begin(), devs.end());
}

void Shell::pickPhysicalDev() {
    // Iterate over each enumerated physical device
    for (size_t i = 0; i < ctx_.physicalDevProps.size(); i++) {
        auto &props = ctx_.physicalDevProps[i];
        ctx_.physicalDevIndex = ctx_.graphicsIndex = ctx_.presentIndex = ctx_.transferIndex = ctx_.computeIndex = UINT32_MAX;

        if (isDevSuitable(props, ctx_.graphicsIndex, ctx_.presentIndex, ctx_.transferIndex, ctx_.computeIndex)) {
            // We found a suitable physical device so set relevant data.
            ctx_.physicalDevIndex = i;
            ctx_.physicalDev = props.device;

            // Convenience variables
            ctx_.memProps = ctx_.physicalDevProps[ctx_.physicalDevIndex].memoryProperties;
            deviceExtensionInfo_ = props.phyDevExtInfos;

            // Flags
            for (const auto &extInfo : deviceExtensionInfo_) {
                if (strcmp(extInfo.name, (char *)VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0)
                    ctx_.debugMarkersEnabled = extInfo.valid;
                if (strcmp(extInfo.name, (char *)VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME) == 0)
                    ctx_.vertexAttributeDivisorEnabled = extInfo.valid;
                if (strcmp(extInfo.name, (char *)VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME) == 0)
                    ctx_.transformFeedbackEnabled = extInfo.valid;
            }

            break;
        }
    }
}

bool Shell::isDevSuitable(const PhysicalDeviceProperties &props, uint32_t &graphicsIndex, uint32_t &presentIndex,
                          uint32_t &transferIndex, uint32_t &computeIndex) {
    if (!determineQueueFamiliesSupport(props, graphicsIndex, presentIndex, transferIndex, computeIndex)) return false;
    if (!determineDeviceExtensionSupport(props)) return false;
    // optional (warn but dont throw)
    determineDeviceFeatureSupport(props);
    // depends on above...
    determineSampleCount(props);
    return true;
}

bool Shell::determineQueueFamiliesSupport(const PhysicalDeviceProperties &props, uint32_t &graphicsIndex,
                                          uint32_t &presentIndex, uint32_t &transferIndex, uint32_t &computeIndex) {
    // Determine graphics and present queues ...

    // Iterate over each queue to learn whether it supports presenting:
    std::vector<vk::Bool32> pSupportsPresent(props.queueProps.size());
    // TODO: I don't feel like re-arranging the whole start up atm.
    auto surface = createSurface(ctx_.instance);
    for (uint32_t i = 0; i < static_cast<uint32_t>(props.queueProps.size()); i++) {
        pSupportsPresent[i] = props.device.getSurfaceSupportKHR(i, surface);

        // Search for a graphics and a present queue in the array of queue
        // families, try to find one that supports both
        if (props.queueProps[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            if (graphicsIndex == UINT32_MAX) graphicsIndex = i;

            if (pSupportsPresent[i] == VK_TRUE) {
                graphicsIndex = i;
                presentIndex = i;
                break;
            }
        }
    }
    if (presentIndex == UINT32_MAX) {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        // TODO: is this the best thing to do?
        for (uint32_t i = 0; i < static_cast<uint32_t>(props.queueProps.size()); ++i) {
            if (pSupportsPresent[i] == VK_TRUE) {
                presentIndex = i;
                break;
            }
        }
    }
    ctx_.instance.destroySurfaceKHR(surface, ALLOC_PLACE_HOLDER);

    // Determine transfer queue ...

    for (uint32_t i = 0; i < static_cast<uint32_t>(props.queueProps.size()); ++i) {
        if ((props.queueProps[i].queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits{} &&
            props.queueProps[i].queueFlags & vk::QueueFlagBits::eTransfer) {
            transferIndex = i;
            break;
        } else if (props.queueProps[i].queueFlags & vk::QueueFlagBits::eTransfer) {
            transferIndex = i;
        }
    }

    // Determine compute queue ... (This logic ain't great. It can put compute with transfer)

    for (uint32_t i = 0; i < static_cast<uint32_t>(props.queueProps.size()); ++i) {
        if ((props.queueProps[i].queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits{} &&
            props.queueProps[i].queueFlags & vk::QueueFlagBits::eCompute) {
            computeIndex = i;
            break;
        } else if (props.queueProps[i].queueFlags & vk::QueueFlagBits::eCompute) {
            computeIndex = i;
        }
    }

    // Do any other selection of device logic here...

    // Just pick the fist one that supports both
    return (graphicsIndex != UINT32_MAX && presentIndex != UINT32_MAX && transferIndex != UINT32_MAX &&
            computeIndex != UINT32_MAX);
}

bool Shell::determineDeviceExtensionSupport(const PhysicalDeviceProperties &props) {
    if (deviceExtensionInfo_.empty()) return true;
    for (const auto &extInfo : props.phyDevExtInfos) {
        if (extInfo.required && !extInfo.valid) return false;
    }
    return true;
}

void Shell::determineDeviceFeatureSupport(const PhysicalDeviceProperties &props) {
    // sampler anisotropy
    ctx_.samplerAnisotropyEnabled = props.features.samplerAnisotropy && settings_.trySamplerAnisotropy;
    if (settings_.trySamplerAnisotropy && !ctx_.samplerAnisotropyEnabled)
        log(LogPriority::LOG_WARN, "cannot enable sampler anisotropy");
    // sample rate shading
    ctx_.sampleRateShadingEnabled = props.features.sampleRateShading && settings_.trySampleRateShading;
    if (settings_.trySampleRateShading && !ctx_.sampleRateShadingEnabled)
        log(LogPriority::LOG_WARN, "cannot enable sample rate shading");
    // compute shading (TODO: this should be more robust)
    ctx_.computeShadingEnabled = props.features.fragmentStoresAndAtomics && settings_.tryComputeShading;
    if (settings_.tryComputeShading && !ctx_.computeShadingEnabled)  //
        log(LogPriority::LOG_WARN, "cannot enable compute shading (actually just can't enable fragment stores and atomics)");
    // tessellation shading
    ctx_.tessellationShadingEnabled = props.features.tessellationShader && settings_.tryTessellationShading;
    if (settings_.tryTessellationShading && !ctx_.tessellationShadingEnabled)  //
        log(LogPriority::LOG_WARN, "cannot enable tessellation shading");
    // geometry shading
    ctx_.geometryShadingEnabled = props.features.geometryShader && settings_.tryGeometryShading;
    if (settings_.tryGeometryShading && !ctx_.geometryShadingEnabled)  //
        log(LogPriority::LOG_WARN, "cannot enable geometry shading");
    // wireframe shading
    ctx_.wireframeShadingEnabled = props.features.fillModeNonSolid && settings_.tryWireframeShading;
    if (settings_.tryWireframeShading && !ctx_.wireframeShadingEnabled)  //
        log(LogPriority::LOG_WARN, "cannot enable wire frame shading (actually just can't enable fill mode non-solid)");
    // independent attachment blending
    ctx_.independentBlendEnabled = props.features.independentBlend && settings_.tryIndependentBlend;
    if (settings_.tryIndependentBlend && !ctx_.independentBlendEnabled)  //
        log(LogPriority::LOG_WARN, "cannot enable independent attachment blending");
    // image cube arrays
    ctx_.imageCubeArrayEnabled = props.features.imageCubeArray && settings_.tryImageCubeArray;
    if (settings_.tryImageCubeArray && !ctx_.imageCubeArrayEnabled)  //
        log(LogPriority::LOG_WARN, "cannot enable image cube arrays");
}

void Shell::determineSampleCount(const PhysicalDeviceProperties &props) {
    /* DEPENDS on determine_device_feature_support */
    ctx_.samples = vk::SampleCountFlagBits::e1;
    if (ctx_.sampleRateShadingEnabled) {
        vk::SampleCountFlags counts = std::min(props.properties.limits.framebufferColorSampleCounts,
                                               props.properties.limits.framebufferDepthSampleCounts);
        // return the highest possible one for now
        if (counts & vk::SampleCountFlagBits::e64)
            ctx_.samples = vk::SampleCountFlagBits::e64;
        else if (counts & vk::SampleCountFlagBits::e32)
            ctx_.samples = vk::SampleCountFlagBits::e32;
        else if (counts & vk::SampleCountFlagBits::e16)
            ctx_.samples = vk::SampleCountFlagBits::e16;
        else if (counts & vk::SampleCountFlagBits::e8)
            ctx_.samples = vk::SampleCountFlagBits::e8;
        else if (counts & vk::SampleCountFlagBits::e4)
            ctx_.samples = vk::SampleCountFlagBits::e4;
        else if (counts & vk::SampleCountFlagBits::e2)
            ctx_.samples = vk::SampleCountFlagBits::e2;
    }
}

bool Shell::determineSwapchainExtent(uint32_t widthHint, uint32_t heightHint, bool refreshCapabilities) {
    // 0, 0 for hints indicates calling from acquire_back_buffer... TODO: more robust solution???
    if (refreshCapabilities) ctx_.surfaceProps.capabilities = ctx_.physicalDev.getSurfaceCapabilitiesKHR(ctx_.surface);

    auto &caps = ctx_.surfaceProps.capabilities;

    vk::Extent2D extent = caps.currentExtent;

    // use the hints
    if (extent.width == (uint32_t)-1) {
        extent.width = widthHint;
        extent.height = heightHint;
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

void Shell::determineDepthFormat() {
    /* allow custom depth formats */
#ifdef __ANDROID__
    // Depth format needs to be vk::Format::eD24UnormS8Uint on Android.
    ctx_.depth_format = vk::Format::eD24UnormS8Uint;
#elif defined(VK_USE_PLATFORM_IOS_MVK)
    if (ctx_.depth_format == vk::Format::eUndefined) ctx_.depth_format = vk::Format::eD32Sfloat;
#else
    if (ctx_.depthFormat == vk::Format::eUndefined) ctx_.depthFormat = helpers::findDepthFormat(ctx_.physicalDev);
        // TODO: turn off depth if undefined still...
#endif
}

void Shell::determineSwapchainSurfaceFormat() {
    // no preferred type
    if (ctx_.surfaceProps.formats.size() == 1 && ctx_.surfaceProps.formats[0].format == vk::Format::eUndefined) {
        ctx_.surfaceFormat = {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
        return;
    } else {
        for (const auto &surfFormat : ctx_.surfaceProps.formats) {
            if (surfFormat.format == vk::Format::eB8G8R8A8Unorm &&
                surfFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
                ctx_.surfaceFormat = surfFormat;
                return;
            }
        }
    }
    log(LogPriority::LOG_INFO, "choosing first available swap surface format!");
    ctx_.surfaceFormat = ctx_.surfaceProps.formats[0];
}

void Shell::determineSwapchainPresentMode() {
    // guaranteed to be present (vert sync)
    vk::PresentModeKHR presentMode = vk::PresentModeKHR::eFifo;
    for (const auto &mode : ctx_.surfaceProps.presentModes) {
        if (mode == vk::PresentModeKHR::eMailbox) {  // triple buffer
            presentMode = mode;
        } else if (mode == vk::PresentModeKHR::eImmediate) {  // no sync
            presentMode = mode;
        }
    }
    ctx_.mode = presentMode;
}

void Shell::determineSwapchainImageCount() {
    auto &caps = ctx_.surfaceProps.capabilities;
    // Determine the number of vk::Image's to use in the swap chain.
    // We need to acquire only 1 presentable image at at time.
    // Asking for minImageCount images ensures that we can acquire
    // 1 presentable image as long as we present it before attempting
    // to acquire another.
    ctx_.imageCount = settings_.backBufferCount;
    if (ctx_.imageCount < caps.minImageCount)
        ctx_.imageCount = caps.minImageCount;
    else if (ctx_.imageCount > caps.maxImageCount)
        ctx_.imageCount = caps.maxImageCount;
}

void Shell::Context::createBuffer(const vk::CommandBuffer &cmd, const vk::BufferUsageFlags usage, const vk::DeviceSize size,
                                  const std::string &&name, BufferResource &stgRes, BufferResource &buffRes,
                                  const void *data, const bool mappable) const {
    // STAGING RESOURCE
    buffRes.memoryRequirements.size =
        helpers::createBuffer(dev, size, vk::BufferUsageFlagBits::eTransferSrc,
                              vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, memProps,
                              stgRes.buffer, stgRes.memory);

    // FILL STAGING BUFFER ON DEVICE
    void *pData;
    pData = dev.mapMemory(stgRes.memory, 0, buffRes.memoryRequirements.size);
    /*
        You can now simply memcpy the vertex data to the mapped memory and unmap it again using unmapMemory.
        Unfortunately the driver may not immediately copy the data into the buffer memory, for example because
        of caching. It is also possible that writes to the buffer are not visible in the mapped memory yet. There
        are two ways to deal with that problem:
            - Use a memory heap that is host coherent, indicated with vk::MemoryPropertyFlagBits::eHostCoherent
            - Call flushMappedMemoryRanges to after writing to the mapped memory, and call
              invalidateMappedMemoryRanges before reading from the mapped memory
        We went for the first approach, which ensures that the mapped memory always matches the contents of the
        allocated memory. Do keep in mind that this may lead to slightly worse performance than explicit flushing,
        but we'll see why that doesn't matter in the next chapter.
    */
    memcpy(pData, data, static_cast<size_t>(size));
    dev.unmapMemory(stgRes.memory);

    // FAST VERTEX BUFFER
    vk::MemoryPropertyFlags memPropFlags = vk::MemoryPropertyFlagBits::eDeviceLocal;
    if (mappable) memPropFlags |= vk::MemoryPropertyFlagBits::eHostVisible;
    helpers::createBuffer(dev, size,
                          // TODO: probably don't need to check memory requirements again
                          usage, memPropFlags, memProps, buffRes.buffer, buffRes.memory);

    // COPY FROM STAGING TO FAST
    helpers::copyBuffer(cmd, stgRes.buffer, buffRes.buffer, buffRes.memoryRequirements.size);
    dbg.setMarkerName(buffRes.buffer, name.c_str());
}
