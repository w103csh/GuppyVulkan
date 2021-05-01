/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "ParticleBuffer.h"

#include <Common/Helpers.h>

#include "Material.h"
#include "Random.h"
#include "Torus.h"
// HANDLERS
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "ParticleHandler.h"
#include "PipelineHandler.h"
#include "PassHandler.h"

namespace Particle {

// INSTANCE DATA

// FOUNTAIN
namespace Fountain {

void DATA::getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes) {
    const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.bindDescs.push_back({});
    createInfoRes.bindDescs.back().binding = BINDING;
    createInfoRes.bindDescs.back().stride = sizeof(DATA);
    createInfoRes.bindDescs.back().inputRate = vk::VertexInputRate::eInstance;

    // velocity
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = vk::Format::eR32G32B32Sfloat;  // vec3
    createInfoRes.attrDescs.back().offset = offsetof(DATA, velocity);
    // timeOfBirth
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = vk::Format::eR32Sfloat;  // float
    createInfoRes.attrDescs.back().offset = offsetof(DATA, timeOfBirth);
}

Base::Base(const ::Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : ::Buffer::Item(std::forward<const ::Buffer::Info>(info)),  //
      ::Buffer::DataItem<DATA>(pData) {
    auto& numParticles = BUFFER_INFO.count;

    glm::vec3 v(0.0f);
    float velocity, theta, phi;
    float rate = pCreateInfo->pInfo->lifespan / numParticles;

    // Fill the buffer with random velocities
    for (uint64_t i = 0; i < numParticles; i++) {
        theta = glm::mix(0.0f, glm::pi<float>() / 20.0f, Random::inst().nextFloatZeroToOne());
        phi = glm::mix(0.0f, glm::two_pi<float>(), Random::inst().nextFloatZeroToOne());

        v.x = sinf(theta) * cosf(phi);
        v.y = cosf(theta);
        v.z = sinf(theta) * sinf(phi);

        velocity = glm::mix(1.25f, 1.5f, Random::inst().nextFloatZeroToOne());
        v = glm::normalize(pCreateInfo->pInfo->emitterBasis * v) * velocity;

        pData_[i].velocity = v;
        pData_[i].timeOfBirth = rate * i;
    }
    dirty = true;
}

}  // namespace Fountain

// FOUNTAIN (EULER COMPUTE)

namespace FountainEuler {

void DATA::getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes) {
    const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.bindDescs.push_back({});
    createInfoRes.bindDescs.back().binding = BINDING;
    createInfoRes.bindDescs.back().stride = sizeof(DATA);
    createInfoRes.bindDescs.back().inputRate = vk::VertexInputRate::eInstance;

    // position / padding
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = vk::Format::eR32G32B32A32Sfloat;  // vec4
    createInfoRes.attrDescs.back().offset = offsetof(DATA, data0);
    // velocity / age
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = vk::Format::eR32G32B32A32Sfloat;  // vec4
    createInfoRes.attrDescs.back().offset = offsetof(DATA, data1);
    // rotation / padding
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = vk::Format::eR32G32B32A32Sfloat;  // vec4
    createInfoRes.attrDescs.back().offset = offsetof(DATA, data2);
}

Base::Base(const ::Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : ::Buffer::Item(std::forward<const ::Buffer::Info>(info)),  //
      ::Buffer::DataItem<DATA>(pData),
      Descriptor::Base(STORAGE_BUFFER_DYNAMIC::PRTCL_EULER) {
    float rate = pCreateInfo->pInfo->lifespan / BUFFER_INFO.count;
    for (uint64_t i = 0; i < BUFFER_INFO.count; i++) {
        // age
        pData_[i].data1[3] = rate * (static_cast<float>(i) - static_cast<float>(BUFFER_INFO.count));
    }

    dirty = true;
}

}  // namespace FountainEuler

}  // namespace Particle

// BUFFER

