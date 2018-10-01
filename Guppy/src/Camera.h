
#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace Camera {

struct Camera {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 clip;
    glm::mat4 mvp;
    VkDeviceSize memory_requirements_size; // TODO: ugh!
    float aspect;
};

void default_orthographic(Camera& camera, float width, float height, float fov = 45.0f);
void default_perspective(Camera& camera, float width, float height, float fov = glm::radians(45.0f));
glm::vec3 get_direction(const Camera& camera);
glm::vec3 get_position(const Camera& camera);
void update_posistion(Camera& camera, const glm::vec3& dir);

}  // namespace Camera

#endif  // !CAMERA_H