
#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

class Camera {
   public:
    Camera(const glm::vec3& eye, const glm::vec3& center, float aspect, float fov = glm::radians(45.0f), float n = 0.1f,
           float f = 1000.0f);

    glm::vec3 getDirection();
    glm::vec3 getPosition();

    inline glm::mat4 getMVP() const { return clip_ * proj_ * view_ * model_; }

    void update(float aspect);
    void update(float aspect, const glm::vec3 &pos_dir);

    VkDeviceSize memory_requirements_size;  // TODO: this ain't great

   private:
    void updatePosition(const glm::vec3 &dir);

    glm::vec3 position_;
    float aspect_;
    float far_;
    float fov_;
    float near_;
    glm::mat4 clip_;
    glm::mat4 model_;
    glm::mat4 proj_;
    glm::mat4 view_;
};

#endif  // !CAMERA_H