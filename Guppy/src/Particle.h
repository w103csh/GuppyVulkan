#ifndef PARTICLE_H
#define PARTICLE_H

#include <glm/glm.hpp>
#include <string_view>

#include "BufferItem.h"
#include "ConstantsAll.h"
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
extern const CreateInfo WAVE_COLOR_VERT_DEFERRED_MRT_CREATE_INFO;
extern const CreateInfo FOUNTAIN_PART_VERT_CREATE_INFO;
extern const CreateInfo FOUNTAIN_PART_FRAG_DEFERRED_MRT_CREATE_INFO;
extern const CreateInfo PARTICLE_EULER_CREATE_INFO;
extern const CreateInfo FOUNTAIN_PART_EULER_VERT_CREATE_INFO;
extern const CreateInfo SHADOW_FOUNTAIN_PART_EULER_VERT_CREATE_INFO;
}  // namespace Particle
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
    void update(const float time, const uint32_t frameIndex);
};
}  // namespace Wave

}  // namespace Particle
}  // namespace Uniform

// MATERIAL

namespace Material {
namespace Particle {
namespace Fountain {

struct DATA : public Material::Obj3d::DATA {
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

struct CreateInfo : public Material::Obj3d::CreateInfo {
    CreateInfo(const ::Particle::Fountain::CreateInfo* pCreateInfo, const glm::mat4 m, const uint32_t imageCount)
        : fntnInfo(*pCreateInfo) {
        dataCount = imageCount;
        pTexture = pCreateInfo->pTexture;
        model = m;
    }
    ::Particle::Fountain::CreateInfo fntnInfo;
    float velocityLowerBound = 1.25f;
    float velocityUpperBound = 1.5f;
};

// TODO: The data here really should be separated from the material data. I made it one large dynamic uniform
// because I didn't feel like testing multiple dynamic uniform types in the descriptor handler at the time.
// Rarely would the fountain data need to updated at a similar interval as the material data...
class Base : public Material::Obj3d::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);

    FlagBits getFlags() override { return data_.flags; }
    void setFlags(FlagBits flags) override { data_.flags = flags; }

    void setTextureData() override { status_ = SetDefaultTextureData(this, &data_); }

    virtual_inline auto getDelta() const { return data_.delta; }
    virtual_inline bool shouldDraw(const float lastTimeOfBirth) const {
        return data_.delta >= 0.0 && data_.delta < (lastTimeOfBirth + data_.lifespan + 0.0001f);
    }
    virtual_inline bool shouldDrawEuler() const { return start_; }

    const glm::mat4& model(const uint32_t index = 0) const override { return data_.model; }

    void start();
    void update(const float time, const float lastTimeOfBirth, const uint32_t frameIndex);
    void updateEuler(const float elapsed, const uint32_t frameIndex);

   private:
    bool start_;
};

}  // namespace Fountain
}  // namespace Particle
}  // namespace Material

// DESCRIPTOR SET

namespace Descriptor {
namespace Set {
namespace Particle {
extern const CreateInfo WAVE_CREATE_INFO;
extern const CreateInfo FOUNTAIN_CREATE_INFO;
extern const CreateInfo EULER_CREATE_INFO;
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
    const uint32_t LOCAL_SIZE_X;
    Euler(Handler& handler);
    void getShaderStageInfoResources(CreateInfoResources& createInfoRes) override;
};

class FountainEuler : public Graphics {
   public:
    const bool DO_BLEND;
    const bool IS_DEFERRED;
    FountainEuler(Handler& handler, const bool doBlend = true, const bool isDeferred = true);
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
};

class ShadowFountainEuler : public Pipeline::Graphics {
   public:
    ShadowFountainEuler(Handler& handler);

   private:
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
};
}  // namespace Particle
}  // namespace Pipeline

#endif  //! PARTICLE_H
