
#ifndef GUPPY_H
#define GUPPY_H

//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>
//#define GLM_FORCE_RADIANS
//#include <glm/glm.hpp>

#include <vector>
#include <set>

#include "Camera.h"
#include "Game.h"
#include "Helpers.h"
//#include "InputHandler.h"
#include "Model.h"
#include "MyShell.h"
#include "Texture.h"
#include "util_init.hpp"

class Guppy : public Game {
   public:
    Guppy(const std::vector<std::string>& args);
    ~Guppy();

    void attach_shell(MyShell& sh);
    void detach_shell();

    void attach_swapchain();
    void detach_swapchain();

    void my_init_vk();
    void my_cleanup_vk();

    void on_key(Key key);
    void on_tick();

    void on_frame(float frame_pred);

    // void createBuffer(VkDeviceSize size,
    //                  VkBufferUsageFlags usage,
    //                  VkMemoryPropertyFlags properties,
    //                  VkBuffer& buffer,
    //                  VkDeviceMemory& bufferMemory);

    // void createImage(const uint32_t width,
    //                 const uint32_t height,
    //                 const uint32_t mipLevels,
    //                 const VkSampleCountFlagBits numSamples,
    //                 const VkFormat format,
    //                 const VkImageTiling tiling,
    //                 const VkImageUsageFlags usage,
    //                 const VkMemoryPropertyFlags properties,
    //                 VkImage& image,
    //                 VkDeviceMemory& imageMemory);

    //// events
    // bool m_framebufferResized = false;

   private:
    struct FrameData {
        // signaled when this struct is ready for reuse
        VkFence fence;

        VkCommandBuffer primary_cmd;
        std::vector<VkCommandBuffer> worker_cmds;

        VkBuffer buf;
        uint8_t* base;
        VkDescriptorSet desc_set;
    };

    bool multithread_;
    bool use_push_constants_;

    VkPhysicalDevice physical_dev_;
    VkDevice dev_;
    VkQueue queue_;
    uint32_t queue_family_;
    VkFormat format_;
    VkDeviceSize aligned_object_data_size;

    VkPhysicalDeviceProperties physical_dev_props_;
    std::vector<VkMemoryPropertyFlags> mem_flags_;

    Model model_;

    VkRenderPass render_pass_;
    ShaderResources vs_;
    ShaderResources fs_;
    std::vector<VkDescriptorSetLayout> desc_set_layouts_;
    VkPipelineLayout pipeline_layout_;
    VkPipelineCache pipeline_cache_;
    VkPipeline pipeline_;

    VkCommandPool primary_cmd_pool_;
    std::vector<VkCommandPool> worker_cmd_pools_;
    VkDescriptorPool desc_pool_;
    VkDeviceMemory frame_data_mem_;
    std::vector<FrameData> frame_data_;
    int frame_data_index_;

    VkClearValue render_pass_clear_value_;
    VkRenderPassBeginInfo render_pass_begin_info_;

    VkCommandBufferBeginInfo primary_cmd_begin_info_;
    VkPipelineStageFlags primary_cmd_submit_wait_stages_;
    VkSubmitInfo primary_cmd_submit_info_;

    VkExtent2D extent_;
    VkViewport viewport_;
    VkRect2D scissor_;

    struct SwapchainImageResource {
        VkImage image;
        VkImageView view;
    };
    uint32_t swapchain_image_count_;
    std::vector<SwapchainImageResource> swapchain_image_resources_;
    std::vector<VkImage> images_;
    std::vector<VkImageView> image_views_;
    std::vector<VkFramebuffer> framebuffers_;

