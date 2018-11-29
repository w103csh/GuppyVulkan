
#ifndef SCENE_H
#define SCENE_H

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
    Scene(const MyShell::Context &ctx, const Game::Settings &settings, const UniformBufferResources &uboResource,
          const std::vector<std::shared_ptr<Texture::Data>> &pTextures, size_t offset);

    std::unique_ptr<TextureMesh> &getTextureMesh(size_t index) { return texMeshes_[index]; }

    inline size_t getOffset() { return offset_; }

    // TODO: there is too much redundancy here...

    size_t moveMesh(const MyShell::Context &ctx, std::unique_ptr<ColorMesh> pMesh);
    size_t moveMesh(const MyShell::Context &ctx, std::unique_ptr<LineMesh> pMesh);
    size_t moveMesh(const MyShell::Context &ctx, std::unique_ptr<TextureMesh> pMesh) { return 0; };
    size_t addMesh(const MyShell::Context &ctx, std::unique_ptr<ColorMesh> pMesh, bool async = true,
                   std::function<void(Mesh *)> callback = nullptr);
    size_t addMesh(const MyShell::Context &ctx, std::unique_ptr<LineMesh> pMesh, bool async = true,
                   std::function<void(Mesh *)> callback = nullptr);
    size_t addMesh(const MyShell::Context &ctx, std::unique_ptr<TextureMesh> pMesh, bool async = true,
                   std::function<void(Mesh *)> callback = nullptr);

    void removeMesh(std::unique_ptr<Mesh> &pMesh);

    inline const VkRenderPass activeRenderPass() const { return plResources_.renderPass; }

    void recordDrawCmds(const MyShell::Context &ctx, const std::vector<FrameData> &frame_data,
                        const std::vector<VkFramebuffer> &framebuffers, const VkViewport &viewport, const VkRect2D &scissor);

    bool update(const MyShell::Context &ctx, int frameIndex);

    const VkCommandBuffer &getDrawCmds(const uint32_t &frame_data_index);

    // TODO: there must be a better way...
    inline size_t readyCount() {
        size_t count = 0;
        for (auto &pMesh : colorMeshes_)
            if (pMesh->getStatus() == STATUS::READY) count++;
        // for (auto &pMesh : lineMeshes_)
        //    if (pMesh->isReady()) count++;
        for (auto &pMesh : texMeshes_) {
            if (pMesh->getStatus() == STATUS::READY) count++;
        }
        return count;
        // return std::count_if(meshes_.begin(), meshes_.end(), [](auto &mesh) { return mesh->isReady(); });
    }

    void destroy(const VkDevice &dev);

    // TODO: not sure about these being here...
    inline const VkPipeline &getPipeline(PIPELINE_TYPE type) { return plResources_.pipelines[static_cast<int>(type)]; }
    void updatePipeline(PIPELINE_TYPE type);

   private:
    size_t offset_;

    // Descriptor
    void updateDescriptorResources(const MyShell::Context &ctx, std::unique_ptr<TextureMesh> &pMesh);
    inline bool isNewTexture(std::unique_ptr<TextureMesh> &pMesh) const {
        return pMesh->getTextureOffset() >= pDescResources_->texSets.size();
    }
    inline const VkDescriptorSet &getColorDescSet(int frameIndex) { return pDescResources_->colorSets[frameIndex]; }
    inline const VkDescriptorSet &getTexDescSet(size_t offset, int frameIndex) {
        return pDescResources_->texSets[offset][frameIndex];
    }
    std::unique_ptr<DescriptorResources> pDescResources_;

    // Pipeline
    PipelineResources plResources_;

    // Draw commands
    void create_draw_cmds(const MyShell::Context &ctx);
    void destroy_cmds(const VkDevice &dev);
    void record(const MyShell::Context &ctx, const VkCommandBuffer &cmd, const VkFramebuffer &framebuffer, const VkFence &fence,
                const VkViewport &viewport, const VkRect2D &scissor, size_t index, bool wait = false);
    std::vector<DrawResources> draw_resources_;

    // Meshes
    // color
    std::vector<std::unique_ptr<ColorMesh>> colorMeshes_;  // Vertex::TYPE::COLOR, PIPELINE_TYPE::TRI_LIST_COLOR
    std::vector<std::unique_ptr<ColorMesh>> lineMeshes_;   // Vertex::TYPE::COLOR, PIPELINE_TYPE::LINE
    // texture
    std::vector<std::unique_ptr<TextureMesh>> texMeshes_;  // Vertex::TYPE::TEX, PIPELINE_TYPE::TRI_LIST_TEX

    // Loading
    std::vector<std::future<Mesh *>> ldgFutures_;
    std::vector<std::unique_ptr<LoadingResources>> ldgResources_;

    // Uniform buffer
    void createDynamicTexUniformBuffer(const MyShell::Context &ctx, const Game::Settings &settings,
                                       const std::vector<std::shared_ptr<Texture::Data>> &textures, std::string markerName = "");
    void destroyUniforms(const VkDevice &dev);
    std::shared_ptr<UniformBufferResources> pDynUBOResource_;
};

#endif  // !SCENE_H