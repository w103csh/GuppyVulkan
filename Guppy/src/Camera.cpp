
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "Constants.h"

void Camera::default_orthographic(Camera &camera, float width, float height, float fov) {
    // TODO
}

void Camera::default_perspective(Camera &camera, float width, float height, float fov) {
    if (width > height) fov *= height / width;

    camera.proj = glm::perspective(fov,             // fov
                                   width / height,  // aspect
                                   0.1f,            // near
                                   100.0f);         // far

    camera.view = glm::lookAt(glm::vec3(-2, 2, 4),  // Camera is at (-5,3,-10), in World Space
                              glm::vec3(0, 0, 0),     // and looks at the origin
                              UP_VECTOR);             // Head is up (set to 0,-1,0 to look upside-down)

    camera.model = glm::mat4(1.0f);

    // Vulkan clip space has inverted Y and half Z.
    camera.clip = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.5f, 1.0f);

    camera.mvp = camera.clip * camera.proj * camera.view * camera.model;
}

glm::vec3 Camera::get_direction(const Camera &camera) {
    glm::vec4 out(0.0f, 0.0f, -1.0f, 1.0f);
    if (UP_VECTOR.z == 1) {
        // I did not test this.
        out.z = 0.0f;
        out.y = 1.0f;
    }
    glm::vec3 dir = camera.view * out;
    return glm::normalize(dir);
}

glm::vec3 Camera::get_position(const Camera &camera) {
    // TODO: never looked at or thought about this. got it off the web
    {
        glm::mat3 mat3(camera.model);
        glm::vec3 d(camera.model[3]);

        glm::vec3 retVec = -d * mat3;
        return retVec;
    }
}

void Camera::update_posistion(Camera &camera, const glm::vec3 &dir) {
    if (!(dir.x == 0 && dir.y == 0 && dir.z == 0)) {
        auto forward = get_direction(camera);
        auto right = glm::normalize(glm::cross(forward, UP_VECTOR));  // TODO: handle looking straight up
        auto up = glm::normalize(glm::cross(right, forward));

        auto d = forward * dir.z;
        d += right * dir.x;
        d += up * dir.y;

        camera.view *= glm::translate(glm::mat4(1.0f), d);
    }
}