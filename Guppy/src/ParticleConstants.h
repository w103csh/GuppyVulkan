#ifndef PARTICLE_CONSTANTS_H
#define PARTICLE_CONSTANTS_H

#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "PipelineConstants.h"

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

struct InstanceInfo {
    std::shared_ptr<Material::Obj3d::Default> pObj3dMaterial;
    Descriptor::Set::bindDataMap computeDescSetBindDataMap;
    Descriptor::Set::bindDataMap graphicsDescSetBindDataMap;
    Descriptor::Set::bindDataMap shadowDescSetBindDataMap;
};

}  // namespace Buffer

}  // namespace Particle

#endif  //! PARTICLE_CONSTANTS_H