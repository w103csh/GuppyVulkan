/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <Common/Helpers.h>

#include "Constants.h"
#include "BufferItem.h"
#include "Obj3d.h"
#include "Uniform.h"

namespace Camera {

// Vulkan clip space has inverted Y and half Z.
constexpr glm::mat4 VULKAN_CLIP_MAT4 = glm::mat4{
    1.0f, 0.0f,  0.0f, 0.0f,  //
    0.0f, -1.0f, 0.0f, 0.0f,  //
    0.0f, 0.0f,  0.5f, 0.0f,  //
    0.0f, 0.0f,  0.5f, 1.0f,
};

struct FrustumInfo {
    float fieldOfViewY = 0.0f;
    float aspectRatio = 0.0f;
    float nearDistance = 0.0f;
    float farDistance = 0.0f;
    glm::vec3 eye = {};
    frustumPlanes planes = {};
    box box = {};
};

namespace Perspective {

namespace Default {

struct CreateInfo : public Buffer::CreateInfo {
    float aspect = (16.0f / 9.0f);
    glm::vec3 eye{2.0f, 2.0f, 4.0f};
    glm::vec3 center{0.0f, 0.0f, 0.0f};
    float fovy = glm::radians(45.0f);
    float n = 0.1f;
    float f = 1000.0f;
};

struct DATA {
    // View matrix created from glm::lookAt
    glm::mat4 view = glm::mat4(1.0f);
    // This is actually clip_ * proj_
    glm::mat4 projection = glm::mat4(1.0f);
    // World space to view space
    glm::mat4 viewProjection = glm::mat4(1.0f);
    alignas(16) glm::vec3 worldPosition;
    // rem 4
};

class Base : public Obj3d::AbstractBase, public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo);

    inline glm::vec3 getCameraSpaceDirection(const glm::vec3 &d = FORWARD_VECTOR) const {
        glm::vec3 direction = glm::mat3(data_.view) * d;
        return glm::normalize(direction);
    }

    inline glm::vec3 getCameraSpacePosition(const glm::vec3 &p = {}) const { return data_.view * glm::vec4(p, 1.0f); }

    // FORWARD_VECTOR is negated here (cameras look in the negative Z direction traditionally).
    // Not sure if this will cause confusion in the future.
    inline glm::vec3 getWorldSpaceDirection(const glm::vec3 &d = -FORWARD_VECTOR, const uint32_t index = 0) const override {
        glm::vec3 direction = glm::inverse(data_.view) * glm::vec4(d, 0.0f);
        return glm::normalize(direction);
    }

    inline glm::vec3 getWorldSpacePosition(const glm::vec3 &p = {}, const uint32_t index = 0) const override {
        return glm::inverse(data_.view) * glm::vec4(p, 1.0f);
    }

    inline glm::mat4 getCameraSpaceToWorldSpaceTransform() const { return glm::inverse(data_.view); }

    Ray getRay(const glm::vec2 &position, const vk::Extent2D &extent) { return getRay(position, extent, far_); }
    Ray getRay(const glm::vec2 &position, const vk::Extent2D &extent, float distance);

    virtual_inline const auto &getMVP() const { return data_.viewProjection; }
    virtual_inline const auto &getMV() const { return data_.view; }

    void setAspect(float aspect);
    void update(const glm::vec3 &moveDir, const glm::vec2 &lookDir, const uint32_t frameIndex);
    // Everything needed to manually update a camera data for debugging.
    void update(const glm::mat4 &view, const glm::mat4 &proj, const glm::vec3 eye, const glm::vec3 center,
                const uint32_t frameIndex);

    frustumPlanes getFrustumPlanes() const;
    frustumPlanes getFrustumPlanesZupLH() const;
    virtual_inline auto getPosition() const { return eye_; }
    virtual_inline auto getCenter() const { return center_; }
    virtual_inline auto getNearDistance() const { return near_; }
    virtual_inline auto getFarDistance() const { return far_; }
    virtual_inline auto getProj() const { return proj_; }
    FrustumInfo getFrustumInfo() const;
    FrustumInfo getFrustumInfoZupLH() const;

    const glm::mat4 &getModel(const uint32_t index = 0) const override { return model_; }

    // Something like this will be required for pauses... A flag should probably exist on PerFramebufferDataItems and
    // BufferManagers should check it per frame, but I'm too lazy atm.
    void updateAllFrames() {
        dirty = true;
        setData();
    }

    // Take another camera's position/look information.
    void takeCameraData(const Base &camera);

   protected:
    glm::mat4 &model(const uint32_t index = 0) override { return model_; }

   private:
    bool updateView(const glm::vec3 &moveDir, const glm::vec2 &lookDir);
    void update(const uint32_t frameIndex = UINT32_MAX);

    // This should be the only way to set the projection data. Also,
    // this is not actually the projection matrix!!!
    inline void setProjectionData() { data_.projection = VULKAN_CLIP_MAT4 * proj_; }

    glm::mat4 model_;
    // projection
    float aspect_;
    float near_;
    float far_;
    float viewRange_;
    float fovy_;
    glm::mat4 proj_;
    // view
    glm::vec3 eye_;
    glm::vec3 center_;
};

}  // namespace Default

namespace CubeMap {

constexpr uint32_t LAYERS = 6;

struct CreateInfo : public Buffer::CreateInfo {
    glm::vec3 position{0.0f, 0.0f, 0.0f};  // (world space)
    float n = 0.1f;                        // near distance
    float f = 2000.0f;                     // far distance
};

struct DATA {
    std::array<glm::mat4, LAYERS> views;
    glm::mat4 proj;
};

class Base : public Obj3d::AbstractBase, public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo);

    inline const glm::mat4 &getModel(const uint32_t index = 0) const override { return model_; }

   protected:
    glm::mat4 &model(const uint32_t index = 0) override { return model_; }

   private:
    void setViews();

    float near_;
    float far_;
    glm::mat4 model_;
};

}  // namespace CubeMap

namespace Basic {

struct CreateInfo : public Buffer::CreateInfo {
    float aspect = (16.0f / 9.0f);
    glm::vec3 eye{2.0f, 2.0f, 4.0f};
    glm::vec3 center{0.0f, 0.0f, 0.0f};
    float fovy = glm::radians(45.0f);
    float n = 0.1f;
    float f = 1000.0f;
};

struct DATA {
    glm::mat4 viewProj;
};

class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo);

    // WARNING: If you use this function view_, and proj_ will fall out of sync.
    void setViewProj(const glm::mat4 &viewProj, const uint32_t index = UINT32_MAX) {
        data_.viewProj = viewProj;
        setData(index);
    }

   private:
    glm::mat4 view_;
    glm::mat4 proj_;
};

}  // namespace Basic

}  // namespace Perspective
}  // namespace Camera

#endif  // !CAMERA_H