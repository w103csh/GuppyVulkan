#ifndef PARTICLE_HANDLER_H
#define PARTICLE_HANDLER_H

#include <memory>
#include <vector>

#include "ConstantsAll.h"
#include "Game.h"
#include "Instance.h"
#include "InstanceManager.h"
#include "MaterialHandler.h"  // This is sketchy...
#include "ParticleBuffer.h"
#include "Types.h"

namespace Particle {

constexpr uint32_t NUM_PARTICLES_EULER_MIN = 1024;

class Handler : public Game::Handler {
    friend class Buffer::Base;

   private:
    template <class TBufferType, class TBufferBaseType, typename TBufferCreateInfo, typename... TArgs>
    auto &make(std::vector<TBufferBaseType> &pBuffers, TBufferCreateInfo *pCreateInfo,
               const std::vector<std::shared_ptr<Material::Base>> &pMaterials, TArgs... args) {
        pBuffers.emplace_back(
            new TBufferType(std::ref(*this), static_cast<Buffer::index>(pBuffers.size()), pCreateInfo, pMaterials, args...));
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
    void startFountain(const StartInfo &info);

    // Assume all isntance data derived from Fountain::Base for now.
    template <typename TInstCreateInfo>
    std::shared_ptr<Instance::Particle::Fountain::Base> &makeInstanceParticleFountain(TInstCreateInfo *pInfo) {
        assert(pInfo != nullptr);
        std::shared_ptr<Instance::Particle::Fountain::Base> &pInst = pInfo->pSharedData;
        if (pInst == nullptr) {
            // assert(pInfo->data.size());
            instFntnMgr_.insert(shell().context().dev, pInfo);
            pInst = instFntnMgr_.pItems.back();
        }
        return pInst;
    }

    template <class TBufferType, typename TMaterialCreateInfo, typename TInstanceCreateInfo, typename TCreateInfo>
    auto &makeBuffer(TCreateInfo *pCreateInfo, const uint32_t numberOfParticles,
                     const std::vector<glm::mat4> &models = {glm::mat4{1.0f}}) {
        // MATERIAL
        std::vector<std::shared_ptr<Material::Base>> pMaterials;
        for (const auto &model : models) {
            TMaterialCreateInfo matInfo(pCreateInfo, model, shell().context().imageCount);
            pMaterials.emplace_back(materialHandler().makeMaterial(&matInfo));
        }
        // INSTANCE
        TInstanceCreateInfo instInfo(pCreateInfo, numberOfParticles);
        // Assume all isntance data derived from Fountain::Base for now.
        auto &pInstFntn = makeInstanceParticleFountain(&instInfo);

        return make<TBufferType>(pBuffers_, pCreateInfo, pMaterials, pInstFntn);
    }

    inline void updateInstanceData(const ::Buffer::Info &info) { instFntnMgr_.updateData(shell().context().dev, info); }

    template <typename TParticle>
    inline void updateParticle(std::unique_ptr<TParticle> &pBuffer) {
        instFntnMgr_.updateData(shell().context().dev, pBuffer->pInstanceData_->BUFFER_INFO);
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
    // INSTANCE
    Instance::Manager<Instance::Particle::Fountain::Base, Instance::Particle::Fountain::Base> instFntnMgr_;
    std::unique_ptr<Instance::Manager<Instance::Particle::FountainEuler::Base, Instance::Particle::FountainEuler::Base>>
        pInstFntnEulerMgr_;
    // LOADING
    std::unordered_set<Particle::Buffer::index> ldgOffsets_;
};

}  // namespace Particle

#endif  // !PARTICLE_HANDLER_H
