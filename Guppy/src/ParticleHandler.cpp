
#include "ParticleHandler.h"

#include "Shell.h"
// HANDLER
#include "RenderPassHandler.h"
#include "TextureHandler.h"
#include "UniformHandler.h"

namespace {
constexpr uint32_t NUM_PARTICLES_BLUEWATER = 8000;
constexpr uint32_t NUM_PARTICLES_TORUS = 64;
constexpr uint32_t NUM_PARTICLES_FIRE = 2000;
constexpr uint32_t NUM_PARTICLES_SMOKE = 1000;
constexpr glm::uvec3 NUM_PARTICLES_ATTR{50, 50, 50};
constexpr uint32_t NUM_PARTICLES_ATTR_TOTAL = NUM_PARTICLES_ATTR.x * NUM_PARTICLES_ATTR.y * NUM_PARTICLES_ATTR.z;
}  // namespace

Particle::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),  //
      doUpdate_(false),
      prtclAttrMgr_{"Particle Attractor", UNIFORM_DYNAMIC::PRTCL_ATTRACTOR, 3, true, "_UD_PRTCL_ATTR"},
      prtclFntnMgr_{"Particle Fountain", UNIFORM_DYNAMIC::PRTCL_FOUNTAIN, 30, true, "_UD_PRTCL_ATTR"},
      instFntnMgr_{"Instance Particle Fountain Data", 8000 * 5, false},
      instVec4Mgr_{
          "Instance Particle Attractor Data", NUM_PARTICLES_ATTR_TOTAL * 2, false,
          static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)},
      pInstFntnEulerMgr_(nullptr) {}

