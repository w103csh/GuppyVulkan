#ifndef LIGHT_H
#define LIGHT_H

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "BufferItem.h"
#include "ConstantsAll.h"
#include "Helpers.h"
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

// **********************
//      Base
// **********************

template <typename T>
class Base : public Obj3d, public Uniform::Base, public Buffer::DataItem<T> {
   public:
    Base(T *pData, CreateInfo *pCreateInfo)
        : Obj3d(),          //
          Uniform::Base(),  //
          Buffer::DataItem<T>(pData),
          model_(pCreateInfo->model) {}

    inline const glm::mat4 &model(uint32_t index = 0) const override { return model_; }

   private:
    glm::mat4 model_;
};

namespace Default {

// **********************
//      Positional
// **********************

namespace Positional {
struct DATA {
    glm::vec3 position{};            // 12 (camera space)
    FlagBits flags{FLAG::SHOW};      // 4
    alignas(16) glm::vec3 La{0.1f};  // 12 (Ambient light intensity)
    // 4 rem
    alignas(16) glm::vec3 L{0.6f};  // 12 Diffuse and specular light intensity
    // 4 rem
};
class Base : public Light::Base<DATA> {
   public:
    Base(const Buffer::Info &&info, DATA *pData, CreateInfo *pCreateInfo);

    void update(glm::vec3 &&position);

    inline const glm::vec3 &getPosition() { return position; }

    void transform(const glm::mat4 t, uint32_t index = 0) override {
        Obj3d::transform(t);
        position = getWorldSpacePosition();
    }

    // FLAG
    inline FlagBits getFlags() const { return pData_->flags; }
    inline void setFlags(const FlagBits &flags) {
        pData_->flags = flags;
        DIRTY = true;
    }

   private:
    glm::vec3 position;  // (world space)
};
}  // namespace Positional

// **********************
//      Spot
// **********************

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
    Base(const Buffer::Info &&info, DATA *pData, CreateInfo *pCreateInfo);
    void transform(const glm::mat4 t, uint32_t index = 0) override {
        Obj3d::transform(t);
        position = getWorldSpacePosition();
        direction = getWorldSpaceDirection();
    }

    void update(glm::vec3 &&position, glm::vec3 &&direction);

    inline const glm::vec3 &getDirection() { return direction; }
    inline const glm::vec3 &getPosition() { return position; }

    inline void setCutoff(float f) { pData_->cutoff = f; }
    inline void setExponent(float f) { pData_->exponent = f; }

   private:
    glm::vec3 direction;  // (world space)
    glm::vec3 position;   // (world space)
};
}  // namespace Spot

}  // namespace Default

}  // namespace Light

#endif  // !LIGHT_H