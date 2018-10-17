#ifndef DIR_LIGHT_H
#define DIR_LIGHT_H

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Constants.h"

namespace DirectionalLight {

enum class TYPE { BASE = 0 };

typedef enum FLAGS {
    HIDE = 0x00000001,
    SHOW = 0x00000002,
    // THROUGH 0x00000008
    MODE_BLINN_PHONG = 0x00000010,
    MODE_BLINN = 0x00000020,
    // THROUGH 0x00000080
} FLAGS;

struct Base {
    Base()
        : pos{6.0f, 6.0f, 6.0f},
          power{40.0f},
          color{1.0f, 1.0f, 1.0f},
          shininess{16.0f},
          diff{0.5f, 0.0f, 0.0f},
          flags{(FLAGS::SHOW | FLAGS::MODE_BLINN_PHONG)},
          spec{1.0f, 1.0f, 1.0f} {};

    Base(glm::vec3 p, float pow, glm::vec3 c, float shi, glm::vec3 d, FLAGS f, glm::vec3 s)
        : pos(p), power(pow), color(c), shininess(shi), diff(d), flags(f), spec(s){};

    // bool operator==(const Base& other) const { return pos == other.pos && normal == other.normal; }

    glm::vec3 pos;
    float power;
    glm::vec3 color;
    float shininess;
    glm::vec3 diff;
    Flags flags;
    alignas(16) glm::vec3 spec;

};  // struct Base

}  // namespace DirectionalLight

#endif  // !DIR_LIGHT_H