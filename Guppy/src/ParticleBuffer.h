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

    constexpr auto getLastTimeOfBirth() const { return pData_[BUFFER_INFO.count - 1].timeOfBirth; }
};

}  // namespace Fountain
}  // namespace Particle
}  // namespace Instance

// BUFFER

namespace Particle {
class Handler;
namespace Buffer {

class Base : public NonCopyable, public Handlee<Handler> {
   public:
    const std::string NAME;
    const PIPELINE PIPELINE_TYPE;

    virtual void start(const StartInfo& info) = 0;
    virtual void update(const float time, const uint32_t frameIndex) = 0;
    virtual bool shouldDraw(const instancePair& instance) = 0;

    void prepare();
    void destroy();

    constexpr const auto& getOffset() const { return offset_; }
    constexpr const auto& getStatus() const { return status_; }
    constexpr const auto& getInstances() const { return instances_; }

    // TODO: These are directly copied from Mesh::Base
    virtual void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                      const uint8_t frameIndex) const = 0;
    const Descriptor::Set::BindData& getDescriptorSetBindData(const PASS& passType, const instancePair& instance) const;

   protected:
    Base(Particle::Handler& handler, const index offset, const std::string&& name, const PIPELINE& pipelineType,
         const std::vector<std::shared_ptr<Material::Base>>& pMaterials);

    FlagBits status_;

   private:
    void loadBuffers();

    index offset_;
    BufferResource vertexRes_;                  // Not used atm.
    std::vector<Vertex::Color> vertices_;       // Not used atm.
    std::unique_ptr<LoadingResource> pLdgRes_;  // Not used atm.
    std::vector<instancePair> instances_;
};

class Fountain : public Base {
   public:
    Fountain(Particle::Handler& handler, const index&& offset, const ::Particle::Fountain::CreateInfo* pCreateInfo,
             const std::vector<std::shared_ptr<Material::Base>>& pMaterials,
             std::shared_ptr<::Instance::Particle::Fountain::Base>& pInstFntn);

    void start(const StartInfo& info) override;
    void update(const float time, const uint32_t frameIndex) override;
    bool shouldDraw(const instancePair& instance) override;

    void draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
              const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
              const uint8_t frameIndex) const override;

   protected:
    std::shared_ptr<::Instance::Particle::Fountain::Base> pInstFntn_;
};

}  // namespace Buffer
}  // namespace Particle

#endif  //! PARTICLE_BUFFER_H
