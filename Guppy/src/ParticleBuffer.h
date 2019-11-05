#ifndef PARTICLE_BUFFER_H
#define PARTICLE_BUFFER_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "ConstantsAll.h"
#include "Descriptor.h"
#include "Handlee.h"
#include "Instance.h"
#include "Particle.h"

// INSTANCE

namespace Instance {
namespace Particle {

// FOUNTAIN
namespace Fountain {

struct DATA {
    static void getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes);
    glm::vec3 velocity;
    float timeOfBirth;
};

class Base;

struct CreateInfo : Instance::CreateInfo<DATA, Base> {
    CreateInfo(const UniformDynamic::Particle::Fountain::CreateInfo* pCreateInfo, const uint32_t numberOfParticles)
        : pInfo(pCreateInfo) {
        countInRange = true;
        dataCount = numberOfParticles;
    }
    const UniformDynamic::Particle::Fountain::CreateInfo* pInfo;
};

class Base : public Buffer::DataItem<DATA>, public Instance::Base {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);

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

struct CreateInfo : Instance::CreateInfo<DATA, Base> {
    CreateInfo(const UniformDynamic::Particle::Fountain::CreateInfo* pCreateInfo, const uint32_t numberOfParticles)
        : pInfo(pCreateInfo) {
        dataCount = numberOfParticles;
    }
    const UniformDynamic::Particle::Fountain::CreateInfo* pInfo;
};

class Base : public Buffer::DataItem<DATA>, public Instance::Base {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);
};

}  // namespace FountainEuler

// VECTOR4
namespace Vector4 {

enum class TYPE {
    ATTRACTOR_POSITION,
    ATTRACTOR_VELOCITY,
};

struct DATA {
    static void getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes);
    glm::vec4 data;
};

class Base;

struct CreateInfo : Instance::CreateInfo<DATA, Base> {
    CreateInfo(const TYPE&& type, const STORAGE_BUFFER_DYNAMIC&& descType, const glm::uvec3& numParticles)
        : type(type), descType(descType), numParticles(numParticles) {
        countInRange = true;
        dataCount = numParticles.x * numParticles.y * numParticles.z;
    }
    TYPE type;
    STORAGE_BUFFER_DYNAMIC descType;
    glm::uvec3 numParticles;
};

class Base : public Buffer::DataItem<DATA>, public Instance::Base {
   public:
    const TYPE VECTOR_TYPE;
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);
};

}  // namespace Vector4

}  // namespace Particle
}  // namespace Instance

// BUFFER

namespace Particle {
class Handler;
namespace Buffer {

struct CreateInfo {
    std::string name = "";
    PIPELINE pipelineTypeGraphics = PIPELINE::ALL_ENUM;
    PIPELINE pipelineTypeCompute = PIPELINE::ALL_ENUM;
};

// BASE

class Base : public NonCopyable, public Handlee<Handler> {
   public:
    virtual ~Base() = default;

    const std::string NAME;
    const PIPELINE PIPELINE_TYPE_COMPUTE;
    const PIPELINE PIPELINE_TYPE_GRAPHICS;
    const PIPELINE PIPELINE_TYPE_SHADOW;

    void toggle();
    virtual void update(const float time, const float elapsed, const uint32_t frameIndex);
    bool shouldDraw();

    void prepare();
    void destroy();

    constexpr const auto& getOffset() const { return offset_; }
    constexpr const auto& getStatus() const { return status_; }
    constexpr const auto& getInstances() const { return instances_; }

    const std::vector<Descriptor::Base*> getSDynamicDataItems(const PIPELINE pipelineType,
                                                              const InstanceInfo& instance) const;

    // TODO: These are directly copied from Mesh::Base. Changed getDescriptorSetBindData slightly.
    virtual void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                      const uint8_t frameIndex) const = 0;
    virtual void dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                          const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                          const uint8_t frameIndex) const {}
    const Descriptor::Set::BindData& getDescriptorSetBindData(const PASS& passType,
                                                              const Descriptor::Set::bindDataMap& map) const;

   protected:
    Base(Particle::Handler& handler, const index offset, const CreateInfo* pCreateInfo,
         const std::vector<std::shared_ptr<Material::Obj3d::Default>>& pObj3dMaterials,
         std::vector<std::shared_ptr<Descriptor::Base>>&& pUniforms,
         const PIPELINE&& pipelineTypeShadow = PIPELINE::ALL_ENUM);

    inline std::shared_ptr<Descriptor::Base>& getTimedUniform() { return pUniforms_[timeUniformOffset_]; }
    virtual_inline auto& getInstUniformOffsets() const { return instUniformOffsets_; }

    FlagBits status_;

    std::vector<VB_INDEX_TYPE> indices_;
    BufferResource indexRes_;
    std::vector<Vertex::Color> vertices_;
    BufferResource vertexRes_;
    std::vector<InstanceInfo> instances_;

    bool paused_;
    std::vector<std::shared_ptr<Descriptor::Base>> pUniforms_;

   private:
    void loadBuffers();

    index offset_;
    std::unique_ptr<LoadingResource> pLdgRes_;

    uint32_t timeUniformOffset_;
    std::array<uint32_t, 2> instUniformOffsets_;
};

// FOUNTAIN

class Fountain : public Base {
   public:
    Fountain(Particle::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo,
             const std::vector<std::shared_ptr<Material::Obj3d::Default>>& pObj3dMaterials,
             std::vector<std::shared_ptr<Descriptor::Base>>&& pUniforms);

    void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
              const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
              const uint8_t frameIndex) const override;

    void update(const float time, const float elapsed, const uint32_t frameIndex) override;
};

// EULER
namespace Euler {

struct CreateInfo : Buffer::CreateInfo {
    Particle::Euler::PushConstant computeFlag = Particle::Euler::FLAG::FOUNTAIN;
    glm::uvec3 localSize{1, 1, 1};
    uint32_t firstInstanceBinding = 1;  // TODO: This was a lazy solution to the problem.
};

// BASE
class Base : public Buffer::Base {
   public:
    Base(Particle::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo,
         const std::vector<std::shared_ptr<Material::Obj3d::Default>>& pObj3dMaterials,
         std::vector<std::shared_ptr<Descriptor::Base>>&& pUniforms,
         const PIPELINE&& pipelineTypeShadow = PIPELINE::ALL_ENUM);

    const glm::uvec3 LOCAL_SIZE;

    void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
              const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
              const uint8_t frameIndex) const override;
    void dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                  const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                  const uint8_t frameIndex) const override;

   private:
    Particle::Euler::PushConstant pushConstant_;
    uint32_t firstInstanceBinding_;
};

// TORUS
class Torus : public Euler::Base {
   public:
    Torus(Particle::Handler& handler, const index&& offset, const Euler::CreateInfo* pCreateInfo,
          const std::vector<std::shared_ptr<Material::Obj3d::Default>>& pObj3dMaterials,
          std::vector<std::shared_ptr<Descriptor::Base>>&& pUniforms);
};

}  // namespace Euler

}  // namespace Buffer
}  // namespace Particle

#endif  //! PARTICLE_BUFFER_H
