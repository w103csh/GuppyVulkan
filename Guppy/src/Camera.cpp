
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

#include "Camera.h"
#include "ConstantsAll.h"
#include "InputHandler.h"

Camera::Default::Perspective::Base::Base(const Buffer::Info &&info, DATA *pData, const Perspective::CreateInfo *pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Obj3d(),
      Uniform::Base(),
      Buffer::DataItem<DATA>(pData),
      model_{1.0f},
      aspect_(pCreateInfo->aspect),
      far_(pCreateInfo->f),
      fov_(pCreateInfo->fov),
      near_(pCreateInfo->n),
      eye_(pCreateInfo->eye),
      center_(pCreateInfo->center) {
    pData_->view = glm::lookAt(pCreateInfo->eye, pCreateInfo->center, UP_VECTOR);
    proj_ = glm::perspective(pCreateInfo->fov, pCreateInfo->aspect, near_, far_);
    // Vulkan clip space has inverted Y and half Z.
    clip_ = glm::mat4{1.0f, 0.0f,  0.0f, 0.0f,   //
                      0.0f, -1.0f, 0.0f, 0.0f,   //
                      0.0f, 0.0f,  0.5f, 0.0f,   //
                      0.0f, 0.0f,  0.5f, 1.0f};  //
    setProjectionData();
    setData();
}

Ray Camera::Default::Perspective::Base::getRay(glm::vec2 &&position, const VkExtent2D &extent, float distance) {
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
        pData_->view,         // model (TODO: add model_ to this?)
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

void Camera::Default::Perspective::Base::setAspect(float aspect) {
    aspect_ = aspect;
    proj_ = glm::perspective(fov_, aspect_, near_, far_);
    setProjectionData();
    setData();
}

void Camera::Default::Perspective::Base::update(const glm::vec3 &pos_dir, const glm::vec3 &look_dir) {
    if (updateView(pos_dir, look_dir)) setData();
}

bool Camera::Default::Perspective::Base::updateView(const glm::vec3 &pos_dir, const glm::vec3 &look_dir) {
    bool update_pos = !glm::all(glm::equal(pos_dir, glm::vec3()));
    bool update_look = !glm::all(glm::equal(look_dir, glm::vec3()));
    // If there is nothing to update then return ...
    if (!update_pos && !update_look) return false;

    // Get othonormal basis for camera view ...
    glm::vec3 w = glm::row(pData_->view, 2);
    glm::vec3 u = glm::row(pData_->view, 0);  // TODO: handle looking straight up
    glm::vec3 v = glm::row(pData_->view, 1);

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

    pData_->view = glm::lookAt(eye_, center_, UP_VECTOR);

    return true;
}
