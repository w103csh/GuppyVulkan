/*
 * This came from LunarG Hologram sample, but has been radically altered at this point.
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
        std::vector<VkSurfaceFormatKHR> surfFormats;
        std::vector<VkPresentModeKHR> presentModes;
    };  // *

    struct PhysicalDeviceProperties {
        VkPhysicalDevice device;
        uint32_t queueFamilyCount;
        std::vector<VkQueueFamilyProperties> queueProps;
        VkPhysicalDeviceMemoryProperties memoryProperties;
        VkPhysicalDeviceProperties properties;
        std::vector<VkExtensionProperties> extensions;
        std::multimap<const char *, VkExtensionProperties, less_str> layerExtensionMap = {};
        VkPhysicalDeviceFeatures features;
    };  // *

    struct Context {
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugReportCallbackEXT debugReport = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugUtilsMessenger = VK_NULL_HANDLE;

        // TODO: These should not be public...
        bool samplerAnisotropyEnabled_ = false;    // *
        bool sampleRateShadingEnabled_ = false;    // *
        bool linearBlittingSupported_ = false;     // *
        bool computeShadingEnabled_ = false;       // *
        bool tessellationShadingEnabled_ = false;  // *
        bool geometryShadingEnabled_ = false;      // *
        bool wireframeShadingEnabled_ = false;     // *

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

    constexpr const Context &context() const { return ctx_; }

    // LOGGING
    enum LogPriority {
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

    inline void onKey(GAME_KEY key) { game_.onKey(key); }                   // TODO: think this through
    inline void onMouse(const MouseInput &input) { game_.onMouse(input); }  // TODO: think this through

    // SWAPCHAIN
    void resizeSwapchain(uint32_t widthHint, uint32_t heightHint, bool refreshCapabilities = true);

   protected:
    Shell(Game &game);

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

    std::vector<const char *> deviceExtensions_;

    std::vector<LayerProperties> layerProps_;          // *
    std::vector<VkExtensionProperties> instExtProps_;  // *

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
};

#endif  // SHELL_H