void Particle::Handler::init() {
    reset();

    prtclAttrMgr_.init(shell().context());
    prtclFntnMgr_.init(shell().context());
    instFntnMgr_.init(shell().context());
    instVec4Mgr_.init(shell().context());
    if (hasInstFntnEulerMgr()) pInstFntnEulerMgr_->init(shell().context());

    uint32_t maxRandom =
        (std::max)((std::max)((std::max)(NUM_PARTICLES_BLUEWATER * 3, NUM_PARTICLES_FIRE * 4), NUM_PARTICLES_SMOKE),
                   NUM_PARTICLES_TORUS);

    // Unfortunately the textures have to be created inside init
    auto rand1dTexInfo = Texture::MakeRandom1dTex(Texture::Particle::RAND_1D_ID, Sampler::Particle::RAND_1D_ID, maxRandom);
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
        wave.update(shell().getCurrentTime<float>(), shell().getElapsedTime<float>(), frameIndex);
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

    ::Buffer::CreateInfo bufferInfo;
    Buffer::CreateInfo partBuffInfo;
    Buffer::Euler::CreateInfo partBuffEulerInfo;
    UniformDynamic::Particle::Fountain::CreateInfo fntnInfo;
    Material::Obj3d::CreateInfo matInfo;
    std::vector<std::shared_ptr<Material::Obj3d::Default>> pMaterials;

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
            // BUFFER
            partBuffInfo = {};
            partBuffInfo.name = "Bluewater Fountain Particle Buffer";
            partBuffInfo.pipelineTypeGraphics = PIPELINE::PRTCL_FOUNTAIN_DEFERRED;

            // MATERIALS
            matInfo = {};
            pMaterials.clear();
            matInfo.pTexture = textureHandler().getTexture(Texture::BLUEWATER_ID);
            // matInfo.model = helpers::affine(glm::vec3{0.5f}, glm::vec3{1.0f});
            // pMaterials.emplace_back(
            //    std::static_pointer_cast<Material::Obj3d::Default>(materialHandler().makeMaterial(&matInfo)));
            matInfo.model = helpers::affine(glm::vec3{0.5f}, glm::vec3{-3.0f, 0.0f, 0.0f}, M_PI_2_FLT, CARDINAL_Y);
            pMaterials.emplace_back(
                std::static_pointer_cast<Material::Obj3d::Default>(materialHandler().makeMaterial(&matInfo)));

            // FOUNTAIN
            fntnInfo = {};
            assert(shell().context().imageCount == 3);  // Potential imageCount problem
            fntnInfo.dataCount = shell().context().imageCount;
            fntnInfo.type = UniformDynamic::Particle::Fountain::INSTANCE::DEFAULT;
            fntnInfo.emitterBasis = helpers::makeArbitraryBasis({-1.0f, 2.0f, 0.0f});
            fntnInfo.lifespan = 20.0f;
            fntnInfo.minParticleSize = 0.1f;
            prtclFntnMgr_.insert(shell().context().dev, &fntnInfo);

            // INSTANCE
            Instance::Particle::Fountain::CreateInfo instInfo(&fntnInfo, NUM_PARTICLES_BLUEWATER);
            instFntnMgr_.insert(shell().context().dev, &instInfo);

            make<Buffer::Fountain>(pBuffers_, &partBuffInfo, pMaterials,
                                   {prtclFntnMgr_.pItems.back(), instFntnMgr_.pItems.back()});
        }

        // EULER(COMPUTE)
        if (hasInstFntnEulerMgr()) {
            // TORUS
            if (true) {
                // BUFFER
                partBuffEulerInfo = {};
                partBuffEulerInfo.name = "Torus Particle Buffer";
                partBuffEulerInfo.computeFlag = Particle::Euler::FLAG::FOUNTAIN;
                partBuffEulerInfo.pipelineTypeCompute = PIPELINE::PRTCL_EULER_COMPUTE;
                partBuffEulerInfo.pipelineTypeGraphics = PIPELINE::PRTCL_FOUNTAIN_EULER_DEFERRED;

                // MATERIALS
                matInfo = {};
                pMaterials.clear();
                matInfo.color = {0.8f, 0.3f, 0.0f};
                matInfo.model = helpers::affine(glm::vec3{0.07f}, glm::vec3{-1.0f, 0.2f, -1.0f});
                // TODO: Texture shouldn't be necessary, and it not used here but I don't feel like addressing the problem
                // atm.
                matInfo.pTexture = textureHandler().getTexture(Texture::BLUEWATER_ID);
                pMaterials.emplace_back(
                    std::static_pointer_cast<Material::Obj3d::Default>(materialHandler().makeMaterial(&matInfo)));

                // FOUNTAIN
                fntnInfo = {};
                assert(shell().context().imageCount == 3);  // Potential imageCount problem
                fntnInfo.dataCount = shell().context().imageCount;
                fntnInfo.type = UniformDynamic::Particle::Fountain::INSTANCE::EULER;
                fntnInfo.emitterBasis = helpers::makeArbitraryBasis({-1.0f, 2.0f, 0.0f});
                fntnInfo.lifespan = 8.0f;
                fntnInfo.minParticleSize = 0.1f;
                fntnInfo.velocityLowerBound = 2.0f;
                fntnInfo.velocityUpperBound = 3.5f;
                prtclFntnMgr_.insert(shell().context().dev, &fntnInfo);

                // INSTANCE
                Instance::Particle::FountainEuler::CreateInfo instInfo(&fntnInfo, NUM_PARTICLES_TORUS);
                pInstFntnEulerMgr_->insert(shell().context().dev, &instInfo);

                make<Buffer::Euler::Torus>(pBuffers_, &partBuffEulerInfo, pMaterials,
                                           {prtclFntnMgr_.pItems.back(), pInstFntnEulerMgr_->pItems.back()});
            }
            // FIRE
            if (true) {
                // BUFFER
                partBuffEulerInfo = {};
                partBuffEulerInfo.name = "Fire Euler Particle Buffer";
                partBuffEulerInfo.computeFlag = Particle::Euler::FLAG::FIRE;
                partBuffEulerInfo.pipelineTypeCompute = PIPELINE::PRTCL_EULER_COMPUTE;
                partBuffEulerInfo.pipelineTypeGraphics = PIPELINE::PRTCL_FOUNTAIN_EULER_DEFERRED;

                // MATERIALS
                matInfo = {};
                pMaterials.clear();
                matInfo.color = {0.8f, 0.3f, 0.0f};
                matInfo.model = helpers::affine(glm::vec3{1.0f}, glm::vec3{-3.0f, 0.0f, -3.0f});
                matInfo.pTexture = textureHandler().getTexture(Texture::FIRE_ID);
                pMaterials.emplace_back(
                    std::static_pointer_cast<Material::Obj3d::Default>(materialHandler().makeMaterial(&matInfo)));

                // FOUNTAIN
                fntnInfo = {};
                assert(shell().context().imageCount == 3);  // Potential imageCount problem
                fntnInfo.dataCount = shell().context().imageCount;
                fntnInfo.type = UniformDynamic::Particle::Fountain::INSTANCE::EULER;
                fntnInfo.emitterBasis = helpers::makeArbitraryBasis({0.0f, 1.0f, 0.0f});
                fntnInfo.lifespan = 3.0f;
                fntnInfo.minParticleSize = 0.5f;
                fntnInfo.acceleration = {0.0f, 0.1f, 0.0f};
                prtclFntnMgr_.insert(shell().context().dev, &fntnInfo);

                // INSTANCE
                Instance::Particle::FountainEuler::CreateInfo instInfo(&fntnInfo, NUM_PARTICLES_FIRE);
                pInstFntnEulerMgr_->insert(shell().context().dev, &instInfo);

                make<Buffer::Euler::Base>(pBuffers_, &partBuffEulerInfo, pMaterials,
                                          {prtclFntnMgr_.pItems.back(), pInstFntnEulerMgr_->pItems.back()});
            }
            // SMOKE
            if (true) {
                // BUFFER
                partBuffEulerInfo = {};
                partBuffEulerInfo.name = "Smoke Euler Particle Buffer";
                partBuffEulerInfo.computeFlag = Particle::Euler::FLAG::SMOKE;
                partBuffEulerInfo.pipelineTypeCompute = PIPELINE::PRTCL_EULER_COMPUTE;
                partBuffEulerInfo.pipelineTypeGraphics = PIPELINE::PRTCL_FOUNTAIN_EULER_DEFERRED;

                // MATERIALS
                matInfo = {};
                pMaterials.clear();
                matInfo.color = {0.8f, 0.3f, 0.0f};
                matInfo.model = helpers::affine(glm::vec3{1.0f}, glm::vec3{-0.5f, 2.0f, 1.0f});
                matInfo.pTexture = textureHandler().getTexture(Texture::SMOKE_ID);
                pMaterials.emplace_back(
                    std::static_pointer_cast<Material::Obj3d::Default>(materialHandler().makeMaterial(&matInfo)));

                // FOUNTAIN
                fntnInfo = {};
                assert(shell().context().imageCount == 3);  // Potential imageCount problem
                fntnInfo.dataCount = shell().context().imageCount;
                fntnInfo.type = UniformDynamic::Particle::Fountain::INSTANCE::EULER;
                fntnInfo.emitterBasis = helpers::makeArbitraryBasis({0.0f, 1.0f, 0.0f});
                fntnInfo.lifespan = 10.0f;
                fntnInfo.minParticleSize = 0.1f;
                fntnInfo.maxParticleSize = 2.5f;
                fntnInfo.acceleration = {0.0f, 0.1f, 0.0f};
                prtclFntnMgr_.insert(shell().context().dev, &fntnInfo);

                // INSTANCE
                Instance::Particle::FountainEuler::CreateInfo instInfo(&fntnInfo, NUM_PARTICLES_SMOKE);
                pInstFntnEulerMgr_->insert(shell().context().dev, &instInfo);

                make<Buffer::Euler::Base>(pBuffers_, &partBuffEulerInfo, pMaterials,
                                          {prtclFntnMgr_.pItems.back(), pInstFntnEulerMgr_->pItems.back()});
            }
            // ATTRACTOR
            if (true) {
                partBuffEulerInfo = {};
                partBuffEulerInfo.name = "Attractor Particle Buffer";
                partBuffEulerInfo.computeFlag = Particle::Euler::FLAG::NONE;
                partBuffEulerInfo.localSize.x = 1000;
                partBuffEulerInfo.firstInstanceBinding = 0;
                partBuffEulerInfo.pipelineTypeCompute = PIPELINE::PRTCL_ATTR_COMPUTE;
                partBuffEulerInfo.pipelineTypeGraphics = PIPELINE::PRTCL_ATTR_PT_DEFERRED;

                // MATERIALS
                matInfo = {};
                pMaterials.clear();
                matInfo.color = COLOR_GREEN;
                // matInfo.model = helpers::affine(glm::vec3{40.0f});s
                pMaterials.emplace_back(
                    std::static_pointer_cast<Material::Obj3d::Default>(materialHandler().makeMaterial(&matInfo)));

                // UNIFORM
                UniformDynamic::Particle::Attractor::CreateInfo attrInfo = {};
                assert(shell().context().imageCount == 3);  // Potential imageCount problem
                attrInfo.dataCount = shell().context().imageCount;
                attrInfo.attractorPosition0 = {5.0f, 0.0f, 0.0f};
                attrInfo.gravity0 = 0.1f;
                attrInfo.attractorPosition1 = {-5.0f, 0.0f, 0.0f};
                attrInfo.gravity1 = 0.1f;
                prtclAttrMgr_.insert(shell().context().dev, &attrInfo);

                // INSTANCE
                Instance::Particle::Vector4::CreateInfo vec4Info = {
                    // POSITION
                    Instance::Particle::Vector4::TYPE::ATTRACTOR_POSITION,
                    STORAGE_BUFFER_DYNAMIC::PRTCL_POSTITION,
                    NUM_PARTICLES_ATTR,
                };
                instVec4Mgr_.insert(shell().context().dev, &vec4Info);
                vec4Info = {
                    // VELOCITY
                    Instance::Particle::Vector4::TYPE::ATTRACTOR_VELOCITY,
                    STORAGE_BUFFER_DYNAMIC::PRTCL_VELOCITY,
                    NUM_PARTICLES_ATTR,
                };
                instVec4Mgr_.insert(shell().context().dev, &vec4Info);

                make<Buffer::Euler::Base>(pBuffers_, &partBuffEulerInfo, pMaterials,
                                          {
                                              prtclAttrMgr_.pItems.back(),                             // attractor
                                              instVec4Mgr_.pItems.at(instVec4Mgr_.pItems.size() - 2),  // position
                                              instVec4Mgr_.pItems.at(instVec4Mgr_.pItems.size() - 1),  // velocity
                                          });
            }
        }
    }

    // TODO: This is too simple.
    doUpdate_ = true;
}

void Particle::Handler::startFountain(const uint32_t offset) { pBuffers_.at(offset)->toggle(); }

void Particle::Handler::recordDraw(const PASS passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                                   const VkCommandBuffer& cmd, const uint8_t frameIndex) {
    // TODO: I know this is slow but its not important atm.
    for (const auto& pBuffer : pBuffers_) {
        if (pBuffer->PIPELINE_TYPE_GRAPHICS == pPipelineBindData->type) {
            if (pBuffer->getStatus() == STATUS::READY) {
                for (const auto& instance : pBuffer->getInstances()) {
                    if (pBuffer->shouldDraw()) {
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
                    if (pBuffer->shouldDraw()) {
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
                    if (pBuffer->shouldDraw()) {
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
    // UNIFORM DYNAMIC
    prtclAttrMgr_.destroy(shell().context().dev);
    prtclFntnMgr_.destroy(shell().context().dev);
    // INSTANCE
    instFntnMgr_.destroy(shell().context().dev);
    instVec4Mgr_.destroy(shell().context().dev);
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