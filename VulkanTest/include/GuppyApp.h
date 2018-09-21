
#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>

#include <vector>
#include <functional>

#include "Camera.h"
#include "InputHandler.h"
#include "Geometry/Model.h"

struct QueueFamilyIndices
{
    int graphicsFamily = -1;
    int presentFamily = -1;
    int transferFamily = -1;

    bool isComplete()
    {
        return graphicsFamily >= 0 && presentFamily >= 0 && transferFamily >= 0;
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct UniformBufferObject
{
    Camera perspective;
};

class GuppyApp
{
public:

    void run();

    void createBuffer(VkDeviceSize size,
                      VkBufferUsageFlags usage,
                      VkMemoryPropertyFlags properties,
                      VkBuffer& buffer,
                      VkDeviceMemory& bufferMemory);

    void createImage(const uint32_t width,
                     const uint32_t height,
                     const uint32_t mipLevels,
                     const VkSampleCountFlagBits numSamples,
                     const VkFormat format,
                     const VkImageTiling tiling,
                     const VkImageUsageFlags usage,
                     const VkMemoryPropertyFlags properties,
                     VkImage& image,
                     VkDeviceMemory& imageMemory);

    // events
    bool m_framebufferResized = false;

private:

    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();


    void createInstance();
    void setupDebugCallback();
    void createSurface();
    std::vector<const char*> getRequiredExtensions();
    void checkExtensions();
    bool checkValidationLayerSupport();
    void pickPhysicalDevice();
    bool isDeviceSuitable(VkPhysicalDevice device);
    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    int rateDeviceSuitability(VkPhysicalDevice device);
    void createLogicalDevice();
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    void createSwapChain();
    void createSwapChainImageViews();
    VkImageView createImageView(const uint32_t mipLevels,
                                const VkImage image,
                                const VkFormat format,
                                const VkImageAspectFlags aspectFlags,
                                const VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
    void createGraphicsPipeline();
    VkShaderModule createShaderModule(const std::vector<char>& code);
    void createRenderPass();
    void createFramebuffers();
    void createCommandPools();
    void createGraphicsCommandBuffers();
    void drawFrame();
    void createSyncObjects();
    void cleanupSwapChain();
    void recreateSwapChain();
    // This pattern is a bit wonky methinks
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void createVertexBuffer();
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void createIndexBuffer();
    void createDescriptorSetLayout();
    void createUniformBuffers();
    void updateUniformBuffer(uint32_t currentImage);
    void createDescriptorPool();
    void createDescriptorSets();
    void createTextureImage(std::string texturePath);
    void transitionImageLayout(const uint32_t srcQueueFamilyIndex,
                               const uint32_t dstQueueFamilyIndex,
                               const uint32_t mipLevels,
                               const VkImage image,
                               const VkFormat format,
                               const VkImageLayout oldLayout,
                               const VkImageLayout newLayout,
                               const VkCommandPool commandPool);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    void createTextureImageView();
    void createTextureSampler();
    void addSetupCommandBuffer(const VkCommandPool commandPool, const std::function<void(VkCommandBuffer&)>& lambda);
    void flushSetupCommandBuffers(const VkCommandPool commandPool, const VkQueue queue, const std::function<void()>& lambda);
    void createModel();
    void createDepthResources();
    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling tiling, const VkFormatFeatureFlags features);
    VkFormat findDepthFormat();
    bool hasStencilComponent(VkFormat format);
    void generateMipmaps(const VkImage image,
                         const VkFormat imageFormat,
                         const int32_t texWidth,
                         const int32_t texHeight,
                         const uint32_t mipLevels);
    VkSampleCountFlagBits getMaxUsableSampleCount();
    void createColorResources();

    // this is a bit wonky methinks
    UniformBufferObject m_ubo = {};

    // Abstracted window from the operating system
    GLFWwindow* m_window;
    // vulkan instance
    VkInstance m_instance;
    // validation layer callback
    VkDebugUtilsMessengerEXT m_callback;
    // surface
    VkSurfaceKHR m_surface;
    // device
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue; // implicitly destroyed by m_device
    VkQueue m_presentQueue; // implicitly destroyed by m_device
    VkQueue m_transferQueue; // implicitly destroyed by m_device
    // swap chain
    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImageView> m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    // graphics pipeline
    VkRenderPass m_renderPass;
    VkDescriptorSetLayout m_descriptorSetLayout;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;
    // commands
    VkCommandPool m_graphicsCommandPool, m_transferCommandPool;
    std::vector<VkCommandBuffer> m_graphicsCommandBuffers, m_setupCommandBuffers;
    // sync objects
    // GPU -> GPU
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    // CPU -> GPU
    std::vector<VkFence> m_inFlightFences;
    size_t m_currentFrame = 0;
    // buffers
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;
    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    // texture
    uint32_t m_mipLevels;
    VkImage m_textureImage;
    VkDeviceMemory m_textureImageMemory;
    VkImageView m_textureImageView;
    VkSampler m_textureSampler;
    // model
    Model m_model;
    // depth
    VkImage m_depthImage;
    VkDeviceMemory m_depthImageMemory;
    VkImageView m_depthImageView;
    // mulit-sampling
    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    VkImage m_colorImage;
    VkDeviceMemory m_colorImageMemory;
    VkImageView m_colorImageView;
};