
#include "ParticleBuffer.h"

#include "Random.h"
#include "Torus.h"
// HANDLERS
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "ParticleHandler.h"
#include "RenderPassHandler.h"

// INSTANCE

namespace Instance {
namespace Particle {

// FOUNTAIN

namespace Fountain {

void DATA::getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes) {
    const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.bindDescs.push_back({});
    createInfoRes.bindDescs.back().binding = BINDING;
    createInfoRes.bindDescs.back().stride = sizeof(DATA);
    createInfoRes.bindDescs.back().inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    // velocity
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    createInfoRes.attrDescs.back().offset = offsetof(DATA, velocity);
    // timeOfBirth
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32_SFLOAT;  // float
    createInfoRes.attrDescs.back().offset = offsetof(DATA, timeOfBirth);
}

Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::DataItem<DATA>(pData),
      Instance::Base(STORAGE_BUFFER_DYNAMIC::DONT_CARE) {
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
    createInfoRes.bindDescs.back().inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    // position / padding
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    createInfoRes.attrDescs.back().offset = offsetof(DATA, data0);
    // velocity / age
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    createInfoRes.attrDescs.back().offset = offsetof(DATA, data1);
    // rotation / padding
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    createInfoRes.attrDescs.back().offset = offsetof(DATA, data2);
}

Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::DataItem<DATA>(pData),
      Instance::Base(STORAGE_BUFFER_DYNAMIC::PARTICLE_EULER) {
    float rate = pCreateInfo->pInfo->lifespan / BUFFER_INFO.count;
    for (uint64_t i = 0; i < BUFFER_INFO.count; i++) {
        // age
        pData_[i].data1[3] = rate * (static_cast<float>(i) - static_cast<float>(BUFFER_INFO.count));
    }

    dirty = true;
}

}  // namespace FountainEuler

}  // namespace Particle
}  // namespace Instance

// BUFFER

