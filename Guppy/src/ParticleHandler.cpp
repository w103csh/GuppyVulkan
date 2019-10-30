
#include "ParticleHandler.h"

#include "Shell.h"
// HANDLER
#include "RenderPassHandler.h"
#include "TextureHandler.h"
#include "UniformHandler.h"

namespace {
constexpr uint32_t NUM_PARTICLES_BW_EULER = Particle::NUM_PARTICLES_EULER_MIN;
constexpr uint32_t NUM_PARTICLES_FIRE = Particle::NUM_PARTICLES_EULER_MIN * 2;
constexpr uint32_t NUM_PARTICLES_SMOKE = Particle::NUM_PARTICLES_EULER_MIN;
}  // namespace

Particle::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),  //
      doUpdate_(false),
      instFntnMgr_{"Instance Particle Fountain Data", 8000 * 5},
      pInstFntnEulerMgr_(nullptr) {}

void Particle::Handler::init() {
    reset();

    instFntnMgr_.init(shell().context());
    if (hasInstFntnEulerMgr()) pInstFntnEulerMgr_->init(shell().context());

    // Unfortunately the textures have to be created inside init
    auto rand1dTexInfo =
        Texture::MakeRandom1dTex(Texture::Particle::RAND_1D_ID, Sampler::Particle::RAND_1D_ID, NUM_PARTICLES_FIRE * 4);
    textureHandler().make(&rand1dTexInfo);
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
        wave.update(shell().getCurrentTime<float>(), frameIndex);
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
        pBuffer->update(shell().getCurrentTime<float>(), shell().getElapsedTime<float>(), frameIndex);
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
        if (true) {
            fntnInfo = {};
            fntnInfo.name = "Bluewater Fountain Particle Buffer";
            fntnInfo.pipelineTypeGraphics = PIPELINE::PARTICLE_FOUNTAIN_DEFERRED;
            fntnInfo.pTexture = textureHandler().getTexture(Texture::BLUEWATER_ID);
            fntnInfo.emitterBasis = helpers::makeArbitraryBasis({-1.0f, 2.0f, 0.0f});
            fntnInfo.lifespan = 20.0f;
            fntnInfo.minParticleSize = 0.1f;

            models.clear();
            models.push_back(helpers::affine(glm::vec3{0.5f}, glm::vec3{1.0f}));
            models.push_back(helpers::affine(glm::vec3{0.5f}, glm::vec3{-3.0f, 0.0f, 0.0f}, M_PI_2_FLT, CARDINAL_Y));

            makeBuffer<Particle::Buffer::Fountain, Material::Particle::Fountain::CreateInfo,
                       Instance::Particle::Fountain::CreateInfo>(&fntnInfo, 8000, models);
        }
        // EULER (COMPUTE)
        if (hasInstFntnEulerMgr()) {
            // TORUS
            if (true) {
                fntnInfo = {};
                fntnInfo.name = "Bluewater Euler Particle Buffer";
                fntnInfo.pipelineTypeCompute = PIPELINE::PARTICLE_EULER_COMPUTE;
                fntnInfo.pipelineTypeGraphics = PIPELINE::PARTICLE_FOUNTAIN_EULER_DEFERRED;
                fntnInfo.pTexture = textureHandler().getTexture(Texture::BLUEWATER_ID);
                fntnInfo.emitterBasis = helpers::makeArbitraryBasis({-1.0f, 2.0f, 0.0f});
                fntnInfo.lifespan = 8.0f;
                fntnInfo.minParticleSize = 0.1f;

                models.clear();
                // models.push_back(helpers::affine(glm::vec3{0.07f}, glm::vec3{1.0f, 4.0f, -1.0f}, M_PI_FLT, CARDINAL_X));
                models.push_back(helpers::affine(glm::vec3{0.07f}, glm::vec3{-1.0f, 0.2f, -1.0f}));

                {
                    // MATERIAL
                    std::vector<std::shared_ptr<Material::Base>> pMaterials;
                    for (const auto& model : models) {
                        Material::Particle::Fountain::CreateInfo matInfo(&fntnInfo, model, shell().context().imageCount);
                        matInfo.color = {0.8f, 0.3f, 0.0f};
                        matInfo.velocityLowerBound = 2.0f;
                        matInfo.velocityUpperBound = 3.5f;
                        pMaterials.emplace_back(materialHandler().makeMaterial(&matInfo));
                    }
                    // INSTANCE
                    Instance::Particle::FountainEuler::CreateInfo instInfo(&fntnInfo, NUM_PARTICLES_BW_EULER);
                    instInfo.countInRange = true;
                    pInstFntnEulerMgr_->insert(shell().context().dev, &instInfo);
                    auto& pInstFntn = pInstFntnEulerMgr_->pItems.back();
                    // BUFFER
                    make<Particle::Buffer::Euler::Torus>(pBuffers_, &fntnInfo, pMaterials, pInstFntn);
                }
            }
            // FIRE
            if (true) {
                fntnInfo = {};
                fntnInfo.name = "Fire Euler Particle Buffer";
                fntnInfo.pipelineTypeCompute = PIPELINE::PARTICLE_EULER_COMPUTE;
                fntnInfo.pipelineTypeGraphics = PIPELINE::PARTICLE_FOUNTAIN_EULER_DEFERRED;
                fntnInfo.pTexture = textureHandler().getTexture(Texture::FIRE_ID);
                fntnInfo.emitterBasis = helpers::makeArbitraryBasis({0.0f, 1.0f, 0.0f});
                fntnInfo.lifespan = 3.0f;
                fntnInfo.minParticleSize = 0.5f;
                fntnInfo.acceleration = {0.0f, 0.1f, 0.0f};
                fntnInfo.computeFlag = Particle::Euler::FLAG::FIRE;

                models.clear();
                models.push_back(helpers::affine(glm::vec3{1.0f}, glm::vec3{-3.0f, 0.0f, -3.0f}));
                models.push_back(helpers::affine(glm::vec3{1.0f}, glm::vec3{-0.5f, 2.0f, 1.0f}));

                // TODO: add this to make template specialization
                {
                    // MATERIAL
                    std::vector<std::shared_ptr<Material::Base>> pMaterials;
                    for (const auto& model : models) {
                        Material::Particle::Fountain::CreateInfo matInfo(&fntnInfo, model, shell().context().imageCount);
                        // matInfo.color = {0.8f, 0.3f, 0.0f};
                        // matInfo.velocityLowerBound = 2.0f;
                        // matInfo.velocityUpperBound = 3.5f;
                        pMaterials.emplace_back(materialHandler().makeMaterial(&matInfo));
                    }
                    // INSTANCE
                    Instance::Particle::FountainEuler::CreateInfo instInfo(&fntnInfo, NUM_PARTICLES_FIRE);
                    instInfo.countInRange = true;
                    pInstFntnEulerMgr_->insert(shell().context().dev, &instInfo);
                    auto& pInstFntn = pInstFntnEulerMgr_->pItems.back();
                    // BUFFER
                    make<Particle::Buffer::Euler::Base>(pBuffers_, &fntnInfo, pMaterials, pInstFntn);
                }
            }
            // SMOKE
            if (true) {
                fntnInfo = {};
                fntnInfo.name = "Smoke Euler Particle Buffer";
                fntnInfo.pipelineTypeCompute = PIPELINE::PARTICLE_EULER_COMPUTE;
                fntnInfo.pipelineTypeGraphics = PIPELINE::PARTICLE_FOUNTAIN_EULER_DEFERRED;
                fntnInfo.pTexture = textureHandler().getTexture(Texture::SMOKE_ID);
                fntnInfo.emitterBasis = helpers::makeArbitraryBasis({0.0f, 1.0f, 0.0f});
                fntnInfo.lifespan = 10.0f;
                fntnInfo.minParticleSize = 0.1f;
                fntnInfo.maxParticleSize = 2.5f;
                fntnInfo.acceleration = {0.0f, 0.1f, 0.0f};
                fntnInfo.computeFlag = Particle::Euler::FLAG::SMOKE;

                models.clear();
                models.push_back(helpers::affine(glm::vec3{1.0f}, glm::vec3{-0.5f, 2.0f, 1.0f}));

                // TODO: add this to make template specialization
                {
                    // MATERIAL
                    std::vector<std::shared_ptr<Material::Base>> pMaterials;
                    for (const auto& model : models) {
                        Material::Particle::Fountain::CreateInfo matInfo(&fntnInfo, model, shell().context().imageCount);
                        // matInfo.color = {0.8f, 0.3f, 0.0f};
                        // matInfo.velocityLowerBound = 2.0f;
                        // matInfo.velocityUpperBound = 3.5f;
                        pMaterials.emplace_back(materialHandler().makeMaterial(&matInfo));
                    }
                    // INSTANCE
                    Instance::Particle::FountainEuler::CreateInfo instInfo(&fntnInfo, NUM_PARTICLES_SMOKE);
                    instInfo.countInRange = true;
                    pInstFntnEulerMgr_->insert(shell().context().dev, &instInfo);
                    auto& pInstFntn = pInstFntnEulerMgr_->pItems.back();
                    // BUFFER
                    make<Particle::Buffer::Euler::Base>(pBuffers_, &fntnInfo, pMaterials, pInstFntn);
                }
            }
        }
    }

    // TODO: This is too simple.
    doUpdate_ = true;
}

