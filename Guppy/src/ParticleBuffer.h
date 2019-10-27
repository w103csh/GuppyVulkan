#ifndef PARTICLE_BUFFER_H
#define PARTICLE_BUFFER_H

#include <glm/glm.hpp>
#include <string>
#include <vector>

#include "ConstantsAll.h"
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
    CreateInfo(const ::Particle::Fountain::CreateInfo* pCreateInfo, const uint32_t numberOfParticles) : pInfo(pCreateInfo) {
        dataCount = numberOfParticles;
    }
    const ::Particle::Fountain::CreateInfo* pInfo;
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
    CreateInfo(const ::Particle::Fountain::CreateInfo* pCreateInfo, const uint32_t numberOfParticles) : pInfo(pCreateInfo) {
        dataCount = numberOfParticles;
    }
    const ::Particle::Fountain::CreateInfo* pInfo;
};

class Base : public Buffer::DataItem<DATA>, public Instance::Base {
   public:
    Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo);
};

}  // namespace FountainEuler

}  // namespace Particle
}  // namespace Instance

// BUFFER

namespace Particle {
class Handler;
namespace Buffer {

// BASE

class Base : public NonCopyable, public Handlee<Handler> {
   public:
    virtual ~Base() = default;

    const std::string NAME;
    const PIPELINE PIPELINE_TYPE_COMPUTE;
    const PIPELINE PIPELINE_TYPE_GRAPHICS;
    const PIPELINE PIPELINE_TYPE_SHADOW;

    virtual void start(const StartInfo& info) = 0;
    virtual void update(const float time, const float elapsed, const uint32_t frameIndex) = 0;
    virtual bool shouldDraw(const InstanceInfo& instance) = 0;

    void prepare();
    void destroy();

    constexpr const auto& getOffset() const { return offset_; }
    constexpr const auto& getStatus() const { return status_; }
    constexpr const auto& getInstances() const { return instances_; }

    virtual const ::Buffer::Item* getInstanceBufferItem() const = 0;

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
    Base(Particle::Handler& handler, const index offset, const std::string&& name,
         const ::Particle::Fountain::CreateInfo* pCreateInfo, const std::vector<std::shared_ptr<Material::Base>>& pMaterials,
         const PIPELINE pipelineTypeShadow = PIPELINE::ALL_ENUM);

    FlagBits status_;

    std::vector<VB_INDEX_TYPE> indices_;
    BufferResource indexRes_;
    std::vector<Vertex::Color> vertices_;
    BufferResource vertexRes_;
    std::vector<InstanceInfo> instances_;

   private:
    void loadBuffers();

    index offset_;
    std::unique_ptr<LoadingResource> pLdgRes_;
};

// FOUNTAIN

class Fountain : public Base {
   public:
    Fountain(Particle::Handler& handler, const index&& offset, const ::Particle::Fountain::CreateInfo* pCreateInfo,
             const std::vector<std::shared_ptr<Material::Base>>& pMaterials,
             std::shared_ptr<::Instance::Particle::Fountain::Base>& pInstFntn);

    void start(const StartInfo& info) override;
    void update(const float time, const float elapsed, const uint32_t frameIndex) override;
    bool shouldDraw(const InstanceInfo& instance) override;

    const ::Buffer::Item* getInstanceBufferItem() const override { return pInstFntn_.get(); }

    void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
              const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
              const uint8_t frameIndex) const override;

   protected:
    std::shared_ptr<::Instance::Particle::Fountain::Base> pInstFntn_;
};

// FOUNTAIN (EULER)

class FountainEuler : public Base {
   public:
    FountainEuler(Particle::Handler& handler, const index&& offset, const ::Particle::Fountain::CreateInfo* pCreateInfo,
                  const std::vector<std::shared_ptr<Material::Base>>& pMaterials,
                  std::shared_ptr<::Instance::Particle::FountainEuler::Base>& pInstFntn);

    void start(const StartInfo& info) override;
    void update(const float time, const float elapsed, const uint32_t frameIndex) override;
    bool shouldDraw(const InstanceInfo& instance) override;

    const ::Buffer::Item* getInstanceBufferItem() const override { return pInstFntn_.get(); }

    void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
              const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
              const uint8_t frameIndex) const override;
    void dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                  const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                  const uint8_t frameIndex) const override;

   protected:
    std::shared_ptr<::Instance::Particle::FountainEuler::Base> pInstFntn_;

   private:
    bool doTorus_;
};

}  // namespace Buffer
}  // namespace Particle

#endif  //! PARTICLE_BUFFER_H