namespace Particle {
namespace Buffer {

// BASE

Base::Base(Particle::Handler& handler, const index offset, const ::Particle::Fountain::CreateInfo* pCreateInfo,
           const std::vector<std::shared_ptr<Material::Base>>& pMaterials, const PIPELINE&& pipelineTypeShadow)
    : Handlee(handler),
      NAME(pCreateInfo->name),
      PIPELINE_TYPE_COMPUTE(pCreateInfo->pipelineTypeCompute),
      PIPELINE_TYPE_GRAPHICS(pCreateInfo->pipelineTypeGraphics),
      PIPELINE_TYPE_SHADOW(pipelineTypeShadow),
      status_(STATUS::READY),
      offset_(offset) {
    assert(!(PIPELINE_TYPE_COMPUTE == PIPELINE::ALL_ENUM && PIPELINE_TYPE_GRAPHICS == PIPELINE::ALL_ENUM));
    // TODO: This should work if a material is not needed...
    assert(pMaterials.size() && "Can't handle particle buffers without a material currently.");
    for (auto& pMaterial : pMaterials) instances_.push_back({pMaterial, {}, {}});
    if (pMaterials.size()) status_ |= STATUS::PENDING_MATERIAL;
}

void Base::prepare() {
    if (status_ & STATUS::PENDING_BUFFERS) {
        loadBuffers();
        // Submit vertex loading commands...
        handler().loadingHandler().loadSubmit(std::move(pLdgRes_));
        status_ ^= STATUS::PENDING_BUFFERS;
    } else if (vertexRes_.buffer == VK_NULL_HANDLE && indexRes_.buffer == VK_NULL_HANDLE) {
        assert(vertices_.empty() && indices_.empty() && "Did you mean to set the status to PENDING_BUFFERS?");
    }

    if (status_ & STATUS::PENDING_MATERIAL) {
        bool pendingMaterial = false;
        for (const auto& instance : instances_) pendingMaterial |= instance.pMaterial->getStatus() != STATUS::READY;
        if (!pendingMaterial) status_ ^= STATUS::PENDING_MATERIAL;
    }

    // TODO: see comment in Mesh::Base::prepare about descriptors.
    if (status_ == STATUS::READY) {
        for (auto& instance : instances_) {
            if (PIPELINE_TYPE_COMPUTE != PIPELINE::ALL_ENUM) {
                handler().descriptorHandler().getBindData(PIPELINE_TYPE_COMPUTE, instance.computeDescSetBindDataMap,
                                                          getGraphicsDynDescItems(instance),
                                                          instance.pMaterial->getTexture());
            }
            if (PIPELINE_TYPE_GRAPHICS != PIPELINE::ALL_ENUM) {
                handler().descriptorHandler().getBindData(PIPELINE_TYPE_GRAPHICS, instance.graphicsDescSetBindDataMap,
                                                          {instance.pMaterial.get()}, instance.pMaterial->getTexture());
            }
            if (PIPELINE_TYPE_SHADOW != PIPELINE::ALL_ENUM) {
                std::set<PASS> passTypes;
                handler().passHandler().getActivePassTypes(passTypes, PIPELINE_TYPE_SHADOW);
                if (passTypes.size()) {
                    handler().descriptorHandler().getBindData(PIPELINE_TYPE_SHADOW, instance.shadowDescSetBindDataMap,
                                                              {instance.pMaterial.get()}, instance.pMaterial->getTexture());
                }
            }
        }
    } else {
        handler().ldgOffsets_.insert(getOffset());
    }
}

void Base::loadBuffers() {
    assert(vertices_.size());

    pLdgRes_ = handler().loadingHandler().createLoadingResources();

    // Vertex buffer
    BufferResource stgRes = {};
    handler().createBuffer(
        pLdgRes_->transferCmd,
        static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
        sizeof(Vertex::Color) * vertices_.size(), NAME + " vertex", stgRes, vertexRes_, vertices_.data());
    pLdgRes_->stgResources.push_back(std::move(stgRes));

    // Index buffer
    if (indices_.size()) {
        stgRes = {};
        handler().createBuffer(
            pLdgRes_->transferCmd,
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
            sizeof(VB_INDEX_TYPE) * indices_.size(), NAME + " index", stgRes, indexRes_, indices_.data());
        pLdgRes_->stgResources.push_back(std::move(stgRes));
    }
}

void Base::destroy() {
    auto& dev = handler().shell().context().dev;
    // vertex
    vkDestroyBuffer(dev, vertexRes_.buffer, nullptr);
    vkFreeMemory(dev, vertexRes_.memory, nullptr);
}

const Descriptor::Set::BindData& Base::getDescriptorSetBindData(const PASS& passType,
                                                                const Descriptor::Set::bindDataMap& map) const {
    for (const auto& [passTypes, bindData] : map) {
        if (passTypes.find(passType) != passTypes.end()) return bindData;
    }
    return map.at(Uniform::PASS_ALL_SET);
}

// FOUNTAIN

Fountain::Fountain(Particle::Handler& handler, const index&& offset, const ::Particle::Fountain::CreateInfo* pCreateInfo,
                   const std::vector<std::shared_ptr<Material::Base>>& pMaterials,
                   std::shared_ptr<::Instance::Particle::Fountain::Base>& pInstFntn)
    : Base(handler, std::forward<const index>(offset), pCreateInfo, pMaterials), pInstFntn_(pInstFntn) {
    assert(getInstances().size());
    assert(pInstFntn_ != nullptr);
}

void Fountain::start(const StartInfo& info) {
    if (info.instanceOffsets.empty()) {
        // Using empty offsets vector to indicate start all.
        for (auto& instance : getInstances())
            std::static_pointer_cast<Material::Particle::Fountain::Base>(instance.pMaterial)->start();
    } else {
        // Start the instances with and offset specified.
        for (const auto& offset : info.instanceOffsets)
            std::static_pointer_cast<Material::Particle::Fountain::Base>(getInstances().at(offset).pMaterial)->start();
    }
}

void Fountain::update(const float time, const float elapsed, const uint32_t frameIndex) {
    float lastTimeOfBirth = pInstFntn_->getLastTimeOfBirth();
    for (const auto& instance : getInstances()) {
        std::static_pointer_cast<Material::Particle::Fountain::Base>(instance.pMaterial)
            ->update(time, lastTimeOfBirth, frameIndex);
        handler().materialHandler().update(instance.pMaterial, frameIndex);
    }
}

bool Fountain::shouldDraw(const InstanceInfo& instance) {
    bool draw = false;
    if (status_ == STATUS::READY) {
        auto lastTimeOfBirth = pInstFntn_->getLastTimeOfBirth();
        draw = std::static_pointer_cast<Material::Particle::Fountain::Base>(instance.pMaterial)->shouldDraw(lastTimeOfBirth);
    }
    return draw;
}

void Fountain::draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                    const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                    const uint8_t frameIndex) const {
    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                            static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                            descSetBindData.descriptorSets[setIndex].data(),
                            static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                            descSetBindData.dynamicOffsets.data());

    // Instance Fountain
    vkCmdBindVertexBuffers(                          //
        cmd,                                         // VkCommandBuffer commandBuffer
        1,                                           // uint32_t firstBinding
        1,                                           // uint32_t bindingCount
        &pInstFntn_->BUFFER_INFO.bufferInfo.buffer,  // const VkBuffer* pBuffers
        &pInstFntn_->BUFFER_INFO.memoryOffset        // const VkDeviceSize* pOffsets
    );

    vkCmdDraw(                          //
        cmd,                            // VkCommandBuffer commandBuffer
        6,                              // uint32_t vertexCount
        pInstFntn_->BUFFER_INFO.count,  // uint32_t instanceCount
        0,                              // uint32_t firstVertex
        0                               // uint32_t firstInstance
    );
}

// EULER

