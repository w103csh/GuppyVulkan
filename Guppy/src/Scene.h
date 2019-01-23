
#ifndef SCENE_H
#define SCENE_H

#include <array>
#include <future>
#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "Mesh.h"
#include "Shell.h"
#include "Uniform.h"

namespace RenderPass {
class Base;
}

class Scene {
    friend class SceneHandler;

   public:
    inline size_t getOffset() { return offset_; }

    std::unique_ptr<ColorMesh> &getColorMesh(size_t index) { return colorMeshes_[index]; }
    std::unique_ptr<LineMesh> &getLineMesh(size_t index) { return lineMeshes_[index]; }
    std::unique_ptr<TextureMesh> &getTextureMesh(size_t index) { return texMeshes_[index]; }

    std::unique_ptr<ColorMesh> &moveMesh(const Game::Settings &settings, const Shell::Context &ctx,
                                         std::unique_ptr<ColorMesh> pMesh);
    std::unique_ptr<LineMesh> &moveMesh(const Game::Settings &settings, const Shell::Context &ctx,
                                        std::unique_ptr<LineMesh> pMesh);
    std::unique_ptr<TextureMesh> &moveMesh(const Game::Settings &settings, const Shell::Context &ctx,
                                           std::unique_ptr<TextureMesh> pMesh);

    void removeMesh(std::unique_ptr<Mesh> &pMesh);

    void record(const Shell::Context &ctx, const uint8_t &frameIndex, std::unique_ptr<RenderPass::Base> &pPass);

    void update(const Game::Settings &settings, const Shell::Context &ctx);

    void destroy(const VkDevice &dev);

   private:
    Scene() = delete;
    Scene(size_t offset);

    size_t offset_;

    // Meshes
    // color
    std::vector<std::unique_ptr<ColorMesh>> colorMeshes_;
    std::vector<std::unique_ptr<LineMesh>> lineMeshes_;
    // texture
    std::vector<std::unique_ptr<TextureMesh>> texMeshes_;

    // Loading
    std::vector<std::future<Mesh *>> ldgFutures_;
};

#endif  // !SCENE_H