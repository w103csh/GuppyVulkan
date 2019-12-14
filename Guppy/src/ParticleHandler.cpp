/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "ParticleHandler.h"

#include "Shell.h"
// HANDLER
#include "MeshHandler.h"
#include "RenderPassHandler.h"
#include "TextureHandler.h"
#include "UniformHandler.h"

namespace {
constexpr uint32_t NUM_PARTICLES_BLUEWATER = 4000;
constexpr uint32_t NUM_PARTICLES_TORUS = 64;
constexpr uint32_t NUM_PARTICLES_FIRE = 2000;
constexpr uint32_t NUM_PARTICLES_SMOKE = 1000;
const glm::uvec3 NUM_PARTICLES_ATTR{50, 50, 50};
const uint32_t NUM_PARTICLES_ATTR_TOTAL = NUM_PARTICLES_ATTR.x * NUM_PARTICLES_ATTR.y * NUM_PARTICLES_ATTR.z;
}  // namespace

Particle::Handler::Handler(Game* pGame)
    : Game::Handler(pGame),  //
      prtclAttrMgr{"Particle Attractor", UNIFORM_DYNAMIC::PRTCL_ATTRACTOR, 3, true, "_UD_PRTCL_ATTR"},
      prtclClthMgr{"Particle Cloth", UNIFORM_DYNAMIC::PRTCL_CLOTH, 5 * 3, true, "_UD_PRTCL_CLTH"},
      prtclFntnMgr{"Particle Fountain", UNIFORM_DYNAMIC::PRTCL_FOUNTAIN, 30, true, "_UD_PRTCL_ATTR"},
      mat4Mgr{"Matrix4 Data", UNIFORM_DYNAMIC::MATRIX_4, 30, true, "_UD_MAT4"},
      vec4Mgr{"Particle Vector4 Data", STORAGE_BUFFER_DYNAMIC::VERTEX, 1000000, false, "_UD_VEC4"},
      hffMgr{"Height Field Fluid Data", UNIFORM_DYNAMIC::HFF, 3, true, "_UD_HFF"},
      waterOffset(Buffer::BAD_OFFSET),
      doUpdate_(false),
      instFntnMgr_{"Particle Fountain Instance Data", 8000 * 5, false},
      pInstFntnEulerMgr_(nullptr) {}

