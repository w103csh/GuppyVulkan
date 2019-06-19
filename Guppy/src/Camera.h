
#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "BufferItem.h"
#include "Helpers.h"
#include "Obj3d.h"
#include "Uniform.h"

namespace Camera {
namespace Default {

namespace Perspective {

struct CreateInfo : public Buffer::CreateInfo {
    float aspect = (16.0f / 9.0f);
    glm::vec3 eye{2.0f, 2.0f, 4.0f};
    glm::vec3 center{0.0f, 0.0f, 0.0f};
    float fov = glm::radians(45.0f);
    float n = 0.1f;
    float f = 1000.0f;
};

struct DATA {
    // View matrix created from glm::lookAt
    glm::mat4 view = glm::mat4(1.0f);
    // This is actually clip_ * proj_
    glm::mat4 projection = glm::mat4(1.0f);
    // World space to view space
    glm::mat4 viewProjection = glm::mat4(1.0f);
    glm::vec3 worldPosition;
    // rem 4
};

class Base : public Obj3d, public Uniform::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo);

    inline glm::vec3 getCameraSpaceDirection(const glm::vec3 &d = FORWARD_VECTOR) const {
        // TODO: deal with model_...
        glm::vec3 direction = pData_->view * glm::vec4(d, 0.0f);
        return glm::normalize(direction);
    }

    inline glm::vec3 getCameraSpacePosition(const glm::vec3 &p = {}) const {
        // TODO: deal with model_...
        return pData_->view * glm::vec4(p, 1.0f);
    }

    inline glm::vec3 getWorldSpaceDirection(const glm::vec3 &d = FORWARD_VECTOR, uint32_t index = 0) const override {
        // TODO: deal with model_...
        glm::vec3 direction = glm::inverse(pData_->view) * glm::vec4(d, 0.0f);
        return glm::normalize(direction);
    }

    inline glm::vec3 getWorldSpacePosition(const glm::vec3 &p = {}, uint32_t index = 0) const override {
        // TODO: deal with model_...
        return glm::inverse(pData_->view) * glm::vec4(p, 1.0f);
    }

    Ray getRay(glm::vec2 &&position, const VkExtent2D &extent) {
        return getRay(std::forward<glm::vec2>(position), extent, far_);
    }
    Ray getRay(glm::vec2 &&position, const VkExtent2D &extent, float distance);

    inline const glm::mat4 getClip() { return clip_; }

    void setAspect(float aspect);
    void update(const glm::vec3 &pos_dir = {}, const glm::vec3 &look_dir = {});

   private:
    inline glm::mat4 getMVP() const { return pData_->projection * getMV(); }
    inline glm::mat4 getMV() const { return pData_->view * model_; }
    bool updateView(const glm::vec3 &pos_dir, const glm::vec3 &look_dir);

    // This should be the only way to set the projection data. Also,
    // this is not actually the projection matrix!!!
    inline void setProjectionData() { pData_->projection = clip_ * proj_; }

    inline void setData() override {
        pData_->view = getMV();
        pData_->viewProjection = getMVP();
        pData_->worldPosition = getWorldSpacePosition();
        DIRTY = true;
    }

    inline const glm::mat4 &model(uint32_t index = 0) const override { return model_; }
    glm::mat4 model_;

    glm::mat4 clip_;
    // projection
    float aspect_;
    float far_;
    float fov_;
    float near_;
    glm::mat4 proj_;
    // view
    // storing eye & center make the matrix creation faster?
    glm::vec3 eye_;
    glm::vec3 center_;
};

}  // namespace Perspective
}  // namespace Default
}  // namespace Camera

#endif  // !CAMERA_H