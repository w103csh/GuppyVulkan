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

#ifndef SHELL_H
#define SHELL_H

#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <vector>
#include <stdexcept>
#include <type_traits>
#include <vulkan/vulkan.hpp>

#include "Debug.h"
#include "Game.h"
#include "Types.h"

// clang-format off
namespace Input { class Handler; }
namespace Sound { class Handler; }
// clang-format on

class Shell {
   public:
    Shell(const Shell &sh) = delete;
    Shell &operator=(const Shell &sh) = delete;
    virtual ~Shell();

    struct BackBuffer {
        uint32_t imageIndex;
        vk::Semaphore acquireSemaphore;
        vk::Semaphore renderSemaphore;
        // signaled when this struct is ready for reuse
        vk::Fence presentFence;
    };

    struct LayerProperties {
        vk::LayerProperties properties;
        std::vector<vk::ExtensionProperties> extensionProps;
    };

    struct SurfaceProperties {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> presentModes;
    };

    struct ExtensionInfo {
        const char *name;
        bool required;
        bool tryToEnabled;
        bool valid;
    };

    struct PhysicalDeviceProperties {
        vk::PhysicalDevice device;
        uint32_t queueFamilyCount;
        std::vector<vk::QueueFamilyProperties> queueProps;
        vk::PhysicalDeviceMemoryProperties memoryProperties;
        vk::PhysicalDeviceProperties properties;
        std::vector<vk::ExtensionProperties> extensionProperties;
        std::multimap<const char *, vk::ExtensionProperties, less_str> layerExtensionMap;
        vk::PhysicalDeviceFeatures features;
        // Physical device extensions
        std::vector<ExtensionInfo> phyDevExtInfos;
        vk::PhysicalDeviceVertexAttributeDivisorFeaturesEXT featVertAttrDiv;
        // vk::PhysicalDeviceVertexAttributeDivisorPropertiesEXT propsVertAttrDiv;
        vk::PhysicalDeviceTransformFeedbackFeaturesEXT featTransFback;
        // vk::PhysicalDeviceTransformFeedbackPropertiesEXT propsTransFback;
    };

    struct Context {
        vk::Instance instance;

        // TODO: make these const after setting somehow.
        bool samplerAnisotropyEnabled;
        bool sampleRateShadingEnabled;
        bool linearBlittingSupported;
        bool computeShadingEnabled;
        bool tessellationShadingEnabled;
        bool geometryShadingEnabled;
        bool wireframeShadingEnabled;
        bool vertexAttributeDivisorEnabled;
        bool transformFeedbackEnabled;
        bool debugMarkersEnabled;
        bool independentBlendEnabled;
        bool imageCubeArrayEnabled;

        vk::PhysicalDevice physicalDev;
        std::vector<PhysicalDeviceProperties> physicalDevProps;
        uint32_t physicalDevIndex;
        vk::PhysicalDeviceMemoryProperties memProps;
        std::map<uint32_t, vk::Queue> queues;
        uint32_t graphicsIndex;
        uint32_t presentIndex;
        uint32_t transferIndex;
        uint32_t computeIndex;

        vk::Device dev;

        // SURFACE (TODO: figure out what is what)
        SurfaceProperties surfaceProps;
        vk::SurfaceFormatKHR surfaceFormat;
        vk::SurfaceKHR surface;
        vk::PresentModeKHR mode;
        vk::SampleCountFlagBits samples;
        uint32_t imageCount;
        vk::Extent2D extent;

        // DEPTH
        vk::Format depthFormat = {};

        // SWAPCHAIN (TODO: figure out what is what)
        vk::SwapchainKHR swapchain;
        std::queue<BackBuffer> backBuffers;
        BackBuffer acquiredBackBuffer;
        vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

        // DEBUG
        Debug::Base dbg;

        void createBuffer(const vk::CommandBuffer &cmd, const vk::BufferUsageFlags usage, const vk::DeviceSize size,
                          const std::string &&name, BufferResource &stgRes, BufferResource &buffRes, const void *data,
                          const bool mappable = false) const;
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

    void onButton(const GameButtonBits buttons);
    void onKey(const GAME_KEY key);
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

    virtual uint32_t getDesiredVersion() { return VK_API_VERSION_1_2; }
    virtual void setPlatformSpecificExtensions() = 0;
    void cleanup();

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

    std::vector<LayerProperties> layerProps_;
    std::vector<vk::ExtensionProperties> instExtProps_;

    double currentTime_, elapsedTime_;
    const Handlers handlers_;

   private:
    void assertAllInstanceLayers() const;
    void assertAllInstanceExtensions() const;

    bool hasAllDeviceLayers(vk::PhysicalDevice phy) const;
    bool hasAllDeviceExtensions(vk::PhysicalDevice phy) const;

    // called by initInstance
    void enumerateInstanceProperties();

    // called by init
    virtual PFN_vkGetInstanceProcAddr load() = 0;
    virtual vk::Bool32 canPresent(vk::PhysicalDevice phy, uint32_t queueFamily) = 0;
    void initInstance();
    void initDebugUtilsMessenger();
    void initPhysicalDev();

    // called by enumerateInstanceLayerExtensionProperties
    void enumerateDeviceLayerExtensionProperties(PhysicalDeviceProperties &props, LayerProperties &layerProps);

    // called by initPhysicalDev
    void enumeratePhysicalDevs(uint32_t physicalDevCount = 1);
    void pickPhysicalDev();

    // called by pickDevice
    bool isDevSuitable(const PhysicalDeviceProperties &props, uint32_t &graphicsIndex, uint32_t &presentIndex,
                       uint32_t &transferIndex, uint32_t &computeIndex);

    // called by isDevSuitable
    bool determineQueueFamiliesSupport(const PhysicalDeviceProperties &props, uint32_t &graphicsIndex,
                                       uint32_t &presentIndex, uint32_t &transferIndex, uint32_t &computeIndex);
    bool determineDeviceExtensionSupport(const PhysicalDeviceProperties &props);
    void determineDeviceFeatureSupport(const PhysicalDeviceProperties &props);
    void determineSampleCount(const PhysicalDeviceProperties &props);

    // called by createContext
    void initDevQueues();
    void createDev();
    void createBackBuffers();
    void destroyBackBuffers();
    virtual void createWindow() = 0;
    virtual vk::SurfaceKHR createSurface(vk::Instance instance) = 0;
    void createSwapchain();
    void destroySwapchain();

    // called by createSwapchain
    void enumerateSurfaceProperties();
    void determineDepthFormat();
    void determineSwapchainSurfaceFormat();
    void determineSwapchainPresentMode();
    void determineSwapchainImageCount();

    // called by resizeSwapchain
    bool determineSwapchainExtent(uint32_t widthHint, uint32_t heightHint, bool refreshCapabilities);

    // called by cleanup
    void destroyInstance();

    Context ctx_;

    const float gameTick_;
    float gameTime_;

    vk::DebugUtilsMessengerEXT debugUtilsMessenger_;
};

#endif  // SHELL_H
