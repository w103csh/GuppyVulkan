
#ifndef SCENE_H
#define SCENE_H

#include <algorithm>
#include <atomic>
#include <array>
#include <future>
#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "Mesh.h"
#include "MyShell.h"

class Scene {
   public:
    Scene() = delete;
    Scene(const MyShell::Context &ctx, const Game::Settings &settings,
          const VkDescriptorBufferInfo &ubo_info, const ShaderResources &vs, const ShaderResources &fs,
          const VkPipelineCache &cache);

    void addMesh(const MyShell::Context &ctx, const VkDescriptorBufferInfo &ubo_info,
                 std::unique_ptr<Mesh> pMesh);

    void removeMesh(Mesh *mesh);

    inline const VkRenderPass activeRenderPass() const { return render_passes_[0]; }

    void recordDrawCmds(const MyShell::Context &ctx, const std::vector<FrameData> &frame_data,
                        const std::vector<VkFramebuffer> &framebuffers, const VkViewport &viewport, const VkRect2D &scissor);

    bool update(const MyShell::Context &ctx, const Game::Settings &settings, const VkDescriptorBufferInfo &ubo_info,
                const ShaderResources &vs, const ShaderResources &fs, const VkPipelineCache &cache);

    const VkCommandBuffer &getDrawCmds(const uint32_t &frame_data_index);

    // TODO: there must be a better way...
    inline size_t readyCount() const {
        size_t count = 0;
        for (auto &pMesh : meshes_)
            if (pMesh->isReady()) count++;
        return count;
        // return std::count_if(meshes_.begin(), meshes_.end(), [](auto &mesh) { return mesh->isReady(); });
    }

    void destroy(const VkDevice &dev);

   private:
    // Just the two types for now...
    enum class PIPELINE_TYPE { BASE = 0, POLY, LINE };

    inline VkPipeline &base_pipeline() { return pipelines_[static_cast<int>(PIPELINE_TYPE::BASE)]; }

    // Descriptor
    void create_descriptor_set_layout(const MyShell::Context &ctx);
    void destroy_descriptor_set_layouts(const VkDevice &dev);
    void create_descriptor_pool(const MyShell::Context &ctx);
    void destroy_descriptor_pool(const VkDevice &dev);

    VkDescriptorSetLayout desc_set_layout_;
    VkDescriptorPool desc_pool_;

    // Pipeline
    void create_pipeline_layout(const VkDevice &dev, VkPipelineLayout &layout);
    void destroy_pipeline_layouts(const VkDevice &dev);
    void create_base_pipeline(const MyShell::Context &ctx, const Game::Settings &settings, const ShaderResources &vs,
                              const ShaderResources &fs, const VkPipelineCache &cache);
    void create_pipelines(const VkDevice &dev, const Game::Settings &settings, const ShaderResources &vs, const ShaderResources &fs,
                          const VkPipelineCache &cache);
    void destroy_pipelines(const VkDevice &dev, bool destroy_base = false);

    VkGraphicsPipelineCreateInfo pipeline_info_;
    VkPipelineLayout layout_;
    // One for each enum for now (3)...
    std::array<VkPipeline, 3> pipelines_;

    // Render pass
    void create_render_pass(const MyShell::Context &ctx, const Game::Settings &settings, bool clear = true,
                            VkImageLayout finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    void destroy_render_passes(const VkDevice &dev);
    std::vector<VkRenderPass> render_passes_;

    // Draw commands
    void create_draw_cmds(const MyShell::Context &ctx);
    void destroy_cmds(const VkDevice &dev);
    void record(const MyShell::Context &ctx, const VkCommandBuffer &cmd, const VkFramebuffer &framebuffer, const VkFence &fence,
                const VkViewport &viewport, const VkRect2D &scissor, size_t index);
    struct DrawResources {
        std::thread thread;
        VkCommandBuffer cmd;
    };
    std::vector<DrawResources> draw_resources_;

    uint32_t tex_count_;
    std::vector<std::future<Mesh *>> loading_futures_;
    std::vector<std::unique_ptr<Mesh>> meshes_;
};

#endif  // !SCENE_H