namespace Particle {
namespace Buffer {

// BASE

Base::Base(Particle::Handler& handler, const index offset, const CreateInfo* pCreateInfo,
           std::shared_ptr<Material::Base>& pMaterial, const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors,
           const GRAPHICS&& shadowPipelineType)
    : Handlee(handler),
      NAME(pCreateInfo->name),
      LOCAL_SIZE(pCreateInfo->localSize),
      COMPUTE_TYPES(pCreateInfo->computePipelineTypes),
      GRAPHICS_TYPES(pCreateInfo->graphicsPipelineTypes),
      SHADOW_PIPELINE_TYPE(shadowPipelineType),
      status_(STATUS::READY),
      draw_(false),
      paused_(true),
      doPausedUpdate_(false),
      workgroupSize_(1, 1, 1),
      indexRes_{},
      vertexRes_{},
      texCoordRes_{},
      pDescriptors_(pDescriptors),
      descInstOffset_(BAD_OFFSET),
      pMaterial_(pMaterial),
      pLdgRes_(nullptr),
      offset_(offset),
      descTimeOffset_(BAD_OFFSET) {
    for (const auto& type : COMPUTE_TYPES) assert(type != COMPUTE::ALL_ENUM);
    for (const auto& type : GRAPHICS_TYPES) assert(type != GRAPHICS::ALL_ENUM);
    assert(!(COMPUTE_TYPES.empty() && GRAPHICS_TYPES.empty()));

    // TODO: better validation on these types
    for (uint32_t i = 0; i < static_cast<uint32_t>(pDescriptors_.size()); i++) {
        assert(pDescriptors_[i] != nullptr && "Uniform data must be created");
        if (pDescriptors_[i]->getDescriptorType() == DESCRIPTOR{UNIFORM_DYNAMIC::PRTCL_FOUNTAIN} ||
            pDescriptors_[i]->getDescriptorType() == DESCRIPTOR{UNIFORM_DYNAMIC::PRTCL_ATTRACTOR} ||
            pDescriptors_[i]->getDescriptorType() == DESCRIPTOR{UNIFORM_DYNAMIC::HFF} ||
            pDescriptors_[i]->getDescriptorType() == DESCRIPTOR{UNIFORM_DYNAMIC::PRTCL_CLOTH}) {
            if (descTimeOffset_ == BAD_OFFSET) {
                descTimeOffset_ = i;
            } else {
                assert(false && "Only one timed uniform works atm.");
                exit(EXIT_FAILURE);
            }
        }
        auto strBuffDynType = std::visit(Descriptor::GetStorageBufferDynamic{}, pDescriptors_[i]->getDescriptorType());
        if (strBuffDynType == STORAGE_BUFFER_DYNAMIC::PRTCL_POSITION ||
            strBuffDynType == STORAGE_BUFFER_DYNAMIC::PRTCL_EULER) {
            descInstOffset_ = i;
        }
    }
    assert(descTimeOffset_ != BAD_OFFSET);

    if (pMaterial_ != nullptr) status_ |= STATUS::PENDING_MATERIAL;
}

void Base::update(const float time, const float elapsed, const uint32_t frameIndex) {
    if (doPausedUpdate_) {
        getTimedUniform()->updatePerFrame(time, elapsed, Descriptor::PAUSED_UPDATE);
        handler().update(getTimedUniform());
        doPausedUpdate_ = false;
    }
    if (paused_ || !draw_) return;  // Don't update when not drawing for now.
    getTimedUniform()->updatePerFrame(time, elapsed, frameIndex);
    handler().update(getTimedUniform(), frameIndex);
}

void Base::prepare() {
    if (status_ & STATUS::PENDING_BUFFERS) {
        loadBuffers();
        // Submit vertex loading commands...
        handler().loadingHandler().loadSubmit(std::move(pLdgRes_));
        status_ ^= STATUS::PENDING_BUFFERS;
    } else if (!vertexRes_.buffer && !indexRes_.buffer) {
        assert(vertices_.empty() && indices_.empty() && "Did you mean to set the status to PENDING_BUFFERS?");
    }

    if (status_ & STATUS::PENDING_MATERIAL) {
        if (pMaterial_->getStatus() == STATUS::READY) {
            status_ ^= STATUS::PENDING_MATERIAL;
        }
    }

    // TODO: see comment in Mesh::Base::prepare about descriptors.
    if (status_ == STATUS::READY) {
        if (COMPUTE_TYPES.size()) getComputeDescSetBindData();
        if (GRAPHICS_TYPES.size()) getGraphicsDescSetBindData();
        if (SHADOW_PIPELINE_TYPE != GRAPHICS::ALL_ENUM) getShadowDescSetBindData();
    } else {
        handler().ldgOffsets_.insert(getOffset());
    }
}

void Base::loadBuffers() {
    assert(vertices_.size() || indices_.size() || texCoords_.size());

    const auto& ctx = handler().shell().context();
    pLdgRes_ = handler().loadingHandler().createLoadingResources();

    // Vertex buffer
    BufferResource stgRes = {};
    if (vertices_.size()) {
        ctx.createBuffer(pLdgRes_->transferCmd,
                         vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
                         sizeof(Vertex::Color) * vertices_.size(), NAME + " vertex", stgRes, vertexRes_, vertices_.data());
        pLdgRes_->stgResources.push_back(std::move(stgRes));
    }

    // Index buffer
    if (indices_.size()) {
        stgRes = {};
        ctx.createBuffer(pLdgRes_->transferCmd,
                         vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
                         sizeof(IndexBufferType) * indices_.size(), NAME + " index", stgRes, indexRes_, indices_.data());
        pLdgRes_->stgResources.push_back(std::move(stgRes));
    }

    // Texture coordinate buffer
    if (texCoords_.size()) {
        stgRes = {};
        ctx.createBuffer(
            pLdgRes_->transferCmd, vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
            sizeof(glm::vec2) * texCoords_.size(), NAME + " tex coords", stgRes, texCoordRes_, texCoords_.data());
        pLdgRes_->stgResources.push_back(std::move(stgRes));
    }
}

void Base::destroy() {
    const auto& ctx = handler().shell().context();
    if (vertices_.size()) ctx.destroyBuffer(vertexRes_);
    if (indices_.size()) ctx.destroyBuffer(indexRes_);
    if (texCoords_.size()) ctx.destroyBuffer(texCoordRes_);
}

const std::vector<Descriptor::Base*> Base::getDynamicDataItems(const PIPELINE pipelineType) const {
    std::vector<Descriptor::Base*> pDescs;
    for (const auto [descSetType, stageFlags] :
         handler().pipelineHandler().getPipeline(pipelineType)->DESC_SET_STAGE_PAIRS) {
        const auto& descSet = handler().descriptorHandler().getDescriptorSet(descSetType);
        for (const auto& [key, bindingInfo] : descSet.getBindingMap()) {
            if (std::visit(Descriptor::IsDynamic{}, bindingInfo.descType)) {
                if (pMaterial_ != nullptr && pMaterial_->getDescriptorType() == bindingInfo.descType) {
                    // MATERIAL
                    pDescs.push_back(pMaterial_.get());
                } else {
                    auto it = std::find_if(pDescriptors_.begin(), pDescriptors_.end(),
                                           [&bindingInfo = bindingInfo](const auto& pDesc) {
                                               return pDesc->getDescriptorType() == bindingInfo.descType;
                                           });
                    if (it == pDescriptors_.end()) {
                        assert(false && "No data found for the descriptor type");
                        exit(EXIT_FAILURE);
                    }
                    pDescs.push_back((*it).get());
                }
            }
        }
    }
    return pDescs;
}

const Descriptor::Set::BindData& Base::getDescriptorSetBindData(const PASS& passType,
                                                                const Descriptor::Set::bindDataMap& map) const {
    for (const auto& [passTypes, bindData] : map) {
        if (passTypes.find(passType) != passTypes.end()) return bindData;
    }
    return map.at(Uniform::PASS_ALL_SET);
}

void Base::getComputeDescSetBindData() {
    for (const auto& pipelineType : COMPUTE_TYPES) {
        computeDescSetBindDataMaps_.emplace_back();
        handler().descriptorHandler().getBindData(pipelineType, computeDescSetBindDataMaps_.back(),
                                                  getDynamicDataItems(pipelineType));
    }
}

void Base::getGraphicsDescSetBindData() {
    for (const auto& pipelineType : GRAPHICS_TYPES) {
        graphicsDescSetBindDataMaps_.emplace_back();
        handler().descriptorHandler().getBindData(pipelineType, graphicsDescSetBindDataMaps_.back(),
                                                  getDynamicDataItems(pipelineType));
    }
}

void Base::getShadowDescSetBindData() {
    std::set<PASS> passTypes;
    handler().passHandler().getActivePassTypes(passTypes, SHADOW_PIPELINE_TYPE);
    if (passTypes.size()) {
        handler().descriptorHandler().getBindData(SHADOW_PIPELINE_TYPE, shadowDescSetBindDataMap_,
                                                  getDynamicDataItems(SHADOW_PIPELINE_TYPE));
    }
}

// FOUNTAIN

Fountain::Fountain(Particle::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo,
                   std::shared_ptr<Material::Base>& pMaterial,
                   const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors,
                   std::shared_ptr<::Instance::Base> pInst)
    : Base(handler, std::forward<const index>(offset), pCreateInfo, pMaterial, pDescriptors), pInst_(pInst) {}

void Fountain::draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                    const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                    const uint8_t frameIndex) const {
    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                           static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                           descSetBindData.descriptorSets[setIndex].data(),
                           static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                           descSetBindData.dynamicOffsets.data());

