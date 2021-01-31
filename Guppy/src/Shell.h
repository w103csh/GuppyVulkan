/*
 * Modifications copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
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

#include <Common/Context.h>
#include <Common/Types.h>

#include "Game.h"

// clang-format off
namespace Input { class Handler; }
namespace Sound { class Handler; }
// clang-format on

class Shell {
   public:
    // Used to peg time related things to fixed factor. Assume 60 fps is a decent target.
    static constexpr double NORMALIZED_ELAPSED_TIME_FACTOR = 1.0 / 60.0;

    Shell(const Shell &sh) = delete;
    Shell &operator=(const Shell &sh) = delete;
    virtual ~Shell();

    struct LayerProperties {
        vk::LayerProperties properties;
        std::vector<vk::ExtensionProperties> extensionProps;
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
    template <typename T = double>
    constexpr T getNormalizedElaspedTime() const {
        static_assert(std::is_floating_point<T>::value, "T must be a floating point type");
        return static_cast<T>(normalizedElapsedTime_);
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

        const auto &shell() const { return *pShell_; }

       protected:
        Handler(Shell *pShell) : pShell_(pShell) {}
        virtual ~Handler() = default;

        // LIFECYCLE
        virtual void init() = 0;
        virtual void update() = 0;
        virtual void destroy() = 0;

       private:
        Shell *pShell_;
    };

    inline Input::Handler &inputHandler() const { return std::ref(*handlers_.pInput); }
    inline Sound::Handler &soundHandler() const { return std::ref(*handlers_.pSound); }

   protected:
    Shell(Game &game, Handlers &&handlers);

    void init();
    void update();
    void addGameTime();
    void destroy();

    virtual void setPlatformSpecificExtensions() = 0;
    void cleanup();

    void addInstanceEnabledLayerName(const char *name) { ctx_.instanceEnabledLayerNames.push_back(name); }
    void addInstanceEnabledExtensionName(const char *name) { ctx_.instanceEnabledExtensionNames.push_back(name); }

    void createContext();
    virtual void destroyContext();

    // SWAPCHAIN
    void acquireBackBuffer();
    void presentBackBuffer();

    Game &game_;
    const Game::Settings &settings_;

    std::vector<Context::ExtensionInfo> deviceExtensionInfo_;

    std::vector<LayerProperties> layerProps_;
    std::vector<vk::ExtensionProperties> instExtProps_;

    // Values are relative to seconds.
    double currentTime_;
    double elapsedTime_;
    double normalizedElapsedTime_;

    const Handlers handlers_;

   private:
    void processInput();

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
    void initPhysicalDevice();

    // called by enumerateInstanceLayerExtensionProperties
    void enumerateDeviceLayerExtensionProperties(Context::PhysicalDeviceProperties &props, LayerProperties &layerProps);

    // called by initPhysicalDev
    void enumeratePhysicalDevices(uint32_t physicalDevCount = 1);
    void pickPhysicalDevice();

    // called by pickDevice
    bool isDeviceSuitable(const Context::PhysicalDeviceProperties &props, uint32_t &graphicsIndex, uint32_t &presentIndex,
                          uint32_t &transferIndex, uint32_t &computeIndex);

    // called by isDevSuitable
    bool determineQueueFamiliesSupport(const Context::PhysicalDeviceProperties &props, uint32_t &graphicsIndex,
                                       uint32_t &presentIndex, uint32_t &transferIndex, uint32_t &computeIndex);
    bool determineDeviceExtensionSupport(const Context::PhysicalDeviceProperties &props);
    void determineDeviceFeatureSupport(const Context::PhysicalDeviceProperties &props);
    void determineSampleCount(const Context::PhysicalDeviceProperties &props);

    // called by createContext
    void initDeviceQueues();
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

    Context ctx_;

    const double gameTick_;
    double gameTime_;

    vk::DebugUtilsMessengerEXT debugUtilsMessenger_;
};

#endif  // SHELL_H