void Particle::Handler::startFountain(const StartInfo& info) {
    pBuffers_.at(info.bufferOffset)->start(info);  //
}

void Particle::Handler::recordDraw(const PASS passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                                   const VkCommandBuffer& cmd, const uint8_t frameIndex) {
    // TODO: I know this is slow but its not important atm.
    for (const auto& pBuffer : pBuffers_) {
        if (pBuffer->PIPELINE_TYPE_GRAPHICS == pPipelineBindData->type) {
            if (pBuffer->getStatus() == STATUS::READY) {
                for (const auto& instance : pBuffer->getInstances()) {
                    if (pBuffer->shouldDraw(instance)) {
                        pBuffer->draw(passType, pPipelineBindData,
                                      pBuffer->getDescriptorSetBindData(passType, instance.graphicsDescSetBindDataMap), cmd,
                                      frameIndex);
                    }
                }
            }
        }
        // This is how lazy I am right now...
        if (pBuffer->PIPELINE_TYPE_SHADOW == pPipelineBindData->type) {
            if (pBuffer->getStatus() == STATUS::READY) {
                for (const auto& instance : pBuffer->getInstances()) {
                    if (pBuffer->shouldDraw(instance)) {
                        pBuffer->draw(passType, pPipelineBindData,
                                      pBuffer->getDescriptorSetBindData(passType, instance.shadowDescSetBindDataMap), cmd,
                                      frameIndex);
                    }
                }
            }
        }
    }
}

