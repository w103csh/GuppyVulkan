#ifndef PARTICLE_CONSTANTS_H
#define PARTICLE_CONSTANTS_H

#include <glm/glm.hpp>
#include <list>
#include <memory>
#include <string>
#include <utility>

#include "PipelineConstants.h"

// clang-format off
namespace Material { class Base; }
namespace Texture { class Base; }
// clang-format on

namespace Particle {

constexpr float BAD_TIME = -1.0f;

struct StartInfo {
    uint32_t bufferOffset;
    std::list<uint32_t> instanceOffsets;
};

namespace Euler {
// clang-format off
enum class FLAG : FlagBits {
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
    std::shared_ptr<Material::Base> pMaterial;
    Descriptor::Set::bindDataMap computeDescSetBindDataMap;
    Descriptor::Set::bindDataMap graphicsDescSetBindDataMap;
    Descriptor::Set::bindDataMap shadowDescSetBindDataMap;
};

}  // namespace Buffer

// FOUNTAIN

namespace Fountain {

struct CreateInfo {
    std::string name = "";
    PIPELINE pipelineTypeGraphics = PIPELINE::ALL_ENUM;
    PIPELINE pipelineTypeCompute = PIPELINE::ALL_ENUM;
    std::shared_ptr<Texture::Base> pTexture = nullptr;
    glm::vec3 acceleration{0.0f, -0.05f, 0.0f};  // Particle acceleration (gravity)
    float lifespan{5.5f};                        // Particle lifespan
    glm::mat3 emitterBasis{1.0f};                // Rotation that rotates y axis to the direction of emitter
    float minParticleSize = 1.0f;                // Minimum size of particle (used as default)
    float maxParticleSize = 1.0f;                // Maximum size of particle
    Euler::PushConstant computeFlag = Euler::FLAG::FOUNTAIN;
};

}  // namespace Fountain

}  // namespace Particle

#endif  //! PARTICLE_CONSTANTS_H