
#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "Object3d.h"
#include "Uniform.h"

namespace Camera {
namespace Default {

namespace Perspective {

struct CreateInfo {
    float aspect = (16.0f / 9.0f);
    const glm::vec3 eye{2.0f, 2.0f, 4.0f};
    const glm::vec3 center{0.0f, 0.0f, 0.0f};
    float fov = glm::radians(45.0f);
    float n = 0.1f;
    float f = 1000.0f;
};

struct DATA {
    glm::mat4 viewProjection = glm::mat4(1.0f);
    // World space to view space
    glm::mat4 view = glm::mat4(1.0f);  // view matrix created from glm::lookAt
};

class Base : public Object3d, public Uniform::Base, public Buffer::DataItem<DATA> {
   public:
    Base(const Buffer::Info &&info, DATA *pData, CreateInfo *pCreateInfo);

    inline glm::vec3 getCameraSpaceDirection(const glm::vec3 &d = FORWARD_VECTOR) const {
        // TODO: deal with model_...
        glm::vec3 direction = pData_->view * glm::vec4(d, 0.0f);
        return glm::normalize(direction);
    }

    inline glm::vec3 getCameraSpacePosition(const glm::vec3 &p = {}) const {
        // TODO: deal with model_...
        return pData_->view * glm::vec4(p, 1.0f);
    }

    inline glm::vec3 getWorldSpaceDirection(const glm::vec3 &d = FORWARD_VECTOR) const override {
        // TODO: deal with model_...
        glm::vec3 direction = glm::inverse(pData_->view) * glm::vec4(d, 0.0f);
        return glm::normalize(direction);
    }

    inline glm::vec3 getWorldSpacePosition(const glm::vec3 &p = {}) const override {
        // TODO: deal with model_...
        return glm::inverse(pData_->view) * glm::vec4(p, 1.0f);
    }

    Ray getRay(glm::vec2 &&position, const VkExtent2D &extent) {
        return getRay(std::forward<glm::vec2>(position), extent, far_);
    }
    Ray getRay(glm::vec2 &&position, const VkExtent2D &extent, float distance);

    void setAspect(float aspect);
    void update(const glm::vec3 &pos_dir = {}, const glm::vec3 &look_dir = {});

   private:
    inline glm::mat4 getMVP() const { return clip_ * proj_ * getMV(); }
    inline glm::mat4 getMV() const { return pData_->view * model_; }
    bool updateView(const glm::vec3 &pos_dir, const glm::vec3 &look_dir);

    inline void setData() override {
        pData_->view = getMV();
        pData_->viewProjection = getMVP();
        DIRTY = true;
    }

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