    // called by attach_shell
    void create_render_pass(bool include_depth, bool include_color, bool clear = true,
                            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    void create_shader_modules();
    void create_descriptor_set_layout(const VkDescriptorSetLayoutCreateFlags descSetLayoutCreateFlags = 0);
    void create_pipeline_layout();
    void create_pipeline();

    void create_frame_data(int count);
    void destroy_frame_data();
    void create_fences();
    // void create_buffers();
    // void create_buffer_memory();
    void create_descriptor_sets(bool use_texture = true);  // *

    // called mostly by on_key
    void update_camera();

    // called by attach_swapchain
    void prepare_viewport();
    void prepare_framebuffers(const VkSwapchainKHR& swapchain);

    // *********************************************

    ImageResource color_resource_;
    ImageResource depth_resource_;
    UniformBufferResources ubo_resource_;

    CommandData cmd_data_;
    // cmd_data_ accessors
    // queues
    inline VkQueue& graphics_queue() { return cmd_data_.queues[cmd_data_.graphics_queue_family]; }
    inline VkQueue& present_queue() { return cmd_data_.queues[cmd_data_.present_queue_family]; }
    inline VkQueue& transfer_queue() { return cmd_data_.queues[cmd_data_.transfer_queue_family]; }
    // cmd_pool
    inline VkCommandPool& graphics_cmd_pool() { return cmd_data_.cmd_pools[cmd_data_.graphics_queue_family]; }
    inline VkCommandPool& present_cmd_pool() { return cmd_data_.cmd_pools[cmd_data_.present_queue_family]; }
    inline VkCommandPool& transfer_cmd_pool() { return cmd_data_.cmd_pools[cmd_data_.transfer_queue_family]; }
    // cmd
    inline VkCommandBuffer& graphics_cmd() { return cmd_data_.cmds[cmd_data_.graphics_queue_family]; }
    inline VkCommandBuffer& present_cmd() { return cmd_data_.cmds[cmd_data_.present_queue_family]; }
    inline VkCommandBuffer& transfer_cmd() { return cmd_data_.cmds[cmd_data_.transfer_queue_family]; }
    // unique
    inline std::set<uint32_t> get_unique_queue_families() {
        return std::set<uint32_t>{
            cmd_data_.graphics_queue_family,
            cmd_data_.present_queue_family,
            cmd_data_.transfer_queue_family,
        };
    }
    // drawing
    std::vector<VkCommandBuffer> draw_cmds_;

    Camera::Camera camera_;
    bool sample_shading_supported_ = false;
    VkSampleCountFlagBits num_samples_;
    struct sample_info info = {};

    BufferResource vertex_res_;
    BufferResource index_res_;
    std::vector<VkDescriptorSet> desc_sets_;
    std::vector<Texture::TextureData> textures_;

    // void initWindow();
    // void init_vulkan(struct sample_info& info);
    // void mainLoop();
    // void cleanup();

    void copyBuffer(VkCommandBuffer& cmd, const VkBuffer& src_buf, const VkBuffer& dst_buf, const VkDeviceSize& size);
    void create_input_assembly_data();
    void create_vertex_data(StagingBufferResource& stg_res);
    void create_index_data(StagingBufferResource& stg_res);
    /*
        void copy_buffer_to_image(VkCommandBuffer& cmd, const VkBuffer& src_buf, const VkImage& dst_img, const uint32_t& width,
                                  const uint32_t& height);
        VkFormat find_supported_format(struct sample_info& info, const std::vector<VkFormat>& candidates,
                                       const VkImageTiling tiling, const VkFormatFeatureFlags features);*/

    void create_draw_cmds();
    void create_model();
    void determine_sample_count(const MyShell::PhysicalDeviceProperties& props);
    void destroy_textures();
    void destroy_descriptor_and_pipelineLayouts();
    void create_pipeline_cache();
    void destroy_pipeline_cache();
    void destroy_pipeline();

    // called by attach_shell
    void create_command_pools_and_buffers();
    void create_descriptor_pool(bool use_texture = true);
    void create_uniform_buffer();

    // called by attach_swapchain
    void get_swapchain_image_data(const VkSwapchainKHR& swapchain);
    void create_color_resources();
    void create_depth_resources();

    // nothing
    void destroy_command_pools();
    void destroy_command_buffers();
    void destroy_color_resources();
    void destroy_depth_resources();
    void destroy_ubo_resources();

    //    void createInstance();
    //    void setupDebugCallback();
    //    void createSurface();
    //    std::vector<const char*> getRequiredExtensions();
    //    void checkExtensions();
    //    bool checkValidationLayerSupport();
    //    void pickPhysicalDevice();
    //    bool isDeviceSuitable(VkPhysicalDevice device);
    //    bool checkDeviceExtensionSupport(VkPhysicalDevice device);
    //    int rateDeviceSuitability(VkPhysicalDevice device);
    //    void createLogicalDevice();
    //    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    //    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    //    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    //    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
    //    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    //    void createSwapChain();
    //    void createSwapChainImageViews();
    //    VkImageView createImageView(const uint32_t mipLevels,
    //                                const VkImage image,
    //                                const VkFormat format,
    //                                const VkImageAspectFlags aspectFlags,
    //                                const VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
    //    void createGraphicsPipeline();
    //    VkShaderModule createShaderModule(const std::vector<char>& code);
    //    void createRenderPass();
    //    void createFramebuffers();
    //    void createCommandPools();
    //    void createGraphicsCommandBuffers();
    //    void drawFrame();
    //    void createSyncObjects();
    //    void cleanupSwapChain();
    //    void recreateSwapChain();
    //    // This pattern is a bit wonky methinks
    //    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    //    void createVertexBuffer();
    //    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    //    void createIndexBuffer();
    //    void createDescriptorSetLayout();
    //    void createUniformBuffers();
    //    void updateUniformBuffer(uint32_t currentImage);
    //    void createDescriptorPool();
    //    void createDescriptorSets();
    //    void createTextureImage(std::string texturePath);
    //    void transitionImageLayout(const uint32_t srcQueueFamilyIndex,
    //                               const uint32_t dstQueueFamilyIndex,
    //                               const uint32_t mipLevels,
    //                               const VkImage image,
    //                               const VkFormat format,
    //                               const VkImageLayout oldLayout,
    //                               const VkImageLayout newLayout,
    //                               const VkCommandPool commandPool);
    //    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
    //    void createTextureImageView();
    //    void createTextureSampler();
    //    void addSetupCommandBuffer(const VkCommandPool commandPool, const std::function<void(VkCommandBuffer&)>& lambda);
    //    void flushSetupCommandBuffers(const VkCommandPool commandPool, const VkQueue queue, const std::function<void()>& lambda);
    //    void createModel();
    //    void createDepthResources();
    //    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling tiling, const
    //    VkFormatFeatureFlags features); VkFormat findDepthFormat(); bool hasStencilComponent(VkFormat format); void
    //    generateMipmaps(const VkImage image,
    //                         const VkFormat imageFormat,
    //                         const int32_t texWidth,
    //                         const int32_t texHeight,
    //                         const uint32_t mipLevels);
    //    VkSampleCountFlagBits getMaxUsableSampleCount();
    //    void createColorResources();
    //
    //    // this is a bit wonky methinks
    //    UniformBufferObject m_ubo = {};
    //
    //    // Abstracted window from the operating system
    //    GLFWwindow* m_window;
    //    // swap chain
    //    VkSwapchainKHR m_swapChain;
    //    std::vector<VkImage> m_swapChainImages;
    //    VkFormat m_swapChainImageFormat;
    //    VkExtent2D m_swapChainExtent;
    //    std::vector<VkImageView> m_swapChainImageViews;
    //    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    //    // graphics pipeline
    //    VkRenderPass m_renderPass;
    //    VkDescriptorSetLayout m_descriptorSetLayout;
    //    VkDescriptorPool m_descriptorPool;
    //    std::vector<VkDescriptorSet> m_descriptorSets;
    //    VkPipelineLayout m_pipelineLayout;
    //    VkPipeline m_graphicsPipeline;
    //    // commands
    //    VkCommandPool m_graphicsCommandPool, m_transferCommandPool;
    //    std::vector<VkCommandBuffer> m_graphicsCommandBuffers, m_setupCommandBuffers;
    //    // sync objects
    //    // GPU -> GPU
    //    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    //    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    //    // CPU -> GPU
    //    std::vector<VkFence> m_inFlightFences;
    //    size_t m_currentFrame = 0;
    //    // buffers
    //    VkBuffer m_vertexBuffer;
    //    VkDeviceMemory m_vertexBufferMemory;
    //    VkBuffer m_indexBuffer;
    //    VkDeviceMemory m_indexBufferMemory;
    //    std::vector<VkBuffer> m_uniformBuffers;
    //    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    //    // texture
    //    uint32_t m_mipLevels;
    //    VkImage m_textureImage;
    //    VkDeviceMemory m_textureImageMemory;
    //    VkImageView m_textureImageView;
    //    VkSampler m_textureSampler;
    //// model
    Model m_model;
    //    // depth
    //    VkImage m_depthImage;
    //    VkDeviceMemory m_depthImageMemory;
    //    VkImageView m_depthImageView;
    //    // mulit-sampling
    //    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;
    //    VkImage m_colorImage;
    //    VkDeviceMemory m_colorImageMemory;
    //    VkImageView m_colorImageView;
};

#endif  // !GUPPY_H