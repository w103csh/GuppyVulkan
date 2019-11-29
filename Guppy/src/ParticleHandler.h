#ifndef PARTICLE_HANDLER_H
#define PARTICLE_HANDLER_H

#include <memory>
#include <unordered_set>
#include <vector>

#include "ConstantsAll.h"
#include "Cloth.h"
#include "Descriptor.h"
#include "DescriptorManager.h"
#include "Game.h"
#include "HeightFieldFluid.h"
#include "Instance.h"
#include "InstanceManager.h"
#include "MaterialHandler.h"  // This is sketchy...
#include "ParticleBuffer.h"
#include "Storage.h"
#include "Types.h"
#include "Uniform.h"

namespace Particle {

class Handler : public Game::Handler {
    friend class Buffer::Base;

   private:
    template <class TBufferType, class TBufferBaseType, typename TBufferCreateInfo, typename... TArgs>
    auto &make(std::vector<TBufferBaseType> &pBuffers, TBufferCreateInfo *pCreateInfo,
               std::shared_ptr<Material::Base> &pMaterial,
               const std::vector<std::shared_ptr<Descriptor::Base>> &pDescriptors, TArgs... args) {
        pBuffers.emplace_back(new TBufferType(std::ref(*this), static_cast<Buffer::index>(pBuffers.size()), pCreateInfo,
                                              pMaterial, pDescriptors, args...));
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

    inline void toggleDraw() {
        for (auto &pBuffer : pBuffers_) pBuffer->toggleDraw();
    }
    inline void togglePause() {
        for (auto &pBuffer : pBuffers_) pBuffer->togglePause();
    }

    inline void update(std::shared_ptr<Descriptor::Base> &pUniform, const int index = -1) {
        if (pUniform->getDescriptorType() == DESCRIPTOR{UNIFORM_DYNAMIC::PRTCL_ATTRACTOR}) {
            prtclAttrMgr.updateData(shell().context().dev, pUniform->BUFFER_INFO, index);
        } else if (pUniform->getDescriptorType() == DESCRIPTOR{UNIFORM_DYNAMIC::PRTCL_FOUNTAIN}) {
            prtclFntnMgr.updateData(shell().context().dev, pUniform->BUFFER_INFO, index);
        } else if (pUniform->getDescriptorType() == DESCRIPTOR{UNIFORM_DYNAMIC::PRTCL_CLOTH}) {
            prtclClthMgr.updateData(shell().context().dev, pUniform->BUFFER_INFO, index);
        } else if (pUniform->getDescriptorType() == DESCRIPTOR{UNIFORM_DYNAMIC::MATRIX_4}) {
            mat4Mgr.updateData(shell().context().dev, pUniform->BUFFER_INFO, index);
        } else if (pUniform->getDescriptorType() == DESCRIPTOR{UNIFORM_DYNAMIC::HFF}) {
            hffMgr.updateData(shell().context().dev, pUniform->BUFFER_INFO, index);
        } else {
            assert(false && "Unhandled uniform type");
            exit(EXIT_FAILURE);
        }
    }

    void recordDraw(const PASS passType, const std::shared_ptr<Pipeline::BindData> &pPipelineBindData,
                    const VkCommandBuffer &cmd, const uint8_t frameIndex);
    void recordDispatch(const PASS passType, const std::shared_ptr<Pipeline::BindData> &pPipelineBindData,
                        const VkCommandBuffer &cmd, const uint8_t frameIndex);

    Descriptor::Manager<Descriptor::Base, UniformDynamic::Particle::Attractor::Base, std::shared_ptr> prtclAttrMgr;
    Descriptor::Manager<Descriptor::Base, UniformDynamic::Particle::Cloth::Base, std::shared_ptr> prtclClthMgr;
    Descriptor::Manager<Descriptor::Base, UniformDynamic::Particle::Fountain::Base, std::shared_ptr> prtclFntnMgr;
    Descriptor::Manager<Descriptor::Base, UniformDynamic::Matrix4::Base, std::shared_ptr> mat4Mgr;
    Descriptor::Manager<Descriptor::Base, Storage::Vector4::Base, std::shared_ptr> vec4Mgr;
    Descriptor::Manager<Descriptor::Base, UniformDynamic::HeightFieldFluid::Simulation::Base, std::shared_ptr> hffMgr;

   private:
    void reset() override;
    inline bool hasInstFntnEulerMgr() const { return pInstFntnEulerMgr_ != nullptr; }

    bool doUpdate_;

    // BUFFERS
    std::vector<std::unique_ptr<Particle::Buffer::Base>> pBuffers_;

    // INSTANCE DATA
    Instance::Manager<Particle::Fountain::Base, Particle::Fountain::Base> instFntnMgr_;
    std::unique_ptr<Descriptor::Manager<Descriptor::Base, Particle::FountainEuler::Base, std::shared_ptr>>
        pInstFntnEulerMgr_;

    // LOADING
    std::unordered_set<Particle::Buffer::index> ldgOffsets_;
};

}  // namespace Particle

#endif  // !PARTICLE_HANDLER_H