namespace Euler {

Base::Base(Particle::Handler& handler, const index&& offset, const ::Particle::Fountain::CreateInfo* pCreateInfo,
           const std::vector<std::shared_ptr<Material::Base>>& pMaterials,
           std::shared_ptr<::Instance::Particle::FountainEuler::Base>& pInstFntn, const PIPELINE&& pipelineTypeShadow)
    : Buffer::Base(handler, std::forward<const index>(offset), pCreateInfo, pMaterials,
                   std::forward<const PIPELINE>(pipelineTypeShadow)),
      pInstFntn_(pInstFntn),
      pushConstant_(pCreateInfo->computeFlag) {
    assert(getInstances().size());
    assert(pInstFntn_ != nullptr);
}

void Base::start(const StartInfo& info) {
    if (info.instanceOffsets.empty()) {
        // Using empty offsets vector to indicate start all.
        for (auto& instance : getInstances())
            std::static_pointer_cast<Material::Particle::Fountain::Base>(instance.pMaterial)->start();
    } else {
        // Start the instances with and offset specified.
        for (const auto& offset : info.instanceOffsets)
            std::static_pointer_cast<Material::Particle::Fountain::Base>(getInstances().at(offset).pMaterial)->start();
    }
}

void Base::update(const float time, const float elapsed, const uint32_t frameIndex) {
    for (const auto& instance : getInstances()) {
        std::static_pointer_cast<Material::Particle::Fountain::Base>(instance.pMaterial)->updateEuler(elapsed, frameIndex);
        handler().materialHandler().update(instance.pMaterial, frameIndex);
    }
}

bool Base::shouldDraw(const InstanceInfo& instance) {
    return status_ == STATUS::READY &&
           std::static_pointer_cast<Material::Particle::Fountain::Base>(instance.pMaterial)->shouldDrawEuler();
}

void Base::draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                const uint8_t frameIndex) const {
    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    vkCmdPushConstants(cmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                       static_cast<uint32_t>(sizeof(pushConstant_)), &pushConstant_);

    vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                            static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                            descSetBindData.descriptorSets[setIndex].data(),
                            static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                            descSetBindData.dynamicOffsets.data());

    // Instance
    vkCmdBindVertexBuffers(                          //
        cmd,                                         // VkCommandBuffer commandBuffer
        1,                                           // uint32_t firstBinding
        1,                                           // uint32_t bindingCount
        &pInstFntn_->BUFFER_INFO.bufferInfo.buffer,  // const VkBuffer* pBuffers
        &pInstFntn_->BUFFER_INFO.memoryOffset        // const VkDeviceSize* pOffsets
    );

    if (vertices_.size() && indices_.size()) {
        assert(vertices_.size() && indices_.size());
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, Vertex::BINDING, 1, &vertexRes_.buffer, offsets);
        vkCmdBindIndexBuffer(cmd, indexRes_.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(                            //
            cmd,                                     // VkCommandBuffer commandBuffer
            static_cast<uint32_t>(indices_.size()),  // uint32_t indexCount
            pInstFntn_->BUFFER_INFO.count,           // uint32_t instanceCount
            0,                                       // uint32_t firstIndex
            0,                                       // int32_t vertexOffset
            0                                        // uint32_t firstInstance
        );

    } else {
        // Draw a billboarded quad which does not require a vertex bind
        vkCmdDraw(                          //
            cmd,                            // VkCommandBuffer commandBuffer
            6,                              // uint32_t vertexCount
            pInstFntn_->BUFFER_INFO.count,  // uint32_t instanceCount
            0,                              // uint32_t firstVertex
            0                               // uint32_t firstInstance
        );
    }
}

void Base::dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                    const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                    const uint8_t frameIndex) const {
    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    vkCmdPushConstants(cmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                       static_cast<uint32_t>(sizeof(pushConstant_)), &pushConstant_);

    vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                            static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                            descSetBindData.descriptorSets[setIndex].data(),
                            static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                            descSetBindData.dynamicOffsets.data());

    // DISPATCH
    vkCmdDispatch(cmd, pInstFntn_->BUFFER_INFO.count / NUM_PARTICLES_EULER_MIN, 1, 1);
}

// FOUNTAIN

Torus::Torus(Particle::Handler& handler, const index&& offset, const ::Particle::Fountain::CreateInfo* pCreateInfo,
             const std::vector<std::shared_ptr<Material::Base>>& pMaterials,
             std::shared_ptr<::Instance::Particle::FountainEuler::Base>& pInstFntn)
    : Euler::Base(handler, std::forward<const index>(offset), pCreateInfo, pMaterials, pInstFntn,
                  PIPELINE::SHADOW_PARTICLE_FOUNTAIN_EULER) {
    Mesh::Torus::Info torusInfo = {};
    Mesh::Torus::make(torusInfo, vertices_, indices_);
    for (auto& instance : instances_) {
        if (instance.pMaterial == nullptr) {
            assert(false);
            continue;
        }
        FlagBits matFlags = instance.pMaterial->getFlags();
        matFlags |= Material::FLAG::IS_MESH;
        instance.pMaterial->setFlags(matFlags);
        handler.materialHandler().update(instance.pMaterial);
    }
    status_ |= PENDING_BUFFERS;
}

}  // namespace Euler

}  // namespace Buffer
}  // namespace Particle
