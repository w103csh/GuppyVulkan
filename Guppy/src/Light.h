/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef LIGHT_H
#define LIGHT_H

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

#include <Common/Helpers.h>

#include "BufferItem.h"
#include "ConstantsAll.h"
#include "Obj3d.h"
#include "Uniform.h"

namespace Light {

typedef enum FLAG {
    SHOW = 0x00000001,
    // THROUGH 0x00000008
    MODE_LAMERTIAN = 0x00000010,
    MODE_BLINN_PHONG = 0x00000020,
    // THROUGH 0x0000080
    DIRECTIONAL = 0x00000100,  // TODO: hook this up (and make positional/diretional the same)
    POSITIONAL = 0x00000200,   // TODO: hook this up
    // THROUGH 0x00000800
    TEST_1 = 0x01000000,
    TEST_2 = 0x02000000,
    TEST_3 = 0x04000000,
    TEST_4 = 0x08000000,
    TEST_ALL = 0x0F000000,
    //
    BITS_MAX_ENUM = 0x7FFFFFFF
} FLAG;

struct CreateInfo : public Buffer::CreateInfo {
    glm::mat4 model{1.0f};
};

// BASE

template <typename TDATA>
class Base : public Obj3d::AbstractBase, public Descriptor::Base, public Buffer::PerFramebufferDataItem<TDATA> {
   public:
    Base(const DESCRIPTOR &&descType, TDATA *pData, const CreateInfo *pCreateInfo)
        : Descriptor::Base(std::forward<const DESCRIPTOR>(descType)),  //
          Buffer::PerFramebufferDataItem<TDATA>(pData),                //
          model_(pCreateInfo->model) {}

    inline const glm::mat4 &getModel(const uint32_t index = 0) const override { return model_; }

   protected:
    glm::mat4 &model(const uint32_t index = 0) override { return model_; }

   private:
    glm::mat4 model_;
};

namespace Default {

// DIRECTIONAL
namespace Directional {
struct DATA {
    glm::vec3 direction{};     // Direction to the light (s) (camera space)
    FlagBits flags;            //
    alignas(16) glm::vec3 La;  // Ambient light intensity
    alignas(16) glm::vec3 L;   // Diffuse and specular light intensity
};
struct CreateInfo : Buffer::CreateInfo {
    glm::vec3 direction;         // Direction to the light (s) (world space)
    FlagBits flags{FLAG::SHOW};  //
    glm::vec3 La{0.05f};         // Ambient light intensity
    glm::vec3 L{0.6f};           // Diffuse and specular light intensity
};
class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo);

    void update(const glm::vec3 direction, const uint32_t frameIndex);

    // FLAG
    inline FlagBits getFlags() const { return data_.flags; }
    inline void setFlags(const FlagBits &flags) { data_.flags = flags; }

    glm::vec3 direction;  // (world space)
};
}  // namespace Directional

// POSITIONAL
namespace Positional {
struct DATA {
    glm::vec3 position{};            // 12 (camera space)
    FlagBits flags{FLAG::SHOW};      // 4
    alignas(16) glm::vec3 La{0.2f};  // 12 (Ambient light intensity)
    // 4 rem
    alignas(16) glm::vec3 L{0.6f};  // 12 Diffuse and specular light intensity
    // 4 rem
};
class Base : public Light::Base<DATA> {
   public:
    Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo);

    void update(glm::vec3 &&position, const uint32_t frameIndex);

    inline const glm::vec3 &getPosition() { return position_; }

    void transform(const glm::mat4 t, const uint32_t index = 0) override {
        Obj3d::AbstractBase::transform(t);
        position_ = getWorldSpacePosition();
    }

    // FLAG
    inline FlagBits getFlags() const { return data_.flags; }
    inline void setFlags(const FlagBits &flags) {
        data_.flags = flags;
        dirty = true;
    }

   private:
    glm::vec3 position_;  // (world space)
};
}  // namespace Positional

// SPOT
namespace Spot {
struct CreateInfo : public Light::CreateInfo {
    float cutoff = glm::radians(15.0f);
    float exponent = 50.0f;
};
struct DATA {
    glm::vec3 position{};                         // 12 (camera space)
    FlagBits flags = FLAG::SHOW;                  // 4
    glm::vec3 La{0.0f};                           // 12 (Ambient light intensity)
    float exponent = 50.0f;                       // 4
    glm::vec3 L{0.9f};                            // 12 Diffuse and specular light intensity
    float cutoff = glm::radians(15.0f);           // 4
    alignas(16) glm::vec3 direction{CARDINAL_Z};  // 12 (camera space)
    // 4 rem
};
class Base : public Light::Base<DATA> {
   public:
    Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo);
    void transform(const glm::mat4 t, const uint32_t index = 0) override {
        Obj3d::AbstractBase::transform(t);
        position_ = getWorldSpacePosition();
        direction_ = getWorldSpaceDirection();
    }

    void update(glm::vec3 &&position, glm::vec3 &&direction, const uint32_t frameIndex);

    inline const glm::vec3 &getDirection() { return direction_; }
    inline const glm::vec3 &getPosition() { return position_; }

    inline void setCutoff(float f) { data_.cutoff = f; }
    inline void setExponent(float f) { data_.exponent = f; }

   private:
    glm::vec3 direction_;  // (world space)
    glm::vec3 position_;   // (world space)
};
}  // namespace Spot

}  // namespace Default

}  // namespace Light

#endif  // !LIGHT_H
