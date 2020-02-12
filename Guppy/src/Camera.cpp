/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

#include "Camera.h"
#include "ConstantsAll.h"
#include "InputHandler.h"

namespace Camera {
namespace Perspective {

namespace Default {

Base::Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM::CAMERA_PERSPECTIVE_DEFAULT),
      Buffer::PerFramebufferDataItem<DATA>(pData),
      model_{1.0f},
      aspect_(pCreateInfo->aspect),
      far_(pCreateInfo->f),
      fov_(pCreateInfo->fov),
      near_(pCreateInfo->n),
      eye_(pCreateInfo->eye),
      center_(pCreateInfo->center) {
    data_.view = glm::lookAt(pCreateInfo->eye, pCreateInfo->center, UP_VECTOR);
    proj_ = glm::perspective(pCreateInfo->fov, pCreateInfo->aspect, near_, far_);
    setProjectionData();
    update();
}

Ray Base::getRay(glm::vec2 &&position, const vk::Extent2D &extent, float distance) {
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
        data_.view,           // model (TODO: add model_ to this?)
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

void Base::setAspect(float aspect) {
    aspect_ = aspect;
    proj_ = glm::perspective(fov_, aspect_, near_, far_);
    setProjectionData();
    update();
}

void Base::update(const glm::vec3 &pos_dir, const glm::vec3 &look_dir, const uint32_t frameIndex) {
    // bool wasUpdate = updateView(pos_dir, look_dir);  // This is triple buffered so just update always for now.
    updateView(pos_dir, look_dir);
    update(frameIndex);
}

void Base::update(const uint32_t frameIndex) {
    data_.view = data_.view * model_;
    data_.viewProjection = data_.projection * data_.view;
    data_.worldPosition = getWorldSpacePosition();
    setData(frameIndex);
}

bool Base::updateView(const glm::vec3 &pos_dir, const glm::vec3 &look_dir) {
    bool update_pos = !glm::all(glm::equal(pos_dir, glm::vec3()));
    bool update_look = !glm::all(glm::equal(look_dir, glm::vec3()));
    // If there is nothing to update then return ...
    if (!update_pos && !update_look) return false;

    // Get othonormal basis for camera view ...
    glm::vec3 w = glm::row(data_.view, 2);
    glm::vec3 u = glm::row(data_.view, 0);  // TODO: handle looking straight up
    glm::vec3 v = glm::row(data_.view, 1);

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

    data_.view = glm::lookAt(eye_, center_, UP_VECTOR);

    return true;
}

}  // namespace Default

namespace CubeMap {

Base::Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM::LIGHT_CUBE_SHADOW),
      Buffer::PerFramebufferDataItem<DATA>(pData),
      near_(pCreateInfo->n),
      far_(pCreateInfo->f) {
    model_ = glm::translate(glm::mat4(1.0f), pCreateInfo->position);
    data_.proj = glm::perspective(glm::half_pi<float>(), 1.0f, near_, far_);
    setViews();
    setData();
}

void Base::setViews() {
    const glm::vec3 position = getWorldSpacePosition();
    // cube map view transforms
    data_.views[0] = glm::lookAt(position, position + CARDINAL_X, -CARDINAL_Y);
    data_.views[1] = glm::lookAt(position, position - CARDINAL_X, -CARDINAL_Y);
    data_.views[2] = glm::lookAt(position, position + CARDINAL_Y, CARDINAL_Z);
    data_.views[3] = glm::lookAt(position, position - CARDINAL_Y, -CARDINAL_Z);
    data_.views[4] = glm::lookAt(position, position + CARDINAL_Z, -CARDINAL_Y);
    data_.views[5] = glm::lookAt(position, position - CARDINAL_Z, -CARDINAL_Y);
}

}  // namespace CubeMap

}  // namespace Perspective
}  // namespace Camera
