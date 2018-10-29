
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"
#include "Constants.h"
#include "Helpers.h"
#include "InputHandler.h"

Camera::Camera(const glm::vec3 &eye, const glm::vec3 &center, float aspect, float fov, float n, float f)
    : Object3d(glm::mat4(1.0f)), aspect_(aspect), center_(center), eye_(eye), far_(f), fov_(fov), near_(n) {
    view_ = glm::lookAt(eye, center, UP_VECTOR);
    proj_ = glm::perspective(fov, aspect, near_, far_);
    // Vulkan clip space has inverted Y and half Z.
    clip_ = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,   //
                      0.0f, -1.0f, 0.0f, 0.0f,  //
                      0.0f, 0.0f, 0.5f, 0.0f,   //
                      0.0f, 0.0f, 0.5f, 1.0f);  //
}

glm::vec3 Camera::getDirection() {
    auto local = Object3d::obj3d_.model * view_;
    glm::vec3 dir = glm::row(local, 2) * -1.0f;
    return glm::normalize(dir);
}

glm::vec3 Camera::getPosition() {
    auto local = Object3d::obj3d_.model * view_;
    // I believe this is the "w" homogenous factor from the book.
    // Still not 100% sure.
    auto w = local[3];
    glm::vec3 pos = -w * local;
    return pos;
}

void Camera::update(float aspect, const glm::vec3 &pos_dir, const glm::vec3 &look_dir) {
    // ASPECT
    if (!helpers::almost_equal(aspect, aspect_, 2)) {
        proj_ = glm::perspective(fov_, aspect, near_, far_);
    }
    // VIEW
    updateView(pos_dir, look_dir);
}

void Camera::updateView(const glm::vec3 &pos_dir, const glm::vec3 &look_dir) {
    bool update_pos = !glm::all(glm::equal(pos_dir, glm::vec3()));
    bool update_look = !glm::all(glm::equal(look_dir, glm::vec3()));
    // If there is nothing to update then return ...
    if (!update_pos && !update_look) return;

    // Get othonormal basis for camera view ...
    auto w = getDirection();
    auto u = glm::normalize(glm::cross(w, UP_VECTOR));  // TODO: handle looking straight up
    auto v = glm::normalize(glm::cross(u, w));

    // MOVEMENT
    if (update_pos) {
        auto pos = w * pos_dir.z;
        pos += u * pos_dir.x;
        pos += v * pos_dir.y;
        // update both eye_ & center_ so that movement doesn't affect look
        eye_ += pos;
        center_ += pos;
    }
    // LOOK
    if (update_look) {
        auto look = u * look_dir.x;
        look += v * look_dir.y;
        center_ += look;
    }

    view_ = glm::lookAt(eye_, center_, UP_VECTOR);
}
