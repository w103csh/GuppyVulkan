
#include "ParticleHandler.h"

#include "Shell.h"
// HANDLER
#include "RenderPassHandler.h"
#include "TextureHandler.h"
#include "UniformHandler.h"

Particle::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),  //
      doUpdate_(false),
      instFntnMgr_{"Instance Particle Fountain Data", (2 * 1000000) + 100} {}

void Particle::Handler::init() {
    reset();
    // INSTANCE
    instFntnMgr_.init(shell().context(), settings());
}

void Particle::Handler::tick() {
    // Check loading offsets...
    if (!ldgOffsets_.empty()) {
        for (auto it = ldgOffsets_.begin(); it != ldgOffsets_.end();) {
            auto& pBuffer = pBuffers_.at(*it);

            // Prepare function should be the only way to set ready status atm.
            assert(pBuffer->getStatus() != STATUS::READY);
            pBuffer->prepare();

            if (pBuffer->getStatus() == STATUS::READY)
                it = ldgOffsets_.erase(it);
            else
                ++it;
        }
    }
}

void Particle::Handler::frame() {
    if (!doUpdate_) return;

    const auto frameIndex = passHandler().getFrameIndex();

    // WAVE
    {
        auto& wave = uniformHandler().uniWaveMgr().getTypedItem(0);
        wave.update(static_cast<float>(shell().getCurrentTime()), frameIndex);
        uniformHandler().update(wave, static_cast<int>(frameIndex));
    }

    // BUFFERS

    for (auto& pBuffer : pBuffers_) {
        // for (const auto& instance : pBuffer->getInstances()) {
        //    auto& pMat = std::static_pointer_cast<Material::Particle::Fountain::Base>(instance.second);
        //    if (pMat->getDelta() > 0.0f) {
        //        shell().log(Shell::LOG_DEBUG, ("delta: " + std::to_string(pMat->getDelta())).c_str());
        //    }
        //}
        pBuffer->update(static_cast<float>(shell().getCurrentTime()), frameIndex);
    }
}

void Particle::Handler::create() {
    const auto& dev = shell().context().dev;
    ::Buffer::CreateInfo bufferInfo = {};
    Particle::Fountain::CreateInfo fntnInfo;
    std::vector<glm::mat4> models;

    // WAVE
    {
        bufferInfo = {};
        assert(shell().context().imageCount == 3);  // Potential imageCount problem
        bufferInfo.dataCount = shell().context().imageCount;
        uniformHandler().uniWaveMgr().insert(dev, &bufferInfo);
    }

    // FOUNTAIN
    {
        // BLUEWATER
        {
            fntnInfo = {};
            fntnInfo.pipelineType = PIPELINE::PARTICLE_FOUNTAIN_DEFERRED;
            fntnInfo.pTexture = textureHandler().getTexture(Texture::BLUEWATER_ID);
            fntnInfo.emitterBasis = helpers::makeArbitraryBasis({-1.0f, 2.0f, 0.0f});
            fntnInfo.lifespan = 20.0f;
            fntnInfo.size = 0.1f;

            models.clear();
            models.push_back(helpers::affine(glm::vec3{0.5f}, glm::vec3{1.0f}));
            models.push_back(helpers::affine(glm::vec3{0.5f}, glm::vec3{1.0f}, M_PI_2_FLT, CARDINAL_Y));

            makeBuffer<Particle::Buffer::Fountain, Material::Particle::Fountain::CreateInfo,
                       Instance::Particle::Fountain::CreateInfo>(&fntnInfo, 8000, models);
        }
    }

    // TODO: This is too simple.
    doUpdate_ = true;
}

void Particle::Handler::startFountain(const StartInfo& info) {
    pBuffers_.at(info.bufferOffset)->start(info);  //
}

void Particle::Handler::record(const PASS passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                               const VkCommandBuffer& cmd, const uint8_t frameIndex) {
    //
    switch (pPipelineBindData->type) {
        case PIPELINE::PARTICLE_FOUNTAIN_DEFERRED: {
            // TODO: I know this is slow but its not important atm.
            for (const auto& pBuffer : pBuffers_) {
                if (pBuffer->PIPELINE_TYPE == pPipelineBindData->type) {
                    if (pBuffer->getStatus() == STATUS::READY) {
                        for (const auto& instance : pBuffer->getInstances()) {
                            pBuffer->draw(passType, pPipelineBindData, pBuffer->getDescriptorSetBindData(passType, instance),
                                          cmd, frameIndex);
                        }
                    }
                }
            }
        } break;
        default:
            assert(false && "Unhandled pipeline type");
    }
}

void Particle::Handler::reset() {
    // BUFFER
    for (auto& pBuffer : pBuffers_) pBuffer->destroy();
    pBuffers_.clear();
    // INSTANCE
    instFntnMgr_.destroy(shell().context().dev);
}