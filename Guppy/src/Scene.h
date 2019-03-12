
#ifndef SCENE_H
#define SCENE_H

#include <array>
#include <future>
#include <vector>
#include <vulkan/vulkan.h>

#include "Handlee.h"
#include "Helpers.h"
#include "Mesh.h"
#include "SelectionManager.h"

// clang-format off
namespace RenderPass    { class Base; }
// clang-format on

namespace Scene {

class Handler;

class Base : public Handlee<Scene::Handler> {
    friend class Handler;

   public:
    Base(Scene::Handler &handler, size_t offset, bool makeFaceSelection);
    virtual ~Base();

    inline size_t getOffset() { return offset_; }

    void record(const uint8_t &frameIndex, std::unique_ptr<RenderPass::Base> &pPass);

    // SELECTION
    inline const std::unique_ptr<Face> &getFaceSelection() { return pSelectionManager_->getFace(); }
    void select(const Ray &ray);

    void destroy();

   private:
    size_t offset_;

    // Selection
    std::unique_ptr<Selection::Manager> pSelectionManager_;
};

}  // namespace Scene

#endif  // !SCENE_H