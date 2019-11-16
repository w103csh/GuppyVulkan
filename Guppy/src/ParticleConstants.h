#ifndef PARTICLE_CONSTANTS_H
#define PARTICLE_CONSTANTS_H

#include "DescriptorConstants.h"
#include "StorageConstants.h"

// clang-format off
namespace Descriptor { class Base; }
namespace Material { namespace Obj3d { class Default; } }
namespace Texture { class Base; }
// clang-format on

namespace Particle {

constexpr float BAD_TIME = -1.0f;

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

namespace Storage {
namespace Vector4 {
struct CreateInfo;
namespace Particle {
namespace Attractor {
void SetDataPosition(Descriptor::Base* pItem, Vector4::DATA*& pData, const CreateInfo* pCreateInfo);
}  // namespace Attractor
}  // namespace Particle
}  // namespace Vector4
}  // namespace Storage

#endif  //! PARTICLE_CONSTANTS_H