void Particle::Handler::recordDispatch(const PASS passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                                       const VkCommandBuffer& cmd, const uint8_t frameIndex) {
    // TODO: I know this is slow but its not important atm.
    for (const auto& pBuffer : pBuffers_) {
        if (pBuffer->PIPELINE_TYPE_COMPUTE == pPipelineBindData->type) {
            if (pBuffer->getStatus() == STATUS::READY) {
                for (const auto& instance : pBuffer->getInstances()) {
                    if (pBuffer->shouldDraw(instance)) {
                        pBuffer->dispatch(passType, pPipelineBindData,
                                          pBuffer->getDescriptorSetBindData(passType, instance.computeDescSetBindDataMap),
                                          cmd, frameIndex);
                    }
                }
            }
        }
    }
}

void Particle::Handler::reset() {
    // BUFFER
    for (auto& pBuffer : pBuffers_) pBuffer->destroy();
    pBuffers_.clear();
    // INSTANCE
    // Fountain
    instFntnMgr_.destroy(shell().context().dev);
    // FountainEuler
    if (pInstFntnEulerMgr_ == nullptr && shell().context().computeShadingEnabled) {
        pInstFntnEulerMgr_ = std::make_unique<
            Instance::Manager<Instance::Particle::FountainEuler::Base, Instance::Particle::FountainEuler::Base>>(
            "Instance Particle Fountain Data", (4000 * 5) * 2, false,
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT));
    } else if (pInstFntnEulerMgr_ != nullptr) {
        pInstFntnEulerMgr_->destroy(shell().context().dev);
        if (!shell().context().computeShadingEnabled) pInstFntnEulerMgr_ = nullptr;
    }
}