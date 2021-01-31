/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include <map>
#include <queue>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "Debug.h"
#include "Types.h"

class Context {
   public:
    Context();
    ~Context() = default;

    struct BackBuffer {
        uint32_t imageIndex;
        vk::Semaphore acquireSemaphore;
        vk::Semaphore renderSemaphore;
        // signaled when this struct is ready for reuse
        vk::Fence presentFence;
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

    std::vector<const char *> instanceEnabledLayerNames;
    std::vector<const char *> instanceEnabledExtensionNames;
    vk::Instance instance;

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
    vk::AllocationCallbacks *pAllocator;

    // SURFACE (TODO: figure out what is what)
    SurfaceProperties surfaceProps;
    vk::SurfaceFormatKHR surfaceFormat;
    vk::SurfaceKHR surface;
    vk::PresentModeKHR mode;
    vk::SampleCountFlagBits samples;
    uint32_t imageCount;
    vk::Extent2D extent;
    glm::mat3 normalizedScreenSpace;

    // DEPTH
    vk::Format depthFormat;

    // SWAPCHAIN (TODO: figure out what is what)
    vk::SwapchainKHR swapchain;
    std::queue<BackBuffer> backBuffers;
    BackBuffer acquiredBackBuffer;
    vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;

    void initInstance(const char *appName, const char *engineName, const bool validate);
    void destroyInstance();

    void initDevice();
    void destroyDevice();

    void initDebug(const bool validate, const bool validateVerbose, const PFN_vkDebugUtilsMessengerCallbackEXT pCallback,
                   void *pUserData);
    void destroyDebug();

    void createBuffer(const vk::CommandBuffer &cmd, const vk::BufferUsageFlags usage, const vk::DeviceSize size,
                      const std::string &&name, BufferResource &stgRes, BufferResource &buffRes, const void *data,
                      const bool mappable = false) const;
    void destroyBuffer(BufferResource &res) const;

   private:
    // DEBUG
    vk::DebugUtilsMessengerEXT debugUtilsMessenger_;
    Debug::Base dbg_;
};

#endif  // !CONTEXT_H