void Particle::Handler::init() {
    reset();

    prtclAttrMgr.init(shell().context());
    prtclClthMgr.init(shell().context());
    prtclFntnMgr.init(shell().context());
    mat4Mgr.init(shell().context());
    vec4Mgr.init(shell().context());
    hffMgr.init(shell().context());
    instFntnMgr_.init(shell().context());
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
        wave.updatePerFrame(shell().getCurrentTime<float>(), shell().getElapsedTime<float>(), frameIndex);
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
    Material::Default::CreateInfo matInfo;
    std::vector<std::shared_ptr<Descriptor::Base>> pDescriptors;

    UniformDynamic::Matrix4::CreateInfo mdlInfo = {};
    assert(shell().context().imageCount == 3);  // Potential imageCount problem
    mdlInfo.dataCount = shell().context().imageCount;

    // WAVE
    {
        bufferInfo = {};
        assert(shell().context().imageCount == 3);  // Potential imageCount problem
        bufferInfo.dataCount = shell().context().imageCount;
        uniformHandler().uniWaveMgr().insert(dev, &bufferInfo);
    }

    bool suppress = true;

    // WATER
    if (!suppress || false) {
        pDescriptors.clear();

        HeightFieldFluid::Info info = {100, 100, 100.0f};

        // BUFFER
        HeightFieldFluid::CreateInfo buffHFFInfo = {};
        buffHFFInfo.name = "Particle Cloth Buffer";
        buffHFFInfo.localSize = {4, 4, 1};
        buffHFFInfo.computePipelineTypes = {COMPUTE::HFF_HGHT, COMPUTE::HFF_NORM};
        buffHFFInfo.graphicsPipelineTypes = {
            GRAPHICS::HFF_OCEAN_DEFERRED,
            GRAPHICS::HFF_CLMN_DEFERRED,
            GRAPHICS::HFF_WF_DEFERRED,
        };
        buffHFFInfo.info = info;

        // HEIGHT FIELD FLUID
        UniformDynamic::HeightFieldFluid::Simulation::CreateInfo hffInfo = {};
        assert(shell().context().imageCount == 3);  // Potential imageCount problem
        hffInfo.dataCount = shell().context().imageCount;
        hffInfo.info = info;
        hffInfo.c = 4.0f;
        hffInfo.maxSlope = 4.0f;
        hffMgr.insert(shell().context().dev, &hffInfo);
        pDescriptors.push_back(hffMgr.pItems.back());

        // MATERIAL
        matInfo = {};
        // matInfo.flags = Material::FLAG::PER_VERTEX_COLOR;
        matInfo.color = COLOR_BLUE;
        matInfo.shininess = 10;
        auto& pMaterial = materialHandler().makeMaterial(&matInfo);

        // INSTANCE
        Instance::Obj3d::CreateInfo instInfo = {};
        // instInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{1.0f, 2.0f, 1.0f})});
        auto& pInstanceData = meshHandler().makeInstanceObj3d(&instInfo);

        // NORMAL
        Storage::Vector4::CreateInfo vec4Info({info.M, info.N, 1});
        vec4Info.descType = STORAGE_BUFFER_DYNAMIC::NORMAL;
        vec4Mgr.insert(shell().context().dev, &vec4Info);
        pDescriptors.push_back(vec4Mgr.pItems.back());

        make<HeightFieldFluid::Buffer>(pBuffers_, &buffHFFInfo, pMaterial, pDescriptors, pInstanceData);
        waterOffset = static_cast<uint32_t>(pBuffers_.size() - 1);
    }

    // FOUNTAIN
    {
        // BLUEWATER
        if (!suppress || false) {
            pDescriptors.clear();

            // BUFFER
            partBuffInfo = {};
            partBuffInfo.name = "Bluewater Fountain Particle Buffer";
            partBuffInfo.graphicsPipelineTypes = {GRAPHICS::PRTCL_FOUNTAIN_DEFERRED};

            // MATERIALS
            matInfo = {};
            matInfo.pTexture = textureHandler().getTexture(Texture::BLUEWATER_ID);
            auto& pMaterial = materialHandler().makeMaterial(&matInfo);

            // MODELS
            // mdlInfo.data = {helpers::affine(glm::vec3{0.5f}, glm::vec3{1.0f})};
            // mat4Mgr.insert(shell().context().dev, &mdlInfo);
            // pDescriptors.push_back(mat4Mgr.pItems.back());
            mdlInfo.data = helpers::affine(glm::vec3{0.5f}, glm::vec3{-3.0f, 0.0f, 0.0f}, M_PI_2_FLT, CARDINAL_Y);
            mat4Mgr.insert(shell().context().dev, &mdlInfo);
            pDescriptors.push_back(mat4Mgr.pItems.back());

            // FOUNTAIN
            fntnInfo = {};
            assert(shell().context().imageCount == 3);  // Potential imageCount problem
            fntnInfo.dataCount = shell().context().imageCount;
            fntnInfo.type = UniformDynamic::Particle::Fountain::INSTANCE::DEFAULT;
            fntnInfo.emitterBasis = helpers::makeArbitraryBasis({-1.0f, 2.0f, 0.0f});
            fntnInfo.lifespan = 20.0f;
            fntnInfo.minParticleSize = 0.1f;
            prtclFntnMgr.insert(shell().context().dev, &fntnInfo);
            pDescriptors.push_back(prtclFntnMgr.pItems.back());

            // INSTANCE
            Particle::Fountain::CreateInfo instInfo(&fntnInfo, NUM_PARTICLES_BLUEWATER);
            instFntnMgr_.insert(shell().context().dev, &instInfo);

            make<Buffer::Fountain>(pBuffers_, &partBuffInfo, pMaterial, pDescriptors, instFntnMgr_.pItems.back());
        }

        // EULER (COMPUTE)
        if (hasInstFntnEulerMgr()) {
            // TORUS
            if (!suppress || false) {
                pDescriptors.clear();

                // BUFFER
                partBuffEulerInfo = {};
                partBuffEulerInfo.vertexType = Particle::Buffer::Euler::VERTEX::MESH;
                partBuffEulerInfo.name = "Torus Particle Buffer";
                partBuffEulerInfo.computeFlag = Particle::Euler::FLAG::FOUNTAIN;
                partBuffEulerInfo.computePipelineTypes = {COMPUTE::PRTCL_EULER};
                partBuffEulerInfo.graphicsPipelineTypes = {GRAPHICS::PRTCL_FOUNTAIN_EULER_DEFERRED};

                // MATERIALS
                matInfo = {};
                matInfo.color = {0.8f, 0.3f, 0.0f};
                // TODO: Texture shouldn't be necessary, and it not used here but I don't feel like addressing the problem
                // atm.
                matInfo.pTexture = textureHandler().getTexture(Texture::BLUEWATER_ID);
                auto& pMaterial = materialHandler().makeMaterial(&matInfo);

                // MODELS
                mdlInfo.data = helpers::affine(glm::vec3{0.25f}, glm::vec3{0.0f, -3.5f, 0.0f});
                mat4Mgr.insert(shell().context().dev, &mdlInfo);
                pDescriptors.push_back(mat4Mgr.pItems.back());

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
                prtclFntnMgr.insert(shell().context().dev, &fntnInfo);
                pDescriptors.push_back(prtclFntnMgr.pItems.back());

                // INSTANCE
                Particle::FountainEuler::CreateInfo instInfo(&fntnInfo, NUM_PARTICLES_TORUS);
                pInstFntnEulerMgr_->insert(shell().context().dev, &instInfo);
                pDescriptors.push_back(pInstFntnEulerMgr_->pItems.back());

                make<Buffer::Euler::Torus>(pBuffers_, &partBuffEulerInfo, pMaterial, pDescriptors);
            }
            // FIRE
            if (!suppress || false) {
                pDescriptors.clear();

                // BUFFER
                partBuffEulerInfo = {};
                partBuffEulerInfo.vertexType = Particle::Buffer::Euler::VERTEX::BILLBOARD;
                partBuffEulerInfo.name = "Fire Euler Particle Buffer";
                partBuffEulerInfo.computeFlag = Particle::Euler::FLAG::FIRE;
                partBuffEulerInfo.computePipelineTypes = {COMPUTE::PRTCL_EULER};
                partBuffEulerInfo.graphicsPipelineTypes = {GRAPHICS::PRTCL_FOUNTAIN_EULER_DEFERRED};

                // MATERIALS
                matInfo = {};
                matInfo.pTexture = textureHandler().getTexture(Texture::FIRE_ID);
                auto& pMaterial = materialHandler().makeMaterial(&matInfo);

                // MODELS
                mdlInfo.data = helpers::affine(glm::vec3{1.0f}, glm::vec3{-3.0f, 0.0f, -3.0f});
                mat4Mgr.insert(shell().context().dev, &mdlInfo);
                pDescriptors.push_back(mat4Mgr.pItems.back());

                // FOUNTAIN
                fntnInfo = {};
                assert(shell().context().imageCount == 3);  // Potential imageCount problem
                fntnInfo.dataCount = shell().context().imageCount;
                fntnInfo.type = UniformDynamic::Particle::Fountain::INSTANCE::EULER;
                fntnInfo.emitterBasis = helpers::makeArbitraryBasis({0.0f, 1.0f, 0.0f});
                fntnInfo.lifespan = 3.0f;
                fntnInfo.minParticleSize = 0.5f;
                fntnInfo.acceleration = {0.0f, 0.1f, 0.0f};
                prtclFntnMgr.insert(shell().context().dev, &fntnInfo);
                pDescriptors.push_back(prtclFntnMgr.pItems.back());

                // INSTANCE
                Particle::FountainEuler::CreateInfo instInfo(&fntnInfo, NUM_PARTICLES_FIRE);
                pInstFntnEulerMgr_->insert(shell().context().dev, &instInfo);
                pDescriptors.push_back(pInstFntnEulerMgr_->pItems.back());

                make<Buffer::Euler::Base>(pBuffers_, &partBuffEulerInfo, pMaterial, pDescriptors);
            }
            // SMOKE
            if (!suppress || false) {
                pDescriptors.clear();

                // BUFFER
                partBuffEulerInfo = {};
                partBuffEulerInfo.vertexType = Particle::Buffer::Euler::VERTEX::BILLBOARD;
                partBuffEulerInfo.name = "Smoke Euler Particle Buffer";
                partBuffEulerInfo.computeFlag = Particle::Euler::FLAG::SMOKE;
                partBuffEulerInfo.computePipelineTypes = {COMPUTE::PRTCL_EULER};
                partBuffEulerInfo.graphicsPipelineTypes = {GRAPHICS::PRTCL_FOUNTAIN_EULER_DEFERRED};

                // MATERIALS
                matInfo = {};
                matInfo.pTexture = textureHandler().getTexture(Texture::SMOKE_ID);
                auto& pMaterial = materialHandler().makeMaterial(&matInfo);

                // MODELS
                mdlInfo.data = helpers::affine(glm::vec3{1.0f}, glm::vec3{-0.5f, 2.0f, 1.0f});
                mat4Mgr.insert(shell().context().dev, &mdlInfo);
                pDescriptors.push_back(mat4Mgr.pItems.back());

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
                prtclFntnMgr.insert(shell().context().dev, &fntnInfo);
                pDescriptors.push_back(prtclFntnMgr.pItems.back());

                // INSTANCE
                Particle::FountainEuler::CreateInfo instInfo(&fntnInfo, NUM_PARTICLES_SMOKE);
                pInstFntnEulerMgr_->insert(shell().context().dev, &instInfo);
                pDescriptors.push_back(pInstFntnEulerMgr_->pItems.back());

                make<Buffer::Euler::Base>(pBuffers_, &partBuffEulerInfo, pMaterial, pDescriptors);
            }
            // ATTRACTOR
            if (!suppress || true) {
                pDescriptors.clear();

                partBuffEulerInfo = {};
                partBuffEulerInfo.vertexType = Particle::Buffer::Euler::VERTEX::BILLBOARD;
                partBuffEulerInfo.name = "Attractor Particle Buffer";
                partBuffEulerInfo.computeFlag = Particle::Euler::FLAG::NONE;
                partBuffEulerInfo.localSize.x = 1000;
                partBuffEulerInfo.firstInstanceBinding = 0;
                partBuffEulerInfo.computePipelineTypes = {COMPUTE::PRTCL_ATTR};
                partBuffEulerInfo.graphicsPipelineTypes = {GRAPHICS::PRTCL_ATTR_PT_DEFERRED};

                // MATERIALS
                matInfo = {};
                matInfo.color = COLOR_GREEN;
                auto& pMaterial = materialHandler().makeMaterial(&matInfo);

                // MODELS
                mdlInfo.data = glm::mat4{1.0f};
                // mdlInfo.data = helpers::affine(glm::vec3{40.0f});
                mat4Mgr.insert(shell().context().dev, &mdlInfo);
                pDescriptors.push_back(mat4Mgr.pItems.back());

                // UNIFORM
                UniformDynamic::Particle::Attractor::CreateInfo attrInfo = {};
                assert(shell().context().imageCount == 3);  // Potential imageCount problem
                attrInfo.dataCount = shell().context().imageCount;
                attrInfo.attractorPosition0 = {5.0f, 0.0f, 0.0f};
                attrInfo.gravity0 = 0.1f;
                attrInfo.attractorPosition1 = {-5.0f, 0.0f, 0.0f};
                attrInfo.gravity1 = 0.1f;
                prtclAttrMgr.insert(shell().context().dev, &attrInfo);
                pDescriptors.push_back(prtclAttrMgr.pItems.back());

                // PARTICLE
                Storage::Vector4::CreateInfo vec4Info(NUM_PARTICLES_ATTR);  // POSITION
                vec4Info.type = Storage::Vector4::TYPE::ATTRACTOR_POSITION;
                vec4Info.descType = STORAGE_BUFFER_DYNAMIC::PRTCL_POSITION;
                vec4Mgr.insert(shell().context().dev, &vec4Info);
                pDescriptors.push_back(vec4Mgr.pItems.back());

                vec4Info = {NUM_PARTICLES_ATTR};  // VELOCITY
                vec4Info.type = Storage::Vector4::TYPE::DONT_CARE;
                vec4Info.descType = STORAGE_BUFFER_DYNAMIC::PRTCL_VELOCITY;
                vec4Mgr.insert(shell().context().dev, &vec4Info);
                pDescriptors.push_back(vec4Mgr.pItems.back());

                make<Buffer::Euler::Base>(pBuffers_, &partBuffEulerInfo, pMaterial, pDescriptors);
            }
        }
    }

    // CLOTH
    if (!suppress || false) {
        pDescriptors.clear();

        // PLANE INFO
        Mesh::Plane::Info planeInfo = {};
        planeInfo.horzDivs = 40;
        planeInfo.vertDivs = 40;
        planeInfo.width = 4.0f;
        planeInfo.height = 4.0f * (216.0f / 250.f);

        // UNIFORMS
        UniformDynamic::Particle::Cloth::CreateInfo clothInfo = {};
        assert(shell().context().imageCount == 3);  // Potential imageCount problem
        clothInfo.dataCount = shell().context().imageCount;
        clothInfo.planeInfo = planeInfo;
        // clothInfo.gravity = {-20.0f, -10.0f, 2.0f};
        clothInfo.gravity = {0.0f, -9.0f, 0.0f};
        // clothInfo.springK = 1000;
        prtclClthMgr.insert(shell().context().dev, &clothInfo);
        pDescriptors.push_back(prtclClthMgr.pItems.back());

        // INSTANCE
        Instance::Obj3d::CreateInfo instInfo = {};
        instInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{1.0f, 2.0f, 1.0f})});
        auto& pInstanceData = meshHandler().makeInstanceObj3d(&instInfo);

        // MATERIAL
        matInfo = {};
        matInfo.shininess = 10;
        matInfo.pTexture = textureHandler().getTexture(Texture::VULKAN_ID);
        auto& pMaterial = materialHandler().makeMaterial(&matInfo);

        // BUFFER
        Buffer::Cloth::CreateInfo prtclClothInfo = {};
        prtclClothInfo.name = "Particle Cloth Buffer";
        prtclClothInfo.localSize = {10, 10, 1};
        prtclClothInfo.computePipelineTypes = {COMPUTE::PRTCL_CLOTH, COMPUTE::PRTCL_CLOTH_NORM};
        prtclClothInfo.graphicsPipelineTypes = {GRAPHICS::PRTCL_CLOTH_DEFERRED};
        prtclClothInfo.planeInfo = planeInfo;
        // prtclClothInfo.geometryInfo.doubleSided = true;

        make<Buffer::Cloth::Base>(pBuffers_, &prtclClothInfo, pMaterial, pDescriptors, pInstanceData);
    }

    // TODO: This is too simple.
    doUpdate_ = true;
}

