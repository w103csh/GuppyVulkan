
#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "Object3d.h"

class Camera : public Object3d {
   public:
    struct Data {
        glm::mat4 viewProjection;
        glm::mat4 view;
    };

    Camera(const glm::vec3 &eye, const glm::vec3 &center, float aspect, float fov = glm::radians(45.0f), float n = 0.1f,
           float f = 1000.0f);

    inline const Data *getData() {
        if (dirty_) {
            data_.view = getMV();
            data_.viewProjection = getMVP();
        }
        return &data_;
    }

    inline glm::vec3 getCameraSpaceDirection(const glm::vec3 &d = FORWARD_VECTOR) const {
        // TODO: deal with model_...
        glm::vec3 direction = view_ * glm::vec4(d, 0.0f);
        return glm::normalize(direction);
    }

    inline glm::vec3 getCameraSpacePosition(const glm::vec3 &p = {}) const {
        // TODO: deal with model_...
        return view_ * glm::vec4(p, 1.0f);
    }

    inline glm::vec3 getWorldSpaceDirection(const glm::vec3 &d = FORWARD_VECTOR) const override {
        // TODO: deal with model_...
        glm::vec3 direction = glm::inverse(view_) * glm::vec4(d, 0.0f);
        return glm::normalize(direction);
    }

    inline glm::vec3 getWorldSpacePosition(const glm::vec3 &p = {}) const override {
        // TODO: deal with model_...
        return glm::inverse(view_) * glm::vec4(p, 1.0f);
    }

    Ray getPickRay(glm::vec2 &&position, const VkExtent2D &extent) {
        return getPickRay(std::forward<glm::vec2>(position), extent, far_);
    }
    Ray getPickRay(glm::vec2 &&position, const VkExtent2D &extent, float distance);

    void update(const float aspect, const glm::vec3 &pos_dir = {}, const glm::vec3 &look_dir = {});

   private:
    inline glm::mat4 getMVP() const { return clip_ * proj_ * getMV(); }
    inline glm::mat4 getMV() const { return view_ * model_; }
    bool updateView(const glm::vec3 &pos_dir, const glm::vec3 &look_dir);

    bool dirty_;
    Data data_;

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
    // World space to view space
    glm::mat4 view_;  // view matrix created from glm::lookAt
};

#endif  // !CAMERA_H