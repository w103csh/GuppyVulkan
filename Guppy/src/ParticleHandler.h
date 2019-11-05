#ifndef PARTICLE_HANDLER_H
#define PARTICLE_HANDLER_H

#include <memory>
#include <unordered_set>
#include <vector>

#include "ConstantsAll.h"
#include "Descriptor.h"
#include "DescriptorManager.h"
#include "Game.h"
#include "Instance.h"
#include "InstanceManager.h"
#include "MaterialHandler.h"  // This is sketchy...
#include "ParticleBuffer.h"
#include "Types.h"

namespace Particle {

class Handler : public Game::Handler {
    friend class Buffer::Base;

   private:
    template <class TBufferType, class TBufferBaseType, typename TBufferCreateInfo>
    auto &make(std::vector<TBufferBaseType> &pBuffers, TBufferCreateInfo *pCreateInfo,
               const std::vector<std::shared_ptr<Material::Obj3d::Default>> &pObj3dMaterials,
               std::vector<std::shared_ptr<Descriptor::Base>> &&pUniforms) {
        pBuffers.emplace_back(new TBufferType(std::ref(*this), static_cast<Buffer::index>(pBuffers.size()), pCreateInfo,
                                              pObj3dMaterials,
                                              std::forward<std::vector<std::shared_ptr<Descriptor::Base>>>(pUniforms)));
        assert(pBuffers.back()->getOffset() == pBuffers.size() - 1);

        if (pBuffers.back()->getStatus() == STATUS::READY || pBuffers.back()->getStatus() & STATUS::PENDING_MATERIAL ||
            pBuffers.back()->getStatus() & STATUS::PENDING_BUFFERS) {
            pBuffers.back()->prepare();
        } else {
            assert(false && "Invalid particle buffer status after instantiation");
            exit(EXIT_FAILURE);
        }

        return pBuffers.back();
    }

   public:
    Handler(Game *pGame);

    void init() override;
    void tick() override;
    void frame() override;

    void create();
    void startFountain(const uint32_t offset);

    inline void update(std::shared_ptr<Descriptor::Base> &pUniform, const int index = -1) {
        if (pUniform->getDescriptorType() == DESCRIPTOR{UNIFORM_DYNAMIC::PRTCL_ATTRACTOR}) {
            prtclAttrMgr_.updateData(shell().context().dev, pUniform->BUFFER_INFO, index);
        } else if (pUniform->getDescriptorType() == DESCRIPTOR{UNIFORM_DYNAMIC::PRTCL_FOUNTAIN}) {
            prtclFntnMgr_.updateData(shell().context().dev, pUniform->BUFFER_INFO, index);
        } else {
            assert(false && "Unhandled uniform type");
            exit(EXIT_FAILURE);
        }
    }

    void recordDraw(const PASS passType, const std::shared_ptr<Pipeline::BindData> &pPipelineBindData,
                    const VkCommandBuffer &cmd, const uint8_t frameIndex);
    void recordDispatch(const PASS passType, const std::shared_ptr<Pipeline::BindData> &pPipelineBindData,
                        const VkCommandBuffer &cmd, const uint8_t frameIndex);

   private:
    void reset() override;
    inline bool hasInstFntnEulerMgr() const { return pInstFntnEulerMgr_ != nullptr; }

    bool doUpdate_;

    // BUFFERS
    std::vector<std::unique_ptr<Particle::Buffer::Base>> pBuffers_;

    // UNIFORM DYNAMIC
    Descriptor::Manager<Descriptor::Base, UniformDynamic::Particle::Attractor::Base, std::shared_ptr> prtclAttrMgr_;
    Descriptor::Manager<Descriptor::Base, UniformDynamic::Particle::Fountain::Base, std::shared_ptr> prtclFntnMgr_;

    // INSTANCE
    Instance::Manager<Instance::Particle::Fountain::Base, Instance::Particle::Fountain::Base> instFntnMgr_;
    Instance::Manager<Instance::Particle::Vector4::Base, Instance::Particle::Vector4::Base> instVec4Mgr_;
    std::unique_ptr<Instance::Manager<Instance::Particle::FountainEuler::Base, Instance::Particle::FountainEuler::Base>>
        pInstFntnEulerMgr_;
    // LOADING
    std::unordered_set<Particle::Buffer::index> ldgOffsets_;
};

}  // namespace Particle

#endif  // !PARTICLE_HANDLER_H