void Particle::Handler::startFountain(const uint32_t offset) {
    if (offset < pBuffers_.size()) pBuffers_[offset]->toggle();
}

void Particle::Handler::recordDraw(const PASS passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                                   const VkCommandBuffer& cmd, const uint8_t frameIndex) {
    // TODO: This is slow.
    for (const auto& pBuffer : pBuffers_) {
        for (auto i = 0; i < pBuffer->GRAPHICS_PIPELINE_TYPES.size(); i++) {
            if (PIPELINE{pBuffer->GRAPHICS_PIPELINE_TYPES[i]} == pPipelineBindData->type) {
                if (pBuffer->getStatus() == STATUS::READY) {
                    if (pBuffer->shouldDraw()) {
                        pBuffer->draw(
                            passType, pPipelineBindData,
                            pBuffer->getDescriptorSetBindData(passType, pBuffer->getGraphicsDescSetBindDataMaps()[i]), cmd,
                            frameIndex);
                    }
                }
            }
        }
        if (PIPELINE{pBuffer->SHADOW_PIPELINE_TYPE} == pPipelineBindData->type) {
            if (pBuffer->getStatus() == STATUS::READY) {
                if (pBuffer->shouldDraw()) {
                    pBuffer->draw(passType, pPipelineBindData,
                                  pBuffer->getDescriptorSetBindData(passType, pBuffer->getShadowDescSetBindDataMap()), cmd,
                                  frameIndex);
                }
            }
        }
    }
}

