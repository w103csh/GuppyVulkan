
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
    Scene(const MyShell::Context &ctx, const Game::Settings &settings, const UniformBufferResources &ubo_res);

    void addMesh(const MyShell::Context &ctx, const VkDescriptorBufferInfo &ubo_info, std::unique_ptr<ColorMesh> pMesh);
    // void addMesh(const MyShell::Context &ctx, const VkDescriptorBufferInfo &ubo_info, std::unique_ptr<LineMesh> pMesh);
    void addMesh(const MyShell::Context &ctx, const VkDescriptorBufferInfo &ubo_info, std::unique_ptr<TextureMesh> pMesh);

    void removeMesh(Mesh *mesh);

    inline const VkRenderPass activeRenderPass() const { return pipeline_resources_.renderPass; }

    void recordDrawCmds(const MyShell::Context &ctx, const std::vector<FrameData> &frame_data,
                        const std::vector<VkFramebuffer> &framebuffers, const VkViewport &viewport, const VkRect2D &scissor);

    bool update(const MyShell::Context &ctx, const VkDescriptorBufferInfo &ubo_info);

    const VkCommandBuffer &getDrawCmds(const uint32_t &frame_data_index);

    // TODO: there must be a better way...
    inline size_t readyCount() const {
        size_t count = 0;
        for (auto &pMesh : colorMeshes_)
            if (pMesh->isReady()) count++;
        // for (auto &pMesh : lineMeshes_)
        //    if (pMesh->isReady()) count++;
        for (auto &pMesh : texMeshes_)
            if (pMesh->isReady()) count++;
        return count;
        // return std::count_if(meshes_.begin(), meshes_.end(), [](auto &mesh) { return mesh->isReady(); });
    }

    void destroy(const VkDevice &dev);

    // TODO: not sure about this being here...
    inline const VkPipeline &getPipeline(PipelineHandler::TOPOLOGY topo) {
        return pipeline_resources_.pipelines[static_cast<int>(topo)];
    }

   private:
    // Descriptor
    void updateDescriptorResources(const MyShell::Context &ctx, const VkDescriptorBufferInfo &ubo_info);
    // void update_descriptor_sets(const MyShell::Context &ctx);

    inline const VkDescriptorSet &getSharedDescriptorSet(int frameIndex) {
        return desc_resources_.sharedSets[frameIndex];
    }
    void destroy_descriptor_resources(const VkDevice &dev);
    DescriptorResources desc_resources_;

    // TODO: not sure about this being here...
    void createSharedDescriptorSets(const MyShell::Context &ctx, const VkDescriptorBufferInfo &ubo_info);
    PipelineResources pipeline_resources_;

    // Draw commands
    void create_draw_cmds(const MyShell::Context &ctx);
    void destroy_cmds(const VkDevice &dev);
    void record(const MyShell::Context &ctx, const VkCommandBuffer &cmd, const VkFramebuffer &framebuffer, const VkFence &fence,
                const VkViewport &viewport, const VkRect2D &scissor, size_t index, bool wait = false);
    std::vector<DrawResources> draw_resources_;

    uint32_t tex_count_;
    std::vector<std::future<Mesh *>> loading_futures_;
    std::vector<std::unique_ptr<ColorMesh>> colorMeshes_;  // Vertex::TYPE::COLOR, PipelineHandler::TOPOLOGY::TRI_LIST_COLOR
    std::vector<std::unique_ptr<ColorMesh>> lineMeshes_;   // Vertex::TYPE::COLOR, PipelineHandler::TOPOLOGY::LINE
    std::vector<std::unique_ptr<TextureMesh>> texMeshes_;  // Vertex::TYPE::TEX, PipelineHandler::TOPOLOGY::TRI_LIST_TEX
};

#endif  // !SCENE_H