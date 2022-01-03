/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>

#include "Camera.h"
#include "ConstantsAll.h"
#include "InputHandler.h"

namespace {
void setFrustumBox(Camera::FrustumInfo &info) {
    // Field of view is the total view angle with relation to the y-axis (glm::perspective).
    float tanHalfFovy = tan(info.fieldOfViewY / 2.0f);

    const float nearZ = info.nearDistance;  // near distance from eye
    const float nearDistToPlaneY = tanHalfFovy * nearZ;
    const float nearDistToPlaneX = nearDistToPlaneY * info.aspectRatio;

    const float farZ = info.farDistance;  // far distance from eye
    const float farDistToPlaneY = tanHalfFovy * farZ;
    const float farDistToPlaneX = farDistToPlaneY * info.aspectRatio;

    // Positions that bound the frustum
    info.box[0] = {-nearDistToPlaneX, nearDistToPlaneY, nearZ};   // nearTL
    info.box[1] = {nearDistToPlaneX, nearDistToPlaneY, nearZ};    // nearTR
    info.box[2] = {-nearDistToPlaneX, -nearDistToPlaneY, nearZ};  // nearBL
    info.box[3] = {nearDistToPlaneX, -nearDistToPlaneY, nearZ};   // nearBR
    info.box[4] = {-farDistToPlaneX, farDistToPlaneY, farZ};      // farTL
    info.box[5] = {farDistToPlaneX, farDistToPlaneY, farZ};       // farTR
    info.box[6] = {-farDistToPlaneX, -farDistToPlaneY, farZ};     // farBL
    info.box[7] = {farDistToPlaneX, -farDistToPlaneY, farZ};      // farBR
}
}  // namespace

