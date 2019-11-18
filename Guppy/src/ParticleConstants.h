#ifndef PARTICLE_CONSTANTS_H
#define PARTICLE_CONSTANTS_H

namespace Particle {

constexpr float BAD_TIME = -1.0f;

// EULER
namespace Euler {
// clang-format off
enum class FLAG : FlagBits {
    NONE =      0x00,
    FOUNTAIN =  0x01,
    FIRE =      0x02,
    SMOKE =     0x04,
};
// clang-format on
using PushConstant = FLAG;
}  // namespace Euler

// BUFFER
namespace Buffer {
using index = uint32_t;
constexpr index BAD_OFFSET = UINT32_MAX;
}  // namespace Buffer

}  // namespace Particle

#endif  //! PARTICLE_CONSTANTS_H