/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef CLOTH_H
#define CLOTH_H

#include <array>
#include <glm/glm.hpp>
#include <memory>

#include "ConstantsAll.h"
#include "Obj3dDrawInst.h"
#include "ParticleBuffer.h"
#include "Pipeline.h"
#include "Plane.h"

// clang-format off
namespace Storage { namespace Vector4 { class Base; } }
// clang-format on

namespace Particle {
namespace Cloth {

constexpr uint32_t ITERATIONS_PER_FRAME_IDEAL = 999;
constexpr float FRAMES_PER_SECOND_FACTOR_IDEAL = 1.0f / 60.0f;
constexpr float ITERATIONS_PER_FRAME_FACTOR = ITERATIONS_PER_FRAME_IDEAL / FRAMES_PER_SECOND_FACTOR_IDEAL;

constexpr float INTERGRATION_STEP_PER_FRAME_IDEAL = 0.000005f;
constexpr float INTEGRATION_STEP_PER_FRAME_FACTOR = INTERGRATION_STEP_PER_FRAME_IDEAL / FRAMES_PER_SECOND_FACTOR_IDEAL;

}  // namespace Cloth
}  // namespace Particle

// SHADER
namespace Shader {
namespace Particle {
extern const CreateInfo CLOTH_COMP_CREATE_INFO;
extern const CreateInfo CLOTH_NORM_COMP_CREATE_INFO;
extern const CreateInfo CLOTH_VERT_CREATE_INFO;
}  // namespace Particle
}  // namespace Shader

// UNIFORM DYNAMIC
namespace UniformDynamic {
namespace Particle {
namespace Cloth {

struct DATA {
    glm::vec3 gravity;
    float springK;
    float mass;
    float inverseMass;
    float restLengthHoriz;
    float restLengthVert;
    float restLengthDiag;
    float dampingConst;
    alignas(8) float delta;
    // rem 4
};

struct CreateInfo : Buffer::CreateInfo {
    glm::vec3 gravity{0.0f, -10.0f, 0.0f};
    float springK = 2000.0f;
    float mass = 0.1f;
    float dampingConst = 0.1f;
    Mesh::Plane::Info planeInfo = {};
};

class Base : public Descriptor::Base, public Buffer::PerFramebufferDataItem<DATA> {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);

    void updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) override;

   private:
    std::array<float, 3> deltas_;
    const glm::vec3 acceleration_;
};

}  // namespace Cloth
}  // namespace Particle
}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
namespace Particle {
extern const CreateInfo CLOTH_CREATE_INFO;
extern const CreateInfo CLOTH_NORM_CREATE_INFO;
}  // namespace Particle
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {
class Handler;
namespace Particle {

class ClothCompute : public Compute {
   public:
    ClothCompute(Handler& handler);
};

class ClothNormalCompute : public Compute {
   public:
    ClothNormalCompute(Handler& handler);
};

class Cloth : public Graphics {
   public:
    const bool DO_BLEND;
    const bool IS_DEFERRED;
    Cloth(Handler& handler);

   private:
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
};

}  // namespace Particle
}  // namespace Pipeline

// BUFFER
namespace Particle {
class Handler;
namespace Buffer {
namespace Cloth {

struct CreateInfo : Buffer::CreateInfo {
    Mesh::Geometry::Info geometryInfo = {};
    Mesh::Plane::Info planeInfo = {};
};

class Base : public Buffer::Base, public Obj3d::InstanceDraw {
   public:
    Base(Handler& handler, const index&& offset, const CreateInfo* pCreateInfo, std::shared_ptr<Material::Base>& pMaterial,
         const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors,
         std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData);

    virtual void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                      const uint8_t frameIndex) const override;
    void dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                  const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                  const uint8_t frameIndex) const override;

    void getComputeDescSetBindData() override;

   private:
    // TOOD: use pDescriptors_ instead of these...
    std::array<std::shared_ptr<Storage::Vector4::Base>, 2> pPosBuffs_;
    std::array<std::shared_ptr<Storage::Vector4::Base>, 2> pVelBuffs_;
    std::shared_ptr<Storage::Vector4::Base> pNormBuff_;
};

}  // namespace Cloth
}  // namespace Buffer
}  // namespace Particle

#endif  // !CLOTH_H