    // Instance Fountain
    cmd.bindVertexBuffers(                        //
        0,                                        // uint32_t firstBinding
        {pInst_->BUFFER_INFO.bufferInfo.buffer},  // const vk::Buffer* pBuffers
        {pInst_->BUFFER_INFO.memoryOffset}        // const vk::DeviceSize* pOffsets
    );

    cmd.draw(                       //
        6,                          // uint32_t vertexCount
        pInst_->BUFFER_INFO.count,  // uint32_t instanceCount
        0,                          // uint32_t firstVertex
        0                           // uint32_t firstInstance
    );
}

void Fountain::update(const float time, const float elapsed, const uint32_t frameIndex) {
    if (paused_) return;
    getTimedUniform()->updatePerFrame(time, elapsed, frameIndex);
    auto lastTimeOfBirth = std::static_pointer_cast<Particle::Fountain::Base>(pInst_)->getLastTimeOfBirth();
    const auto& pTimedUniform = std::static_pointer_cast<UniformDynamic::Particle::Fountain::Base>(getTimedUniform());
    if (pTimedUniform->getDelta() > (lastTimeOfBirth + pTimedUniform->getLifespan() + 0.0001f)) {
        pTimedUniform->reset();
        paused_ = true;
    }
    handler().update(getTimedUniform(), frameIndex);
}

