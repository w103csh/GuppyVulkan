/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef PARTICLE_BUFFER_H
#define PARTICLE_BUFFER_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "BufferItem.h"
#include "ConstantsAll.h"
#include "Descriptor.h"
#include "Handlee.h"
#include "Obj3dDrawInst.h"
#include "Particle.h"

// clang-format off
namespace Material { class Base; }
// clang-format on

namespace Particle {
using TEXCOORD = glm::vec2;
}

// INSTANCE DATA
namespace Particle {

// FOUNTAIN
namespace Fountain {

struct DATA {
    static void getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes);
    glm::vec3 velocity;
    float timeOfBirth;
};

class Base;

struct CreateInfo : ::Buffer::CreateInfo {
    CreateInfo(const UniformDynamic::Particle::Fountain::CreateInfo* pCreateInfo, const uint32_t numberOfParticles)
        : pInfo(pCreateInfo) {
        countInRange = true;
        dataCount = numberOfParticles;
    }
    const UniformDynamic::Particle::Fountain::CreateInfo* pInfo;
};

class Base : public ::Buffer::DataItem<DATA>, public Instance::Base {
   public:
    Base(const ::Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);

    virtual_inline auto getLastTimeOfBirth() const { return pData_[BUFFER_INFO.count - 1].timeOfBirth; }
};

}  // namespace Fountain

// FOUNTAIN (EULER)
namespace FountainEuler {

struct DATA {
    static void getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes);
    /**
     * data0[0]: position.x
     * data0[1]: position.y
     * data0[2]: position.z
     * data0[3]: padding
     *
     * data1[0]: veloctiy.x
     * data1[1]: veloctiy.y
     * data1[2]: veloctiy.z
     * data1[3]: age
     *
     * data2[0]: rotation angle
     * data2[1]: rotation velocity
     * data2[2]: padding
     * data2[3]: padding
     */
    glm::vec4 data0;
    glm::vec4 data1;
    glm::vec4 data2;
};

class Base;

struct CreateInfo : ::Buffer::CreateInfo {
    CreateInfo(const UniformDynamic::Particle::Fountain::CreateInfo* pCreateInfo, const uint32_t numberOfParticles)
        : pInfo(pCreateInfo) {
        countInRange = true;
        dataCount = numberOfParticles;
    }
    const UniformDynamic::Particle::Fountain::CreateInfo* pInfo;
};

class Base : public ::Buffer::DataItem<DATA>, public Descriptor::Base {
   public:
    Base(const ::Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);
};

}  // namespace FountainEuler

}  // namespace Particle

// BUFFER

namespace Particle {
class Handler;
namespace Buffer {

struct CreateInfo {
    std::string name = "";
    glm::uvec3 localSize{1, 1, 1};
    std::vector<COMPUTE> computePipelineTypes;
    std::vector<GRAPHICS> graphicsPipelineTypes;
};

// BASE

class Base : public NonCopyable, public Handlee<Handler> {
   public:
    virtual ~Base() = default;

    const std::string NAME;
    const glm::uvec3 LOCAL_SIZE;
    const std::vector<COMPUTE> COMPUTE_PIPELINE_TYPES;
    const std::vector<GRAPHICS> GRAPHICS_PIPELINE_TYPES;
    const GRAPHICS SHADOW_PIPELINE_TYPE;

    inline void toggle() {
        toggleDraw();
        togglePause();
    }
    inline void toggleDraw() { draw_ = !draw_; }
    inline void togglePause() {
        paused_ = !paused_;
        if (paused_) doPausedUpdate_ = true;
    }
    inline bool shouldDraw() const { return status_ == STATUS::READY && draw_; }
    virtual inline bool shouldDispatch() const { return status_ == STATUS::READY && !paused_; }

    virtual void update(const float time, const float elapsed, const uint32_t frameIndex);

    void prepare();
    virtual void destroy();

    constexpr const auto& getOffset() const { return offset_; }
    constexpr const auto& getStatus() const { return status_; }
    constexpr const auto& getPaused() const { return paused_; }
    constexpr const auto& getDraw() const { return draw_; }

    const std::vector<Descriptor::Base*> getSDynamicDataItems(const PIPELINE pipelineType) const;

