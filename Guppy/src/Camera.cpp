
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "Constants.h"
#include "Helpers.h"

Camera::Camera(const glm::vec3 &eye, const glm::vec3 &center, float aspect, float fov, float n, float f)
    : position_(eye), aspect_(aspect), far_(f), fov_(fov), near_(n), model_(1.0f) {
    view_ = glm::lookAt(eye, center, UP_VECTOR);
    proj_ = glm::perspective(fov, aspect, near_, far_);
    // Vulkan clip space has inverted Y and half Z.
    clip_ = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,   //
                      0.0f, -1.0f, 0.0f, 0.0f,  //
                      0.0f, 0.0f, 0.5f, 0.0f,   //
                      0.0f, 0.0f, 0.5f, 1.0f);  //
}

glm::vec3 Camera::getDirection() {
    glm::vec3 dir = glm::row(view_, 2) * -1.0f;
    return glm::normalize(dir);
}

glm::vec3 Camera::getPosition() {
    //// TODO: never looked at or thought about this. got it off the web
    //{
    //    glm::mat3 mat3(camera.model);
    //    glm::vec3 d(camera.model[3]);

    //    glm::vec3 retVec = -d * mat3;
    //    return retVec;
    //}
    return glm::vec3();
}

void Camera::update(float aspect) {
    if (!helpers::almost_equal(aspect, aspect_, 2)) {
        proj_ = glm::perspective(fov_, aspect, near_, far_);
    }
}

void Camera::update(float aspect,const  glm::vec3 &pos_dir) {
    update(aspect);
    if (!glm::all(glm::equal(pos_dir, glm::vec3()))) {
        updatePosition(pos_dir);
    }
}

void Camera::updatePosition(const glm::vec3 & pos_dir) {
    auto forward = getDirection();
    auto right = glm::normalize(glm::cross(forward, UP_VECTOR));  // TODO: handle looking straight up
    auto up = glm::normalize(glm::cross(right, forward));

    auto movement = forward * pos_dir.z;
    movement += right * pos_dir.x;
    movement += up * pos_dir.y;

    model_ = glm::translate(model_, movement);
}