namespace Camera {
namespace Perspective {

namespace Default {

Base::Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM::CAMERA_PERSPECTIVE_DEFAULT),
      Buffer::PerFramebufferDataItem<DATA>(pData),
      aspect_(pCreateInfo->aspect),
      near_(pCreateInfo->n),
      far_(pCreateInfo->f),
      fovy_(pCreateInfo->fovy),
      eye_(pCreateInfo->eye),
      center_(pCreateInfo->center) {
    data_.view = glm::lookAt(eye_, center_, UP_VECTOR);
    proj_ = glm::perspective(fovy_, aspect_, near_, far_);
    setProjectionData();
    update();
}

Ray Base::getRay(const glm::vec2 &position, const vk::Extent2D &extent, float distance) {
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
        data_.view,           // model (TODO: view is not a model matrix...)
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

frustumPlanes Base::getFrustumPlanes() const {
    assert(GLM_CONFIG_CLIP_CONTROL == GLM_CLIP_CONTROL_RH_NO);

    frustumPlanes planes;

    // Left clipping plane
    planes[0].x = data_.viewProjection[0][3] + data_.viewProjection[0][0];
    planes[0].y = data_.viewProjection[1][3] + data_.viewProjection[1][0];
    planes[0].z = data_.viewProjection[2][3] + data_.viewProjection[2][0];
    planes[0].w = data_.viewProjection[3][3] + data_.viewProjection[3][0];

    // Right clipping plane
    planes[1].x = data_.viewProjection[0][3] - data_.viewProjection[0][0];
    planes[1].y = data_.viewProjection[1][3] - data_.viewProjection[1][0];
    planes[1].z = data_.viewProjection[2][3] - data_.viewProjection[2][0];
    planes[1].w = data_.viewProjection[3][3] - data_.viewProjection[3][0];

    // Top clipping plane
    planes[2].x = data_.viewProjection[0][3] + data_.viewProjection[0][1];
    planes[2].y = data_.viewProjection[1][3] + data_.viewProjection[1][1];
    planes[2].z = data_.viewProjection[2][3] + data_.viewProjection[2][1];
    planes[2].w = data_.viewProjection[3][3] + data_.viewProjection[3][1];

    // Bottom clipping plane
    planes[3].x = data_.viewProjection[0][3] - data_.viewProjection[0][1];
    planes[3].y = data_.viewProjection[1][3] - data_.viewProjection[1][1];
    planes[3].z = data_.viewProjection[2][3] - data_.viewProjection[2][1];
    planes[3].w = data_.viewProjection[3][3] - data_.viewProjection[3][1];

    // Near clipping plane
    planes[4].x = data_.viewProjection[0][2];
    planes[4].y = data_.viewProjection[1][2];
    planes[4].z = data_.viewProjection[2][2];
    planes[4].w = data_.viewProjection[3][2];

    // Far clipping plane
    planes[5].x = data_.viewProjection[0][3] - data_.viewProjection[0][2];
    planes[5].y = data_.viewProjection[1][3] - data_.viewProjection[1][2];
    planes[5].z = data_.viewProjection[2][3] - data_.viewProjection[2][2];
    planes[5].w = data_.viewProjection[3][3] - data_.viewProjection[3][2];

    // Normalize the plane equations
    for (auto &p : planes) helpers::normalizePlane(p);

    return planes;
}

frustumPlanes Base::getFrustumPlanesZupLH() const {
    assert(GLM_CONFIG_CLIP_CONTROL == GLM_CLIP_CONTROL_RH_NO);

    frustumPlanes planes;

    // Left clipping plane
    planes[0].x = data_.viewProjection[0][3] + data_.viewProjection[0][0];
    planes[0].y = data_.viewProjection[2][3] + data_.viewProjection[2][0];
    planes[0].z = data_.viewProjection[1][3] + data_.viewProjection[1][0];
    planes[0].w = data_.viewProjection[3][3] + data_.viewProjection[3][0];
    // Right clipping plane
    planes[1].x = data_.viewProjection[0][3] - data_.viewProjection[0][0];
    planes[1].y = data_.viewProjection[2][3] - data_.viewProjection[2][0];
    planes[1].z = data_.viewProjection[1][3] - data_.viewProjection[1][0];
    planes[1].w = data_.viewProjection[3][3] - data_.viewProjection[3][0];
    // Top clipping plane
    planes[2].x = data_.viewProjection[0][3] + data_.viewProjection[0][1];
    planes[2].y = data_.viewProjection[2][3] + data_.viewProjection[2][1];
    planes[2].z = data_.viewProjection[1][3] + data_.viewProjection[1][1];
    planes[2].w = data_.viewProjection[3][3] + data_.viewProjection[3][1];
    // Bottom clipping plane
    planes[3].x = data_.viewProjection[0][3] - data_.viewProjection[0][1];
    planes[3].y = data_.viewProjection[2][3] - data_.viewProjection[2][1];
    planes[3].z = data_.viewProjection[1][3] - data_.viewProjection[1][1];
    planes[3].w = data_.viewProjection[3][3] - data_.viewProjection[3][1];
    // Near clipping plane
    planes[4].x = data_.viewProjection[0][2];
    planes[4].y = data_.viewProjection[2][2];
    planes[4].z = data_.viewProjection[1][2];
    planes[4].w = data_.viewProjection[3][2];
    // Far clipping plane
    planes[5].x = data_.viewProjection[0][3] - data_.viewProjection[0][2];
    planes[5].y = data_.viewProjection[2][3] - data_.viewProjection[2][2];
    planes[5].z = data_.viewProjection[1][3] - data_.viewProjection[1][2];
    planes[5].w = data_.viewProjection[3][3] - data_.viewProjection[3][2];

    // Normalize the plane equations
    for (auto &p : planes) helpers::normalizePlane(p);

    return planes;
}

FrustumInfo Base::getFrustumInfo() const {
    FrustumInfo info;
    info.fieldOfViewY = fovy_;
    info.aspectRatio = aspect_;
    info.nearDistance = near_;
    info.farDistance = far_;
    info.eye = eye_;
    info.planes = getFrustumPlanes();
    setFrustumBox(info);
    return info;
}

FrustumInfo Base::getFrustumInfoZupLH() const {
    FrustumInfo info;
    info.fieldOfViewY = fovy_;
    info.aspectRatio = aspect_;
    info.nearDistance = near_;
    info.farDistance = far_;
    info.eye = helpers::convertToZupLH(eye_);
    info.planes = getFrustumPlanesZupLH();
    setFrustumBox(info);
    for (auto it = info.box.begin(); it != info.box.end(); it++) *it = helpers::convertToZupLH(*it);
    return info;
}

void Base::setAspect(float aspect) {
    aspect_ = aspect;
    proj_ = glm::perspective(fovy_, aspect_, near_, far_);
    setProjectionData();
    update();
}

void Base::update(const glm::vec3 &moveDir, const glm::vec2 &lookDir, const uint32_t frameIndex) {
    updateView(moveDir, lookDir);
    update(frameIndex);
}

void Base::update(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 eye, const glm::vec3 center,
                  const uint32_t frameIndex) {
    data_.view = view;
    proj_ = proj;
    setProjectionData();
    eye_ = eye;
    center_ = center;
    update(frameIndex);
}

void Base::update(const uint32_t frameIndex) {
    data_.viewProjection = data_.projection * data_.view;
    data_.worldPosition = eye_;
    setData(frameIndex);
    // For now there is no model matrix. The camera is never transformed but is always reconstructed using glm::lookAt().
    // Currently, this is only used by the debug camera, which makes this an unecessary per frame update.
    model_ = helpers::moveAndRotateTo(eye_, center_, UP_VECTOR);
}

void Base::takeCameraData(const Base &camera) {
    eye_ = camera.getPosition();
    data_.view = camera.getMV();
    updateView({}, {});
}

bool Base::updateView(const glm::vec3 &moveDir, const glm::vec2 &lookDir) {
    // if (glm::all(glm::equal(moveDir, glm::vec3())) && glm::all(glm::equal(lookDir, glm::vec2()))) return false;

    // Get the orthonormal vectors from the previously calculated view matrix.
    const glm::vec3 f = {data_.view[0][2], data_.view[1][2], data_.view[2][2]};  // forward
    const glm::vec3 s = {data_.view[0][0], data_.view[1][0], data_.view[2][0]};  // right
    const glm::vec3 u = {data_.view[0][1], data_.view[1][1], data_.view[2][1]};  // up

    // MOVE
    eye_ += f * moveDir.z;
    eye_ += s * moveDir.x;
    eye_ += u * moveDir.y;

    // LOOK
    center_ = eye_ - f;
    center_ += s * lookDir.x;
    center_ += u * lookDir.y;

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

namespace Basic {

Base::Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM_DYNAMIC::CAMERA_PERSPECTIVE_BASIC),
      Buffer::PerFramebufferDataItem<DATA>(pData) {
    view_ = glm::lookAt(pCreateInfo->eye, pCreateInfo->center, UP_VECTOR);
    proj_ = glm::perspective(pCreateInfo->fovy, pCreateInfo->aspect, pCreateInfo->n, pCreateInfo->f);
    data_.viewProj = (VULKAN_CLIP_MAT4 * proj_) * view_;
    setData();
}

}  // namespace Basic

}  // namespace Perspective
}  // namespace Camera