    // TODO: These are directly copied from Mesh::Base. Changed getDescriptorSetBindData slightly.
    virtual void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                      const uint8_t frameIndex) const = 0;
    virtual void dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                          const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                          const uint8_t frameIndex) const {}
    const Descriptor::Set::BindData& getDescriptorSetBindData(const PASS& passType,
                                                              const Descriptor::Set::bindDataMap& map) const;

    virtual_inline const auto& getComputeDescSetBindDataMaps() const { return computeDescSetBindDataMaps_; }
    virtual void getComputeDescSetBindData();

    virtual_inline const auto& getGraphicsDescSetBindDataMaps() const { return graphicsDescSetBindDataMaps_; }
    virtual void getGraphicsDescSetBindData();

    virtual_inline const auto& getShadowDescSetBindDataMap() const { return shadowDescSetBindDataMap_; }
    virtual void getShadowDescSetBindData();

   protected:
    Base(Particle::Handler& handler, const index offset, const CreateInfo* pCreateInfo,
         std::shared_ptr<Material::Base>& pMaterial, const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors,
         const GRAPHICS&& shadowPipelineType = GRAPHICS::ALL_ENUM);

    inline std::shared_ptr<Descriptor::Base>& getTimedUniform() { return pDescriptors_[descTimeOffset_]; }

    FlagBits status_;

    bool draw_;
    bool paused_;
    bool doPausedUpdate_;

    glm::uvec3 workgroupSize_;

    std::vector<IndexBufferType> indices_;
    BufferResource indexRes_;
    std::vector<Vertex::Color> vertices_;
    BufferResource vertexRes_;
    std::vector<TEXCOORD> texCoords_;
    BufferResource texCoordRes_;

    std::vector<std::shared_ptr<Descriptor::Base>> pDescriptors_;
    uint32_t descInstOffset_;

    std::shared_ptr<Material::Base> pMaterial_;
    std::vector<Descriptor::Set::bindDataMap> computeDescSetBindDataMaps_;
    std::vector<Descriptor::Set::bindDataMap> graphicsDescSetBindDataMaps_;
    Descriptor::Set::bindDataMap shadowDescSetBindDataMap_;

    std::unique_ptr<LoadingResource> pLdgRes_;

   private:
    virtual void loadBuffers();

    index offset_;
    uint32_t descTimeOffset_;
};

// FOUNTAIN

class Fountain : public Base {
   public:
    Fountain(Particle::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo,
             std::shared_ptr<Material::Base>& pMaterial, const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors,
             std::shared_ptr<::Instance::Base> pInst);

    void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
              const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
              const uint8_t frameIndex) const override;

    void update(const float time, const float elapsed, const uint32_t frameIndex) override;

   private:
    std::shared_ptr<::Instance::Base> pInst_;
};

// EULER
namespace Euler {

enum class VERTEX {
    DONT_CARE = -1,
    BILLBOARD = 0,
    MESH,
    POINT,
};

struct CreateInfo : Buffer::CreateInfo {
    Particle::Euler::PushConstant computeFlag = Particle::Euler::FLAG::FOUNTAIN;
    uint32_t firstInstanceBinding = 1;  // TODO: This was a lazy solution to the problem.
    VERTEX vertexType = VERTEX::DONT_CARE;
};

// BASE
class Base : public Buffer::Base {
   public:
    const VERTEX VERTEX_TYPE;

    Base(Particle::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo,
         std::shared_ptr<Material::Base>& pMaterial, const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors,
         const GRAPHICS&& shadowPipelineType = GRAPHICS::ALL_ENUM);

    void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
              const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
              const uint8_t frameIndex) const override;
    void dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                  const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                  const uint8_t frameIndex) const override;

   private:
    Particle::Euler::PushConstant pushConstant_;
    uint32_t firstInstanceBinding_;
};

// TORUS
class Torus : public Euler::Base {
   public:
    Torus(Particle::Handler& handler, const index&& offset, const Euler::CreateInfo* pCreateInfo,
          std::shared_ptr<Material::Base>& pMaterial, const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors);
};

}  // namespace Euler

}  // namespace Buffer
}  // namespace Particle

#endif  //! PARTICLE_BUFFER_H