// EULER

namespace Euler {

Base::Base(Particle::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo,
           std::shared_ptr<Material::Base>& pMaterial, const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors,
           const GRAPHICS&& shadowPipelineType)
    : Buffer::Base(handler, std::forward<const index>(offset), pCreateInfo, pMaterial, pDescriptors,
                   std::forward<const GRAPHICS>(shadowPipelineType)),
      VERTEX_TYPE(pCreateInfo->vertexType),
      pushConstant_(pCreateInfo->computeFlag),
      firstInstanceBinding_(pCreateInfo->firstInstanceBinding) {
    assert(VERTEX_TYPE != VERTEX::DONT_CARE);
    assert(pDescriptors_.size());
    assert(descInstOffset_ != BAD_OFFSET);
    assert(pDescriptors_.at(descInstOffset_)->BUFFER_INFO.count % LOCAL_SIZE.x == 0);
}

void Base::draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                const uint8_t frameIndex) const {
    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    if (pPipelineBindData->type != PIPELINE{GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER} &&
        pushConstant_ != Particle::Euler::FLAG::NONE) {
        cmd.pushConstants(pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                          static_cast<uint32_t>(sizeof(pushConstant_)), &pushConstant_);
    }

    cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                           static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                           descSetBindData.descriptorSets[setIndex].data(),
                           static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                           descSetBindData.dynamicOffsets.data());

    auto pInstData = pDescriptors_.at(descInstOffset_);

    // Instance
    cmd.bindVertexBuffers(                           //
        firstInstanceBinding_,                       // uint32_t firstBinding
        {pInstData->BUFFER_INFO.bufferInfo.buffer},  // const vk::Buffer* pBuffers
        {pInstData->BUFFER_INFO.memoryOffset}        // const vk::DeviceSize* pOffsets
    );

    if (vertices_.size() && indices_.size()) {
        assert(vertices_.size() && indices_.size());
        assert(VERTEX_TYPE == VERTEX::MESH);

        cmd.bindVertexBuffers(Vertex::BINDING, {vertexRes_.buffer}, {0});
        cmd.bindIndexBuffer(indexRes_.buffer, 0, vk::IndexType::eUint32);

        cmd.drawIndexed(                             //
            static_cast<uint32_t>(indices_.size()),  // uint32_t indexCount
            pInstData->BUFFER_INFO.count,            // uint32_t instanceCount
            0,                                       // uint32_t firstIndex
            0,                                       // int32_t vertexOffset
            0                                        // uint32_t firstInstance
        );

    } else {
        assert(VERTEX_TYPE != VERTEX::MESH);

        // TODO: The point type should really just use the typical instance obj3d and per vertex rate for the position
        // data. Right now the position data is per instance, but that is something to do when time permits.
        uint32_t vertexCount = VERTEX_TYPE == VERTEX::BILLBOARD ? 6 : 1;

        // Draw a billboarded quad which does not require a vertex bind
        cmd.draw(                          //
            vertexCount,                   // uint32_t vertexCount
            pInstData->BUFFER_INFO.count,  // uint32_t instanceCount
            0,                             // uint32_t firstVertex
            0                              // uint32_t firstInstance
        );
    }
}

