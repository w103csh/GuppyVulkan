#ifndef LIGHT_H
#define LIGHT_H

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Helpers.h"

namespace Light {

enum class TYPE { BASE = 0 };

typedef enum FLAGS {
    HIDE = 0x00000001,
    SHOW = 0x00000002,
    // THROUGH 0x00000008
    MODE_LAMERTIAN = 0x00000010,
    MODE_BLINN_PHONG = 0x00000020,
    // THROUGH 0x00000080
} FLAGS;

class Ambient : public Object3d {
   public:
    struct Data {
        Object3d::Data obj3d;
        // 64
        glm::vec3 color;
        Flags flags;
        // 16
        float intensity;
        float phongExp;
        // 8 rem
    };

    Ambient()
        : color{1.0f, 1.0f, 1.0f},
          flags{FLAGS::HIDE},
          intensity(0.7f),
          phongExp(1000.0f){};

    // Ambient(glm::mat4 m, glm::vec3 c, Flags f, float I, float p) : data_(m) color(c), flags(f), intensity(I), phongExp(p){};

    // bool operator==(const Base& other) const { return pos == other.pos && normal == other.normal; }

    inline Data getData() { return {obj3d_, color, flags, intensity, phongExp}; };

    glm::vec3 color;
    Flags flags;
    // 16
    float intensity;
    float phongExp;

};  // struct Base

}  // namespace Light

#endif  // !LIGHT_H