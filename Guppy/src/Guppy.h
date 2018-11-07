
#ifndef GUPPY_H
#define GUPPY_H

#include <vector>

#include "Camera.h"
#include "Light.h"
#include "Game.h"
#include "Helpers.h"
#include "MyShell.h"
#include "Scene.h"
#include "Texture.h"
#include "Shader.h"

class Guppy : public Game {
   public:
    Guppy(const std::vector<std::string>& args);
    ~Guppy();

    void attach_shell(MyShell& sh);
    void detach_shell();

    void attach_swapchain();
    void detach_swapchain();

    void on_key(KEY key);
    void on_tick();

    void on_frame(float frame_pred);

   private:
    bool multithread_;
    bool use_push_constants_;

    bool sim_paused_;
    bool sim_fade_;
    // Simulation sim_;

    VkPhysicalDevice physical_dev_;
    VkDevice dev_;
    VkQueue queue_;
    uint32_t queue_family_;
    VkFormat format_;
    VkDeviceSize aligned_object_data_size;

    VkPhysicalDeviceProperties physical_dev_props_;
    std::vector<VkMemoryPropertyFlags> mem_flags_;

    VkCommandPool primary_cmd_pool_;
    std::vector<VkCommandPool> worker_cmd_pools_;
    VkDescriptorPool desc_pool_;
    VkDeviceMemory frame_data_mem_;
    std::vector<FrameData> frame_data_;
    int frame_data_index_;
    const auto& frameData() const { return frame_data_; }

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
    std::vector<VkFramebuffer> framebuffers_;

    void create_frame_data(int count);
    void destroy_frame_data();
    void create_fences();

    // called by attach_shell

    // called by attach_swapchain
    void prepare_viewport();
    void prepare_framebuffers(const VkSwapchainKHR& swapchain);

    // *********************************************

    // called by attach_shell
    void createUniformBuffer(std::string markName = "");
    void createLights();
    void createTextures();
    void createScenes();

    // called by detach_shell
    void destroyUniformBuffer();
    void destroyTextures();

    // called by attach_swapchain
    void get_swapchain_image_data(const VkSwapchainKHR& swapchain);

    // called by detach_swapchain

    // called by create_frame_data
    void create_color_resources();
    void create_depth_resources();

    // called by destroy_frame_data
    void destroy_color_resources();
    void destroy_depth_resources();

    // Scene
    inline const std::unique_ptr<Scene>& active_scene() const { return scenes_[active_scene_index_]; }

    // textures
    void addTexture(const VkDevice& dev, std::string path, std::string normPath = "", std::string specPath = "");
    std::shared_ptr<Texture::Data> getTextureByPath(std::string path);
    void updateTextures(const VkDevice& dev);
    float test = 1.0;

    // uniform buffer
    void updateUniformBuffer();
    void copyUniformBufferMemory();

    // frame buffer
    ImageResource color_resource_;
    ImageResource depth_resource_;

    // uniform buffer
    DefaultUniformBuffer defUBO_;
    UniformBufferResources UBOResource_;
    // camera
    Camera camera_;
    // lights
    bool showLightHelpers_ = true;
    std::vector<Light::Positional> positionalLights_;
    size_t lightHelperOffset_;

    // scene
    int active_scene_index_;
    std::vector<std::unique_ptr<Scene>> scenes_;

    // textures
    std::vector<std::shared_ptr<Texture::Data>> textures_;
    std::vector<std::future<std::shared_ptr<Texture::Data>>> texFutures_;
};

#endif  // !GUPPY_H