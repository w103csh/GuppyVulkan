#ifndef STORAGE_CONSTANTS_H
#define STORAGE_CONSTANTS_H

#include <glm/glm.hpp>

namespace Storage {

// VECTOR4
namespace Vector4 {
enum class TYPE {
    DONT_CARE,
    ATTRACTOR_POSITION,
};
using DATA = glm::vec4;
}  // namespace Vector4

// VECTOR2
namespace Vector2 {
enum class TYPE {
    DONT_CARE,
    TEX_COORDS,
};
using DATA = glm::vec2;
}  // namespace Vector2

// POST-PROCESS
namespace PostProcess {
// Offsets
constexpr uint8_t AccumulatedLuminace = 0;
constexpr uint8_t ImageSize = 1;
}  // namespace PostProcess

}  // namespace Storage

#endif  //! STORAGE_CONSTANTS_H