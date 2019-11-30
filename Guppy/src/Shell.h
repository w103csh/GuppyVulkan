/*
 * Modifications copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
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

#ifndef SHELL_H
#define SHELL_H

#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <vulkan/vulkan.h>

#include "Game.h"

// clang-format off
namespace Input { class Handler; }
namespace Sound { class Handler; }
// clang-format on

// structure for comparing char arrays
struct less_str {
    bool operator()(char const *a, char const *b) const { return std::strcmp(a, b) < 0; }
};

class Shell {
   public:
    Shell(const Shell &sh) = delete;
    Shell &operator=(const Shell &sh) = delete;
    virtual ~Shell();

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
        std::vector<VkSurfaceFormatKHR> surfFormats;
        std::vector<VkPresentModeKHR> presentModes;
    };  // *

    struct ExtensionInfo {
        const char *name;
        bool required;
        bool tryToEnabled;
        bool valid;
    };

    struct PhysicalDeviceProperties {
        VkPhysicalDevice device;
        uint32_t queueFamilyCount;
        std::vector<VkQueueFamilyProperties> queueProps;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        VkPhysicalDeviceProperties properties;
        std::vector<VkExtensionProperties> extensionProperties;
        std::multimap<const char *, VkExtensionProperties, less_str> layerExtensionMap;
        VkPhysicalDeviceFeatures features;
        // Physical device extensions
        std::vector<ExtensionInfo> phyDevExtInfos;
        VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT featVertAttrDiv{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_ATTRIBUTE_DIVISOR_FEATURES_EXT};
        // VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT propsVertAttrDiv;
        VkPhysicalDeviceTransformFeedbackFeaturesEXT featTransFback{
            VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TRANSFORM_FEEDBACK_FEATURES_EXT};
        // VkPhysicalDeviceTransformFeedbackPropertiesEXT propsTransFback;
    };

    struct Context {
        VkInstance instance = VK_NULL_HANDLE;

        // TODO: make these const after setting somehow.
        bool samplerAnisotropyEnabled = false;
        bool sampleRateShadingEnabled = false;
        bool linearBlittingSupported = false;
        bool computeShadingEnabled = false;
        bool tessellationShadingEnabled = false;
        bool geometryShadingEnabled = false;
        bool wireframeShadingEnabled = false;
        bool vertexAttributeDivisorEnabled = false;
        bool transformFeedbackEnabled = false;
        bool debugMarkersEnabled = false;
        bool independentBlendEnabled = false;

        VkPhysicalDevice physicalDev = VK_NULL_HANDLE;
        std::vector<PhysicalDeviceProperties> physicalDevProps;  // *
        uint32_t physicalDevIndex = 0;                           // *
        VkPhysicalDeviceMemoryProperties memProps = {};          // *
        std::map<uint32_t, VkQueue> queues;                      // *
        uint32_t graphicsIndex = 0;                              // *
        uint32_t presentIndex = 0;                               // *
        uint32_t transferIndex = 0;                              // *
        uint32_t computeIndex = 0;                               // *

        VkDevice dev = VK_NULL_HANDLE;

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
        VkPipelineStageFlags waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    };

    constexpr const auto &context() const { return ctx_; }
    template <typename T = double>
    constexpr T getCurrentTime() const {
        static_assert(std::is_floating_point<T>::value, "T must be a floating point type");
        return static_cast<T>(currentTime_);
    }
    template <typename T = double>
    constexpr T getElapsedTime() const {
        static_assert(std::is_floating_point<T>::value, "T must be a floating point type");
        return static_cast<T>(elapsedTime_);
    }

    // LOGGING
    enum class LogPriority {
        LOG_DEBUG,
        LOG_INFO,
        LOG_WARN,
        LOG_ERR,
    };
    virtual void log(LogPriority priority, const char *msg) const;

    virtual void run() = 0;
    virtual void quit() const = 0;

    // SHADER RECOMPILING
    virtual void asyncAlert(uint64_t milliseconds) = 0;  // TODO: think this through
    virtual void checkDirectories() = 0;                 // TODO: think this through
    virtual void watchDirectory(const std::string &directory,
                                const std::function<void(std::string)> callback) = 0;  // TODO: think this through

    void onKey(GAME_KEY key);
    inline void onMouse(const MouseInput &input) { game_.onMouse(input); }  // TODO: think this through

    // SWAPCHAIN
    void resizeSwapchain(uint32_t widthHint, uint32_t heightHint, bool refreshCapabilities = true);

    bool limitFramerate;
    uint8_t framesPerSecondLimit;

    // HANDLERS
    struct Handlers {
        std::unique_ptr<Input::Handler> pInput;
        std::unique_ptr<Sound::Handler> pSound;
    };

    // HANDLER
    class Handler {
       public:
        Handler(const Handler &) = delete;             // Prevent construction by copying
        Handler &operator=(const Handler &) = delete;  // Prevent assignment

        inline Input::Handler &inputHandler() const { return std::ref(*pShell_->handlers_.pInput); }
        inline Sound::Handler &soundHandler() const { return std::ref(*pShell_->handlers_.pSound); }

       protected:
        Handler(Shell *pShell) : pShell_(pShell) {}
        virtual ~Handler() = default;

        virtual void init() = 0;
        virtual void update(const double elapsed) = 0;
        virtual void destroy() = 0;

        Shell *pShell_;
    };

    inline Input::Handler &inputHandler() const { return std::ref(*handlers_.pInput); }
    inline Sound::Handler &soundHandler() const { return std::ref(*handlers_.pSound); }

   protected:
    Shell(Game &game, Handlers &&handlers);

    void init();
    void update(const double elapsed);
    void destroy();

    virtual uint32_t getDesiredVersion() { return VK_MAKE_VERSION(1, 1, 0); }
    virtual void setPlatformSpecificExtensions() = 0;
    void initVk();
    void cleanupVk();

    void createContext();
    virtual void destroyContext();

    void addGameTime(float time);

    // SWAPCHAIN
    void acquireBackBuffer();
    void presentBackBuffer();

    Game &game_;
    const Game::Settings &settings_;

    std::vector<const char *> instanceLayers_;
    std::vector<const char *> instanceExtensions_;

    std::vector<ExtensionInfo> deviceExtensionInfo_;

    std::vector<LayerProperties> layerProps_;          // *
    std::vector<VkExtensionProperties> instExtProps_;  // *

    double currentTime_, elapsedTime_;
    const Handlers handlers_;

   private:
    bool debugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t object,
                             size_t location, int32_t msgCode, const char *layerPrefix, const char *msg);
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugReportCallback(VkDebugReportFlagsEXT flags,
                                                              VkDebugReportObjectTypeEXT objType, uint64_t object,
                                                              size_t location, int32_t msgCode, const char *layerPrefix,
                                                              const char *msg, void *userData) {
        Shell *shell = reinterpret_cast<Shell *>(userData);
        return shell->debugReportCallback(flags, objType, object, location, msgCode, layerPrefix, msg);
    }

    void assertAllInstanceLayers() const;
    void assertAllInstanceExtensions() const;

    bool hasAllDeviceLayers(VkPhysicalDevice phy) const;
    bool hasAllDeviceExtensions(VkPhysicalDevice phy) const;

    // called by initInstance
    void enumerateInstanceProperties();           // *
    void determineApiVersion(uint32_t &version);  // *

    // called by initVk
    void enumerateInstanceLayerExtensionProperties(LayerProperties &layerProps);  // *
    void initValidationMessenger();                                               // *
    virtual PFN_vkGetInstanceProcAddr loadVk() = 0;
    virtual VkBool32 canPresent(VkPhysicalDevice phy, uint32_t queueFamily) = 0;
    void initInstance();
    void initDebugReport();
    void initPhysicalDev();

    // called by enumerateInstanceLayerExtensionProperties
    void enumerateInstanceExtensionProperties();                                                                 // *
    void enumerateDeviceLayerExtensionProperties(PhysicalDeviceProperties &props, LayerProperties &layerProps);  // *

    // called by initPhysicalDev
    void enumeratePhysicalDevs(uint32_t physicalDevCount = 1);  // *
    void pickPhysicalDev();                                     // *

    // called by pickDevice
    bool isDevSuitable(const PhysicalDeviceProperties &props, uint32_t &graphicsIndex, uint32_t &presentIndex,
                       uint32_t &transferIndex, uint32_t &computeIndex);  // *

    // called by isDevSuitable
    bool determineQueueFamiliesSupport(const PhysicalDeviceProperties &props, uint32_t &graphicsIndex,
                                       uint32_t &presentIndex, uint32_t &transferIndex, uint32_t &computeIndex);  // *
    bool determineDeviceExtensionSupport(const PhysicalDeviceProperties &props);                                  // *
    void determineDeviceFeatureSupport(const PhysicalDeviceProperties &props);                                    // *
    void determineSampleCount(const PhysicalDeviceProperties &props);                                             // *

    // called by createContext
    void initDevQueues();  // *
    void createDev();
    void createBackBuffers();
    void destroyBackBuffers();
    virtual void createWindow() = 0;
    virtual VkSurfaceKHR createSurface(VkInstance instance) = 0;
    void createSwapchain();
    void destroySwapchain();

    // called by createSwapchain
    void enumerateSurfaceProperties();       // *
    void determineDepthFormat();             // *
    void determineSwapchainSurfaceFormat();  // *
    void determineSwapchainPresentMode();    // *
    void determineSwapchainImageCount();     // *

    // called by resizeSwapchain
    bool determineSwapchainExtent(uint32_t widthHint, uint32_t heightHint, bool refreshCapabilities);  // *

    // called by cleanupVk
    void destroyInstance();  // *

    Context ctx_;

    const float gameTick_;
    float gameTime_;

    VkDebugReportCallbackEXT debugReportCallback_;
    VkDebugUtilsMessengerEXT debugUtilsMessenger_;
};

#endif  // SHELL_H
