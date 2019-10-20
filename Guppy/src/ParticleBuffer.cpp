
#include "ParticleBuffer.h"

#include "Random.h"
// HANDLERS
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "ParticleHandler.h"

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
      Buffer::DataItem<DATA>(pData) {
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

//// FOUNTAIN (TRANSFORM FEEDBACK)
//
// namespace FountainTF {
//
// void DATA::getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes, const VkVertexInputRate inputRate) {
//    const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
//    createInfoRes.bindDescs.push_back({});
//    createInfoRes.bindDescs.back().binding = BINDING;
//    createInfoRes.bindDescs.back().stride = sizeof(DATA);
//    createInfoRes.bindDescs.back().inputRate = inputRate;
//
//    // position
//    createInfoRes.attrDescs.push_back({});
//    createInfoRes.attrDescs.back().binding = BINDING;
//    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
//    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
//    createInfoRes.attrDescs.back().offset = offsetof(DATA, position);
//    // velocity
//    createInfoRes.attrDescs.push_back({});
//    createInfoRes.attrDescs.back().binding = BINDING;
//    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
//    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
//    createInfoRes.attrDescs.back().offset = offsetof(DATA, velocity);
//    // age
//    createInfoRes.attrDescs.push_back({});
//    createInfoRes.attrDescs.back().binding = BINDING;
//    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
//    createInfoRes.attrDescs.back().format = VK_FORMAT_R32_SFLOAT;  // float
//    createInfoRes.attrDescs.back().offset = offsetof(DATA, age);
//}
//
// Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
//    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
//      Buffer::DataItem<DATA>(pData) {
//    assert(BUFFER_INFO.count % 2 == 0);
//    auto numParticles = BUFFER_INFO.count / 2;
//
//    // Fill the first age buffer
//    float rate = pCreateInfo->pInfo->lifespan / numParticles;
//    for (uint64_t i = 0; i < numParticles; i++) {
//        pData_[i].age = rate * (i - numParticles);
//    }
//
//    dirty = true;
//}
//
//}  // namespace FountainTF

}  // namespace Particle
}  // namespace Instance

// BUFFER

