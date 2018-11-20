#ifndef LIGHT_H
#define LIGHT_H

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Object3d.h"

namespace Light {

enum class TYPE { BASE = 0 };

typedef enum FLAGS {
    SHOW = 0x00000001,
    // THROUGH 0x00000008
    MODE_LAMERTIAN = 0x00000010,
    MODE_BLINN_PHONG = 0x00000020,
    // THROUGH 0x00000080
    BITS_MAX_ENUM = 0x7FFFFFFF
} FLAGS;

template <class T>
struct Base : public Object3d {
   public:
    Base() : flags_(FLAGS::SHOW){};

    // bool operator==(const Base& other) const { return pos == other.pos && normal == other.normal; }

    inline FlagBits getFlags() const { return flags_; }
    inline void setFlags(FlagBits flags) { flags_ = flags; }

    virtual void getData(T& data) = 0;

   protected:
    FlagBits flags_;
};  // class Base

struct PositionalData {
    glm::vec3 position;        // 12
    FlagBits flags;            // 4
    alignas(16) glm::vec3 La;  // 12 (Ambient light intensity)
    // 4 rem
    alignas(16) glm::vec3 L;  // 12 Diffuse and specular light intensity
    // 4 rem
};  // struct BaseData

class Positional : public Base<PositionalData> {
   public:
    Positional() : Base(), La_(glm::vec3(0.1f)), L_(glm::vec3(0.6f)){};

    void getData(PositionalData& data) override {
        data.position = getWorldSpacePosition();
        data.flags = flags_;
        data.La = La_;
        data.L = L_;
    };

   private:
    glm::vec3 La_;
    glm::vec3 L_;

};  // class Positional

struct SpotData {
    glm::vec3 position;               // 12
    FlagBits flags;                   // 4
    glm::vec3 La;                     // 12 (Ambient light intensity)
    float exponent;                   // 4
    glm::vec3 L;                      // 12 Diffuse and specular light intensity
    float cutoff;                     // 4
    alignas(16) glm::vec3 direction;  // 12
    // 4 rem
};

class Spot : public Base<SpotData> {
   public:
    Spot() : Base(), La_(glm::vec3(0.5f)), L_(glm::vec3(0.9f)), exponent_(50.0f), cutoff_(glm::radians(15.0f)){};

    void getData(SpotData& data) override {
        data.flags = flags_;
        data.position = getWorldSpacePosition();
        data.direction = getWorldSpaceDirection();
        data.La = La_;
        data.L = L_;
        data.exponent = exponent_;
        data.cutoff = cutoff_;
    };

    inline void setCutoff(float f) { cutoff_ = f; }
    inline void setExponent(float f) { exponent_ = f; }

   private:
    glm::vec3 La_;
    glm::vec3 L_;
    float exponent_;
    float cutoff_;

};  // class Spot

}  // namespace Light

#endif  // !LIGHT_H