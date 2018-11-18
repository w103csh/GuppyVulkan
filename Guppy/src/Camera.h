
#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Object3d.h"

class Camera : public Object3d {
   public:
    struct Data {
        glm::mat4 mvp;
        alignas(16) glm::vec3 position;
        // rem 4
    };

    Camera(const glm::vec3 &eye, const glm::vec3 &center, float aspect, float fov = glm::radians(45.0f), float n = 0.1f,
           float f = 1000.0f);

    inline Data getData() { return data_; }

    inline glm::vec3 getWorldDirection(const glm::vec3 &d = FORWARD_VECTOR) const {
        glm::vec3 direction = glm::inverse(view_) * glm::vec4(d, 0.0f);
        return glm::normalize(direction);
    }

    inline glm::vec3 getWorldPosition(const glm::vec3 &p = {}) const {
        // TODO: deal with model...
        return glm::inverse(view_) * glm::vec4(p, 1.0f);
    }

    inline glm::mat4 getMVP() const { return clip_ * proj_ * view_ /** model_*/; }

    void update(const float aspect, const glm::vec3 &pos_dir = {}, const glm::vec3 &look_dir = {});

   private:
    void updateView(const glm::vec3 &pos_dir, const glm::vec3 &look_dir);

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