namespace Particle {
namespace Buffer {

// BASE

Base::Base(Particle::Handler& handler, const index offset, const std::string&& name, const PIPELINE& pipelineType,
           const std::vector<std::shared_ptr<Material::Base>>& pMaterials)
    : Handlee(handler), NAME(name), PIPELINE_TYPE(pipelineType), status_(STATUS::READY), offset_(offset) {
    assert(PIPELINE_TYPE != PIPELINE::ALL_ENUM);
    // TODO: This should work if a material is not needed...
    assert(pMaterials.size() && "Can't handle particle buffers without a material currently.");
    for (auto& pMaterial : pMaterials) instances_.push_back({{}, pMaterial});
    if (pMaterials.size()) status_ |= STATUS::PENDING_MATERIAL;
}

void Base::prepare() {
    if (status_ & STATUS::PENDING_BUFFERS) {
        assert(false);  // This code should work but was not tested.
        loadBuffers();
        // Submit vertex loading commands...
        handler().loadingHandler().loadSubmit(std::move(pLdgRes_));
        status_ ^= STATUS::PENDING_BUFFERS;
    }

    if (status_ & STATUS::PENDING_MATERIAL) {
        bool pendingMaterial = false;
        for (const auto& instance : instances_) pendingMaterial |= instance.second->getStatus() != STATUS::READY;
        if (!pendingMaterial) status_ ^= STATUS::PENDING_MATERIAL;
    }

    // TODO: see comment in Mesh::Base::prepare about descriptors.
    if (status_ == STATUS::READY) {
        for (auto& instance : instances_) {
            handler().descriptorHandler().getBindData(PIPELINE_TYPE, instance.first, instance.second,
                                                      instance.second->getTexture());
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
}

void Base::destroy() {
    auto& dev = handler().shell().context().dev;
    // vertex
    vkDestroyBuffer(dev, vertexRes_.buffer, nullptr);
    vkFreeMemory(dev, vertexRes_.memory, nullptr);
}

const Descriptor::Set::BindData& Base::getDescriptorSetBindData(const PASS& passType, const instancePair& instance) const {
    for (const auto& [passTypes, bindData] : instance.first) {
        if (passTypes.find(passType) != passTypes.end()) return bindData;
    }
    return instance.first.at(Uniform::PASS_ALL_SET);
}

// FOUNTAIN

Fountain::Fountain(Particle::Handler& handler, const index&& offset, const ::Particle::Fountain::CreateInfo* pCreateInfo,
                   const std::vector<std::shared_ptr<Material::Base>>& pMaterials,
                   std::shared_ptr<::Instance::Particle::Fountain::Base>& pInstFntn)
    : Base(handler, std::forward<const index>(offset), "Particle Fountain Buffer", pCreateInfo->pipelineType, pMaterials),
      pInstFntn_(pInstFntn) {
    assert(getInstances().size());
    assert(pInstFntn_ != nullptr);
}

void Fountain::start(const StartInfo& info) {
    if (info.instanceOffsets.empty()) {
        // Using empty offsets vector to indicate start all.
        for (auto& instance : getInstances())
            std::static_pointer_cast<Material::Particle::Fountain::Base>(instance.second)->start();
    } else {
        // Start the instances with and offset specified.
        for (const auto& offset : info.instanceOffsets)
            std::static_pointer_cast<Material::Particle::Fountain::Base>(getInstances().at(offset).second)->start();
    }
}

void Fountain::update(const float time, const uint32_t frameIndex) {
    float lastTimeOfBirth = pInstFntn_->getLastTimeOfBirth();
    for (const auto& instance : getInstances()) {
        std::static_pointer_cast<Material::Particle::Fountain::Base>(instance.second)
            ->update(time, lastTimeOfBirth, frameIndex);
        handler().materialHandler().update(instance.second, frameIndex);
    }
}

bool Fountain::shouldDraw(const instancePair& instance) {
    bool draw = false;
    if (status_ == STATUS::READY) {
        auto lastTimeOfBirth = pInstFntn_->getLastTimeOfBirth();
        draw = std::static_pointer_cast<Material::Particle::Fountain::Base>(instance.second)->shouldDraw(lastTimeOfBirth);
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

//// FOUNTAIN (TRANSFORM FEEDBACK)
//
// FountainTF::FountainTF(Particle::Handler& handler, const index&& offset, const ::Particle::Fountain::CreateInfo*
// pCreateInfo,
//                       const std::vector<std::shared_ptr<Material::Base>>& pMaterials,
//                       std::shared_ptr<::Instance::Particle::FountainTF::Base>& pInstFntn)
//    : Base(handler, std::forward<const index>(offset), "Particle Fountain Transform Feedback Buffer",
//           pCreateInfo->pipelineType, pMaterials),
//      pInstFntn_(pInstFntn),
//      flip_(false) {
//    assert(getInstances().size());
//    assert(pInstFntn_ != nullptr);
//}
//
// void FountainTF::start(const StartInfo& info) {
//    assert(false);
//    // if (info.instanceOffsets.empty()) {
//    //    // Using empty offsets vector to indicate start all.
//    //    for (auto& instance : getInstances())
//    //        std::static_pointer_cast<Material::Particle::Fountain::Base>(instance.second)->start();
//    //} else {
//    //    // Start the instances with and offset specified.
//    //    for (const auto& offset : info.instanceOffsets)
//    //        std::static_pointer_cast<Material::Particle::Fountain::Base>(getInstances().at(offset).second)->start();
//    //}
//}
//
// void FountainTF::update(const float time, const uint32_t frameIndex) {
//    return;
//    // float lastTimeOfBirth = pInstFntn_->getLastTimeOfBirth();
//    // for (const auto& instance : getInstances()) {
//    //    std::static_pointer_cast<Material::Particle::Fountain::Base>(instance.second)
//    //        ->update(time, lastTimeOfBirth, frameIndex);
//    //    handler().materialHandler().update(instance.second, frameIndex);
//    //}
//}
//
// bool FountainTF::shouldDraw(const instancePair& instance) {
//    bool draw = true;
//    // if (status_ == STATUS::READY) {
//    //    auto lastTimeOfBirth = pInstFntn_->getLastTimeOfBirth();
//    //    draw =
//    std::static_pointer_cast<Material::Particle::Fountain::Base>(instance.second)->shouldDraw(lastTimeOfBirth);
//    //}
//    return draw;
//}
//
// void FountainTF::draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
//                      const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
//                      const uint8_t frameIndex) const {
//    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);
//
//    vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);
//
//    vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
//                            static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
//                            descSetBindData.descriptorSets[setIndex].data(),
//                            static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
//                            descSetBindData.dynamicOffsets.data());
//
//    // Instance Fountain
//    vkCmdBindVertexBuffers(                          //
//        cmd,                                         // VkCommandBuffer commandBuffer
//        1,                                           // uint32_t firstBinding
//        1,                                           // uint32_t bindingCount
//        &pInstFntn_->BUFFER_INFO.bufferInfo.buffer,  // const VkBuffer* pBuffers
//        &pInstFntn_->BUFFER_INFO.memoryOffset        // const VkDeviceSize* pOffsets
//    );
//
//    vkCmdBeginTransformFeedbackEXT(  //
//        cmd,                         // VkCommandBuffer commandBuffer
//                                     // uint32_t firstCounterBuffer
//                                     // uint32_t counterBufferCount
//                                     // const VkBuffer* pCounterBuffers
//                                     // const VkDeviceSize* pCounterBufferOffsets
//    );
//
//    // VkDeviceSize size = (pInstFntn_->BUFFER_INFO.bufferInfo.range * pInstFntn_->BUFFER_INFO.count) / 2;
//    VkDeviceSize size = (pInstFntn_->BUFFER_INFO.bufferInfo.range * pInstFntn_->BUFFER_INFO.count) / 8;
//    VkDeviceSize offset = (flip_) ? size : 0;
//    ext::vkCmdBindTransformFeedbackBuffers(          //
//        cmd,                                         // VkCommandBuffer commandBuffer
//        0,                                           // uint32_t firstBinding
//        1,                                           // uint32_t bindingCount
//        &pInstFntn_->BUFFER_INFO.bufferInfo.buffer,  // const VkBuffer* pBuffers
//        &offset,                                     // const VkDeviceSize* pOffsets
//        &size                                        // const VkDeviceSize* pSizes
//    );
//
//    // vkCmdDraw(                          //
//    //    cmd,                            // VkCommandBuffer commandBuffer
//    //    6,                              // uint32_t vertexCount
//    //    pInstFntn_->BUFFER_INFO.count,  // uint32_t instanceCount
//    //    0,                              // uint32_t firstVertex
//    //    0                               // uint32_t firstInstance
//    //);
//}

}  // namespace Buffer
}  // namespace Particle
