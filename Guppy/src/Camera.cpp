
#include <glm/gtc/matrix_access.hpp>

#include "Camera.h"
#include "Constants.h"
#include "InputHandler.h"

Camera::Camera(const glm::vec3 &eye, const glm::vec3 &center, float aspect, float fov, float n, float f)
    : Object3d(glm::mat4(1.0f)),
      dirty_(true),
      data_({glm::mat4(1.0f), glm::mat4(1.0f)}),
      aspect_(aspect),
      center_(center),
      eye_(eye),
      far_(f),
      fov_(fov),
      near_(n) {
    view_ = glm::lookAt(eye, center, UP_VECTOR);
    proj_ = glm::perspective(fov, aspect, near_, far_);
    // Vulkan clip space has inverted Y and half Z.
    clip_ = glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,   //
                      0.0f, -1.0f, 0.0f, 0.0f,  //
                      0.0f, 0.0f, 0.5f, 0.0f,   //
                      0.0f, 0.0f, 0.5f, 1.0f);  //
}

Ray Camera::getRay(glm::vec2 &&position, const VkExtent2D &extent, float distance) {
    /*  Viewport appears to take x, y, w, h where:
            w, y - lower left
            w, h - width and height

        This means that incoming position will need the y-value
        inverted (Both windows & glfw).
    */
    glm::vec4 viewport(0.0f, 0.0f, extent.width, extent.height);

    glm::vec3 win = {position.x, extent.height - position.y, 0.0f};

    // It looks like this returns a point in world space.
    auto d = glm::unProject(  //
        win,                  // win
        view_,                // model (TODO: add model_ to this?)
        proj_,                // projection
        viewport              // viewport
    );

    auto e = getWorldSpacePosition();

    // TODO: what really is "d" after unproject???
    auto directionUnit = glm::normalize(d - e);
    d = e + (directionUnit * distance);

    // TODO: I am pretty sure that we shouldn't be picking things between
    // the camera position and the near plane of the projection. I believe
    // the world space position returned here is wrong then...
    return {e, d, d - e, directionUnit};
}

void Camera::update(const float aspect, const glm::vec3 &pos_dir, const glm::vec3 &look_dir) {
    // ASPECT
    if (!helpers::almost_equal(aspect, aspect_, 2)) {
        proj_ = glm::perspective(fov_, aspect, near_, far_);
    }
    // VIEW
    dirty_ |= updateView(pos_dir, look_dir);
}

bool Camera::updateView(const glm::vec3 &pos_dir, const glm::vec3 &look_dir) {
    bool update_pos = !glm::all(glm::equal(pos_dir, glm::vec3()));
    bool update_look = !glm::all(glm::equal(look_dir, glm::vec3()));
    // If there is nothing to update then return ...
    if (!update_pos && !update_look) return false;

    // Get othonormal basis for camera view ...
    glm::vec3 w = glm::row(view_, 2);
    glm::vec3 u = glm::row(view_, 0);  // TODO: handle looking straight up
    glm::vec3 v = glm::row(view_, 1);

    // MOVEMENT
    if (update_pos) {
        auto pos = w * (pos_dir.z * -1);  // w is pointing in -z direction
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

    return true;
}
