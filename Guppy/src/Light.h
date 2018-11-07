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
    BITS_MAX_ENUM = 0x7FFFFFFF
} FLAGS;

class Positional : public Object3d {
   public:
    struct Data {
        glm::vec4 position;       // 16
        glm::vec3 La;             // 12 (Ambient light intensity)
        FlagBits flags;           // 4
        alignas(16) glm::vec3 L;  // 12 Diffuse and specular light intensity
        // 4 rem
    };

    Positional() : flags_(FLAGS::SHOW), La_(glm::vec3(0.2f)), L_(glm::vec3(0.6f)){};

    // bool operator==(const Base& other) const { return pos == other.pos && normal == other.normal; }

    inline FlagBits getFlags() const { return flags_; }
    inline void setFlags(FlagBits flags) { flags_ = flags; }

    void getData(Data& data);

   private:
    FlagBits flags_;
    glm::vec3 La_;
    glm::vec3 L_;

};  // struct Base

}  // namespace Light

#endif  // !LIGHT_H