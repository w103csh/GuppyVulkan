
#ifndef SCENE_H
#define SCENE_H

#include <array>
#include <future>
#include <vector>
#include <vulkan/vulkan.h>

#include "Handlee.h"
#include "Helpers.h"
#include "Mesh.h"

// clang-format off
namespace RenderPass { class Base; }
// clang-format on

namespace Scene {

class Handler;

class Base : public Handlee<Scene::Handler> {
    friend class Handler;

   public:
    Base(const Scene::Handler &handler, size_t offset);
    virtual ~Base();

    inline size_t getOffset() { return offset_; }

    inline std::unique_ptr<ColorMesh> &getColorMesh(const size_t &index) { return colorMeshes_[index]; }
    inline std::unique_ptr<LineMesh> &getLineMesh(const size_t &index) { return lineMeshes_[index]; }
    inline std::unique_ptr<TextureMesh> &getTextureMesh(const size_t &index) { return texMeshes_[index]; }

    std::unique_ptr<ColorMesh> &moveMesh(std::unique_ptr<ColorMesh> pMesh);
    std::unique_ptr<LineMesh> &moveMesh(std::unique_ptr<LineMesh> pMesh);
    std::unique_ptr<TextureMesh> &moveMesh(std::unique_ptr<TextureMesh> pMesh);

    void removeMesh(std::unique_ptr<Mesh> &pMesh);

    void record(const uint8_t &frameIndex, std::unique_ptr<RenderPass::Base> &pPass);

    void update();

    void destroy();

   protected:
   private:
    size_t offset_;

    // MESH
    // COLOR
    std::vector<std::unique_ptr<ColorMesh>> colorMeshes_;
    std::vector<std::unique_ptr<LineMesh>> lineMeshes_;
    // TEXTURE
    std::vector<std::unique_ptr<TextureMesh>> texMeshes_;

    // LOADING
    std::vector<std::future<Mesh *>> ldgFutures_;
};

}  // namespace Scene

#endif  // !SCENE_H