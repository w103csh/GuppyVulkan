/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef PARTICLE_H
#define PARTICLE_H

#include <glm/glm.hpp>
#include <string_view>

#include "BufferItem.h"
#include "ConstantsAll.h"
#include "Descriptor.h"
#include "Instance.h"
#include "Pipeline.h"

// SAMPLER

namespace Sampler {
namespace Particle {
constexpr std::string_view RAND_1D_ID = "Particle Random 1D Sampler";
}  // namespace Particle
}  // namespace Sampler

// TEXTURE

namespace Texture {
namespace Particle {
constexpr std::string_view RAND_1D_ID = "Particle Random 1D Texture";
}  // namespace Particle
}  // namespace Texture

// SHADER

namespace Shader {
namespace Particle {
extern const CreateInfo WAVE_VERT_CREATE_INFO;
extern const CreateInfo FOUNTAIN_VERT_CREATE_INFO;
extern const CreateInfo FOUNTAIN_FRAG_DEFERRED_MRT_CREATE_INFO;
// EULER
extern const CreateInfo EULER_CREATE_INFO;
extern const CreateInfo FOUNTAIN_EULER_VERT_CREATE_INFO;
extern const CreateInfo SHDW_FOUNTAIN_EULER_VERT_CREATE_INFO;
// ATTRACTOR
extern const CreateInfo ATTR_COMP_CREATE_INFO;
extern const CreateInfo ATTR_VERT_CREATE_INFO;
}  // namespace Particle
namespace Link {
namespace Particle {
extern const CreateInfo FOUNTAIN_CREATE_INFO;
}  // namespace Particle
}  // namespace Link
}  // namespace Shader

// UNIFORM

namespace Uniform {
namespace Particle {

// TODO: This should be a dynamic uniform!!!!
namespace Wave {
struct DATA {
    float time;
    float freq = 2.5f;
    float velocity = 2.5f;
    float amp = 0.6f;
};
class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const Buffer::CreateInfo* pCreateInfo);
    void updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) override;
};
}  // namespace Wave

}  // namespace Particle
}  // namespace Uniform

// UNIFORM DYNAMIC

namespace UniformDynamic {
namespace Particle {

// FOUNTAIN
namespace Fountain {

enum class INSTANCE {
    DEFAULT,
    EULER,
};

struct DATA {
    glm::vec3 acceleration;              // Particle acceleration (gravity)
    float lifespan;                      // Particle lifespan
    glm::vec3 emitterPosition;           // World position of the emitter.
    float time = ::Particle::BAD_TIME;   // Simulation time
    glm::mat4 emitterBasis;              // Rotation that rotates y axis to the direction of emitter
    float minParticleSize;               // Minimum size of particle (used as default)
    float maxParticleSize;               // Maximum size of particle
    float delta = ::Particle::BAD_TIME;  // Elapsed time between frames from the start signal
    float velocityLowerBound;            // Lower bound of the generated random velocity (euler)
    float velocityUpperBound;            // Upper bound of the generated random velocity (euler)
    glm::vec3 _padding;
};

struct CreateInfo : public ::Buffer::CreateInfo {
    INSTANCE type = INSTANCE::DEFAULT;
    glm::vec3 acceleration{0.0f, -0.05f, 0.0f};  // Particle acceleration (gravity)
    float lifespan{5.5f};                        // Particle lifespan
    glm::mat3 emitterBasis{1.0f};                // Rotation that rotates y axis to the direction of emitter
    float minParticleSize = 1.0f;                // Minimum size of particle (used as default)
    float maxParticleSize = 1.0f;                // Maximum size of particle
    float velocityLowerBound = 1.25f;            // Lower bound of the generated random velocity (euler)
    float velocityUpperBound = 1.5f;             // Upper bound of the generated random velocity (euler)
};

class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);

    const INSTANCE INSTANCE_TYPE;

    // Descriptor::Base
    void updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) override;

    void reset();

    virtual_inline auto getDelta() const { return data_.delta; }
    virtual_inline auto getLifespan() const { return data_.lifespan; }
};

}  // namespace Fountain

// ATTRACTOR
namespace Attractor {

struct DATA {
    glm::vec3 attractorPosition0;
    float gravity0;
    glm::vec3 attractorPosition1;
    float gravity1;
    float inverseMass;
    float maxDistance;
    alignas(8) float delta;
    // rem 4
};

struct CreateInfo : public Buffer::CreateInfo {
    glm::vec3 attractorPosition0{5.0f, 0.0f, 0.0f};
    float gravity0 = 1000.0f;
    glm::vec3 attractorPosition1{-5.0f, 0.0f, 0.0f};
    float gravity1 = 1000.0f;
    float inverseMass = 1.0f / 0.1f;
    float maxDistance = 45.0f;
};

class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);

    void updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) override;
};

}  // namespace Attractor

}  // namespace Particle
}  // namespace UniformDynamic

// DESCRIPTOR SET

namespace Descriptor {
namespace Set {
namespace Particle {
extern const CreateInfo WAVE_CREATE_INFO;
extern const CreateInfo FOUNTAIN_CREATE_INFO;
extern const CreateInfo EULER_CREATE_INFO;
extern const CreateInfo ATTRACTOR_CREATE_INFO;
}  // namespace Particle
}  // namespace Set
}  // namespace Descriptor

// PIPELINE

namespace Pipeline {

void GetFountainEulerInputAssemblyInfoResources(CreateInfoResources& createInfoRes);

struct CreateInfo;
class Handler;

namespace Particle {

class Wave : public Graphics {
   public:
    const bool IS_DEFERRED;
    Wave(Handler& handler, const bool isDeferred = true);
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
};

class Fountain : public Graphics {
   public:
    const bool DO_BLEND;
    const bool IS_DEFERRED;
    Fountain(Handler& handler, const bool isDeferred = true);
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
};

class Euler : public Compute {
   public:
    Euler(Handler& handler);
};

class FountainEuler : public Graphics {
   public:
    const bool DO_BLEND;
    const bool IS_DEFERRED;
    FountainEuler(Handler& handler, const bool doBlend = true, const bool isDeferred = true);
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
};

class ShadowFountainEuler : public Graphics {
   public:
    ShadowFountainEuler(Handler& handler);
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
};

class AttractorCompute : public Compute {
   public:
    AttractorCompute(Handler& handler);
};

class AttractorPoint : public Pipeline::Graphics {
   public:
    const bool DO_BLEND;
    const bool IS_DEFERRED;
    AttractorPoint(Handler& handler, const bool doBlend = false, const bool isDeferred = true);
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
};

}  // namespace Particle
}  // namespace Pipeline

#endif  //! PARTICLE_H