void Particle::Handler::recordDispatch(const PASS passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                                       const VkCommandBuffer& cmd, const uint8_t frameIndex) {
    // TODO: This is slow.
    for (const auto& pBuffer : pBuffers_) {
        for (auto i = 0; i < pBuffer->COMPUTE_PIPELINE_TYPES.size(); i++) {
            if (PIPELINE{pBuffer->COMPUTE_PIPELINE_TYPES[i]} == pPipelineBindData->type) {
                if (pBuffer->getStatus() == STATUS::READY) {
                    if (pBuffer->shouldDispatch()) {
                        pBuffer->dispatch(
                            passType, pPipelineBindData,
                            pBuffer->getDescriptorSetBindData(passType, pBuffer->getComputeDescSetBindDataMaps()[i]), cmd,
                            frameIndex);
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
    prtclAttrMgr.destroy(shell().context().dev);
    prtclClthMgr.destroy(shell().context().dev);
    prtclFntnMgr.destroy(shell().context().dev);
    mat4Mgr.destroy(shell().context().dev);
    vec4Mgr.destroy(shell().context().dev);
    hffMgr.destroy(shell().context().dev);
    instFntnMgr_.destroy(shell().context().dev);
    if (pInstFntnEulerMgr_ == nullptr && shell().context().computeShadingEnabled) {
        pInstFntnEulerMgr_ =
            std::make_unique<Descriptor::Manager<Descriptor::Base, Particle::FountainEuler::Base, std::shared_ptr>>(
                "Instance Particle Fountain Data", STORAGE_BUFFER_DYNAMIC::VERTEX, (4000 * 5) * 2, false);
    } else if (pInstFntnEulerMgr_ != nullptr) {
        pInstFntnEulerMgr_->destroy(shell().context().dev);
        if (!shell().context().computeShadingEnabled) pInstFntnEulerMgr_ = nullptr;
    }
}