void Base::dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                    const Descriptor::Set::BindData& descSetBindData, const vk::CommandBuffer& cmd,
                    const uint8_t frameIndex) const {
    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    cmd.bindPipeline(pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    if (pushConstant_ != Particle::Euler::FLAG::NONE) {
        cmd.pushConstants(pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                          static_cast<uint32_t>(sizeof(pushConstant_)), &pushConstant_);
    }

    cmd.bindDescriptorSets(pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                           static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                           descSetBindData.descriptorSets[setIndex].data(),
                           static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                           descSetBindData.dynamicOffsets.data());

    assert(LOCAL_SIZE.y == 1 && LOCAL_SIZE.z == 1);  // Haven't thought about anything else.

    cmd.dispatch(pDescriptors_.at(descInstOffset_)->BUFFER_INFO.count / LOCAL_SIZE.x, 1, 1);
}

// TORUS

Torus::Torus(Particle::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo,
             std::shared_ptr<Material::Base>& pMaterial, const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors)
    : Euler::Base(handler, std::forward<const index>(offset), pCreateInfo, pMaterial, pDescriptors,
                  GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER) {
    Mesh::Torus::Info torusInfo = {};
    Mesh::Torus::make(torusInfo, vertices_, indices_);
    assert(VERTEX_TYPE == VERTEX::MESH);
    assert(pMaterial_ != nullptr);
    FlagBits matFlags = pMaterial_->getFlags();
    matFlags |= Material::FLAG::IS_MESH;
    pMaterial_->setFlags(matFlags);
    handler.materialHandler().update(pMaterial_);
    status_ |= PENDING_BUFFERS;
}

}  // namespace Euler

}  // namespace Buffer
}  // namespace Particle
