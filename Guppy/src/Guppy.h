
#ifndef GUPPY_H
#define GUPPY_H

#include <vector>
#include <set>

#include "Camera.h"
#include "Game.h"
#include "Helpers.h"
#include "Model.h"
#include "MyShell.h"
#include "Texture.h"

class Guppy : public Game {
   public:
    Guppy(const std::vector<std::string>& args);
    ~Guppy();

    void attach_shell(MyShell& sh);
    void detach_shell();

    void attach_swapchain();
    void detach_swapchain();

    void on_key(Key key);
    void on_tick();

    void on_frame(float frame_pred);

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

    bool sim_paused_;
    bool sim_fade_;
    //Simulation sim_;

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
    //std::vector<VkImage> images_; // replaced by above
    //std::vector<VkImageView> image_views_; // replaced by above
    std::vector<VkFramebuffer> framebuffers_;

    // called by attach_shell
    void create_render_pass(bool include_depth, bool include_color, bool clear = true,
                            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    void create_shader_modules();
    void create_descriptor_set_layout(const VkDescriptorSetLayoutCreateFlags descSetLayoutCreateFlags = 0);
    void create_pipeline_layout();
    void create_pipelines();

    void create_frame_data(int count);
    void destroy_frame_data();
    void create_fences();
    // void create_buffers();
    // void create_buffer_memory();
    void create_descriptor_sets(bool use_texture = true);  // *

    // called mostly by on_key
    void update_camera(bool fix_me = true);

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

    VkPipeline pipeline_line_;

    // drawing
    std::vector<VkCommandBuffer> draw_cmds_;

    Camera::Camera camera_;
    bool sample_shading_supported_ = false;
    VkSampleCountFlagBits num_samples_;

    BufferResource vertex_res_;
    BufferResource index_res_;
    std::vector<VkDescriptorSet> desc_sets_;
    std::vector<Texture::TextureData> textures_;

    void copyBuffer(VkCommandBuffer& cmd, const VkBuffer& src_buf, const VkBuffer& dst_buf, const VkDeviceSize& size);
    void create_input_assembly_data();
    void create_vertex_data(StagingBufferResource& stg_res);
    void create_index_data(StagingBufferResource& stg_res);

    void create_draw_cmds();
    void create_model();
    void determine_sample_count(const MyShell::PhysicalDeviceProperties& props);
    void destroy_textures();
    void destroy_descriptor_and_pipeline_layouts();
    void create_pipeline_cache();
    void destroy_pipeline_cache();
    void destroy_pipelines();

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
    void destroy_descriptor_pool();
    void destroy_shader_modules();
    void destroy_render_pass();
    void destroy_uniform_buffer();
    void destroy_input_assembly_data();
};

#endif  // !GUPPY_H