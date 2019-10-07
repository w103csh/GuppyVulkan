#ifndef PARTICLE_CONSTANTS_H
#define PARTICLE_CONSTANTS_H

#include <glm/glm.hpp>
#include <list>
#include <memory>
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

// BUFFER

namespace Buffer {

using index = uint32_t;
constexpr index BAD_OFFSET = UINT32_MAX;

using instancePair = std::pair<Descriptor::Set::bindDataMap, std::shared_ptr<Material::Base>>;

}  // namespace Buffer

// FOUNTAIN

namespace Fountain {

struct CreateInfo {
    PIPELINE pipelineType = PIPELINE::ALL_ENUM;
    std::shared_ptr<Texture::Base> pTexture = nullptr;
    glm::vec3 acceleration{0.0f, -0.05f, 0.0f};  // Particle acceleration (gravity)
    float lifespan{5.5f};                        // Particle lifespan
    glm::mat3 emitterBasis{1.0f};                // Rotation that rotates y axis to the direction of emitter
    float size{1.0f};                            // Size of particle
};

}  // namespace Fountain

}  // namespace Particle

#endif  //! PARTICLE_CONSTANTS_H