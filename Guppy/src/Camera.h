
#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Helpers.h"

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

    glm::vec3 getDirection() override;
    glm::vec3 getPosition() override;

    inline glm::mat4 getMVP() const { return clip_ * proj_ * view_ * Object3d::obj3d_.model; }

    void update(float aspect, const glm::vec3 &pos_dir = {}, const glm::vec3 &look_dir = {});

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
    glm::vec3 eye_;
    glm::vec3 center_;
    glm::mat4 view_;
};

#endif  // !CAMERA_H