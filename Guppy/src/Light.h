#ifndef LIGHT_H
#define LIGHT_H

#include <array>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Helpers.h"
#include "Object3d.h"

namespace Light {

typedef enum FLAG {
    SHOW = 0x00000001,
    // THROUGH 0x00000008
    MODE_LAMERTIAN = 0x00000010,
    MODE_BLINN_PHONG = 0x00000020,
    // THROUGH 0x00000080
    BITS_MAX_ENUM = 0x7FFFFFFF
} FLAG;

class Base : public Object3d, public DataObject {};

namespace Default {

class Positional : public Base {
   public:
    struct DATA {
        glm::vec3 position{};            // 12
        FlagBits flags{FLAG::SHOW};      // 4
        alignas(16) glm::vec3 La{0.1f};  // 12 (Ambient light intensity)
        // 4 rem
        alignas(16) glm::vec3 L{0.6f};  // 12 Diffuse and specular light intensity
        // 4 rem
    };

    Positional(DATA data = {}) : Base(), data_(data){};

   private:
    inline VkDeviceSize getDataSize() const { return sizeof DATA; }
    inline const void* getData() { return &data_; }

    DATA data_;
};

class Spot : public Base {
   public:
    struct DATA {
        glm::vec3 position{};                         // 12
        FlagBits flags{FLAG::SHOW};                   // 4
        glm::vec3 La{0.0f};                           // 12 (Ambient light intensity)
        float exponent{50.0f};                        // 4
        glm::vec3 L{0.9f};                            // 12 Diffuse and specular light intensity
        float cutoff{glm::radians(15.0f)};            // 4
        alignas(16) glm::vec3 direction{CARDINAL_Z};  // 12
        // 4 rem
    };

    Spot(DATA data = {}) : Base(), data_(data){};

    inline void setCutoff(float f) { data_.cutoff = f; }
    inline void setExponent(float f) { data_.exponent = f; }

   private:
    inline VkDeviceSize getDataSize() const { return sizeof DATA; }
    inline const void* getData() { return &data_; }

    DATA data_;
};

}  // namespace Default

}  // namespace Light

#endif  // !LIGHT_H