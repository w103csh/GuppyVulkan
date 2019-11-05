
#include "ParticleBuffer.h"

#include "Random.h"
#include "Torus.h"
// HANDLERS
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "ParticleHandler.h"
#include "PipelineHandler.h"
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
      Instance::Base(STORAGE_BUFFER_DYNAMIC::PRTCL_EULER) {
    float rate = pCreateInfo->pInfo->lifespan / BUFFER_INFO.count;
    for (uint64_t i = 0; i < BUFFER_INFO.count; i++) {
        // age
        pData_[i].data1[3] = rate * (static_cast<float>(i) - static_cast<float>(BUFFER_INFO.count));
    }

    dirty = true;
}

}  // namespace FountainEuler

// VECTOR4
namespace Vector4 {

void DATA::getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes) {
    const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.bindDescs.push_back({});
    createInfoRes.bindDescs.back().binding = BINDING;
    createInfoRes.bindDescs.back().stride = sizeof(DATA);
    createInfoRes.bindDescs.back().inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
    createInfoRes.attrDescs.back().offset = offsetof(DATA, data);
}

Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::DataItem<DATA>(pData),
      Instance::Base(pCreateInfo->descType),
      VECTOR_TYPE(pCreateInfo->type) {
    assert(std::visit(Descriptor::GetStorageBufferDynamic{}, getDescriptorType()) != STORAGE_BUFFER_DYNAMIC::DONT_CARE);
    switch (VECTOR_TYPE) {
        case TYPE::ATTRACTOR_POSITION: {
            assert(BUFFER_INFO.count ==
                   pCreateInfo->numParticles.x * pCreateInfo->numParticles.y * pCreateInfo->numParticles.z);
            // Initial positions of the particles
            glm::vec4 p{0.0f, 0.0f, 0.0f, 1.0f};
            float dx = 2.0f / (pCreateInfo->numParticles.x - 1), dy = 2.0f / (pCreateInfo->numParticles.y - 1),
                  dz = 2.0f / (pCreateInfo->numParticles.z - 1);
            // We want to center the particles at (0,0,0)
            glm::mat4 transf = glm::translate(glm::mat4(1.0f), glm::vec3(-1, -1, -1));
            uint32_t idx = 0;
            for (uint32_t i = 0; i < pCreateInfo->numParticles.x; i++) {
                for (uint32_t j = 0; j < pCreateInfo->numParticles.y; j++) {
                    for (uint32_t k = 0; k < pCreateInfo->numParticles.z; k++) {
                        pData_[idx].data.x = dx * i;
                        pData_[idx].data.y = dy * j;
                        pData_[idx].data.z = dz * k;
                        pData_[idx].data.w = 1.0f;
                        pData_[idx].data = transf * pData_[idx].data;
                        idx++;
                    }
                }
            }
        } break;
        default:;
    }
    dirty = true;
}

}  // namespace Vector4

}  // namespace Particle
}  // namespace Instance

// BUFFER

namespace Particle {
namespace Buffer {

// BASE

Base::Base(Particle::Handler& handler, const index offset, const CreateInfo* pCreateInfo,
           const std::vector<std::shared_ptr<Material::Obj3d::Default>>& pObj3dMaterials,
           std::vector<std::shared_ptr<Descriptor::Base>>&& pUniforms, const PIPELINE&& pipelineTypeShadow)
    : Handlee(handler),
      NAME(pCreateInfo->name),
      PIPELINE_TYPE_COMPUTE(pCreateInfo->pipelineTypeCompute),
      PIPELINE_TYPE_GRAPHICS(pCreateInfo->pipelineTypeGraphics),
      PIPELINE_TYPE_SHADOW(pipelineTypeShadow),
      status_(STATUS::READY),
      paused_(true),
      timeUniformOffset_(BAD_OFFSET),
      instUniformOffsets_{BAD_OFFSET, BAD_OFFSET},
      pUniforms_(pUniforms),
      offset_(offset) {
    assert(!(PIPELINE_TYPE_COMPUTE == PIPELINE::ALL_ENUM && PIPELINE_TYPE_GRAPHICS == PIPELINE::ALL_ENUM));

    assert(pObj3dMaterials.size() && "Can't handle particle buffers without an obj3d material currently.");
    for (auto& pMaterial : pObj3dMaterials) instances_.push_back({pMaterial});

    int instOffset = -1;
    for (uint32_t i = 0; i < static_cast<uint32_t>(pUniforms_.size()); i++) {
        assert(pUniforms_[i] != nullptr && "Uniform data must be created");
        if (pUniforms_[i]->getDescriptorType() == DESCRIPTOR{UNIFORM_DYNAMIC::PRTCL_FOUNTAIN} ||
            pUniforms_[i]->getDescriptorType() == DESCRIPTOR{UNIFORM_DYNAMIC::PRTCL_ATTRACTOR}) {
            if (timeUniformOffset_ == BAD_OFFSET) {
                timeUniformOffset_ = 0;
            } else {
                assert(false && "Only one timed uniform works atm.");
                exit(EXIT_FAILURE);
            }
        }
        // This type check is not great.
        if (std::visit(Descriptor::IsStorageBufferDynamic{}, pUniforms_[i]->getDescriptorType())) {
            instUniformOffsets_.at(++instOffset) = i;
        }
    }
    assert(timeUniformOffset_ != BAD_OFFSET);
    assert(instUniformOffsets_[0] != BAD_OFFSET);

    if (instances_.size()) status_ |= STATUS::PENDING_MATERIAL;
}

void Base::toggle() { paused_ = !paused_; }

void Base::update(const float time, const float elapsed, const uint32_t frameIndex) {
    if (paused_) return;
    getTimedUniform()->update(time, elapsed, frameIndex);
    handler().update(getTimedUniform(), frameIndex);
}

bool Base::shouldDraw() { return status_ == STATUS::READY && !paused_; }

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
        for (const auto& instance : instances_) pendingMaterial |= instance.pObj3dMaterial->getStatus() != STATUS::READY;
        if (!pendingMaterial) status_ ^= STATUS::PENDING_MATERIAL;
    }

    // TODO: see comment in Mesh::Base::prepare about descriptors.
    if (status_ == STATUS::READY) {
        for (auto& instance : instances_) {
            if (PIPELINE_TYPE_COMPUTE != PIPELINE::ALL_ENUM) {
                handler().descriptorHandler().getBindData(PIPELINE_TYPE_COMPUTE, instance.computeDescSetBindDataMap,
                                                          getSDynamicDataItems(PIPELINE_TYPE_COMPUTE, instance));
            }
            if (PIPELINE_TYPE_GRAPHICS != PIPELINE::ALL_ENUM) {
                handler().descriptorHandler().getBindData(PIPELINE_TYPE_GRAPHICS, instance.graphicsDescSetBindDataMap,
                                                          getSDynamicDataItems(PIPELINE_TYPE_GRAPHICS, instance));
            }
            if (PIPELINE_TYPE_SHADOW != PIPELINE::ALL_ENUM) {
                std::set<PASS> passTypes;
                handler().passHandler().getActivePassTypes(passTypes, PIPELINE_TYPE_SHADOW);
                if (passTypes.size()) {
                    handler().descriptorHandler().getBindData(PIPELINE_TYPE_SHADOW, instance.shadowDescSetBindDataMap,
                                                              getSDynamicDataItems(PIPELINE_TYPE_SHADOW, instance));
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

const std::vector<Descriptor::Base*> Base::getSDynamicDataItems(const PIPELINE pipelineType,
                                                                const InstanceInfo& instance) const {
    std::vector<Descriptor::Base*> pDescs;
    for (const auto descSetType : handler().pipelineHandler().getPipeline(pipelineType)->DESCRIPTOR_SET_TYPES) {
        const auto& descSet = handler().descriptorHandler().getDescriptorSet(descSetType);
        for (const auto& [key, bindingInfo] : descSet.getBindingMap()) {
            if (std::visit(Descriptor::IsDynamic{}, bindingInfo.descType)) {
                if (instance.pObj3dMaterial != nullptr &&
                    instance.pObj3dMaterial->getDescriptorType() == bindingInfo.descType) {
                    // PER-INSTANCE
                    pDescs.push_back(instance.pObj3dMaterial.get());
                } else {
                    // PER-BUFFER
                    auto it = std::find_if(pUniforms_.begin(), pUniforms_.end(), [&bindingInfo](const auto& pDesc) {
                        return bindingInfo.descType == pDesc->getDescriptorType();
                    });
                    if (it == pUniforms_.end()) {
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

// FOUNTAIN

Fountain::Fountain(Particle::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo,
                   const std::vector<std::shared_ptr<Material::Obj3d::Default>>& pObj3dMaterials,
                   std::vector<std::shared_ptr<Descriptor::Base>>&& pUniforms)
    : Base(handler, std::forward<const index>(offset), pCreateInfo, pObj3dMaterials,
           std::forward<std::vector<std::shared_ptr<Descriptor::Base>>>(pUniforms)) {
    assert(instances_.size());
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

    auto pInstData = pUniforms_.at(getInstUniformOffsets()[0]);

    // Instance Fountain
    vkCmdBindVertexBuffers(                         //
        cmd,                                        // VkCommandBuffer commandBuffer
        0,                                          // uint32_t firstBinding
        1,                                          // uint32_t bindingCount
        &pInstData->BUFFER_INFO.bufferInfo.buffer,  // const VkBuffer* pBuffers
        &pInstData->BUFFER_INFO.memoryOffset        // const VkDeviceSize* pOffsets
    );

    vkCmdDraw(                         //
        cmd,                           // VkCommandBuffer commandBuffer
        6,                             // uint32_t vertexCount
        pInstData->BUFFER_INFO.count,  // uint32_t instanceCount
        0,                             // uint32_t firstVertex
        0                              // uint32_t firstInstance
    );
}

void Fountain::update(const float time, const float elapsed, const uint32_t frameIndex) {
    if (paused_) return;
    getTimedUniform()->update(time, elapsed, frameIndex);
    auto lastTimeOfBirth =
        std::static_pointer_cast<Instance::Particle::Fountain::Base>(pUniforms_.at(getInstUniformOffsets()[0]))
            ->getLastTimeOfBirth();
    auto& pTimedUniform = std::static_pointer_cast<UniformDynamic::Particle::Fountain::Base>(getTimedUniform());
    if (pTimedUniform->getDelta() > (lastTimeOfBirth + pTimedUniform->getLifespan() + 0.0001f)) {
        pTimedUniform->reset();
        paused_ = true;
    }
    handler().update(getTimedUniform(), frameIndex);
}

// EULER

namespace Euler {

Base::Base(Particle::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo,
           const std::vector<std::shared_ptr<Material::Obj3d::Default>>& pObj3dMaterials,
           std::vector<std::shared_ptr<Descriptor::Base>>&& pUniforms, const PIPELINE&& pipelineTypeShadow)
    : Buffer::Base(handler, std::forward<const index>(offset), pCreateInfo, pObj3dMaterials,
                   std::forward<std::vector<std::shared_ptr<Descriptor::Base>>>(pUniforms),
                   std::forward<const PIPELINE>(pipelineTypeShadow)),
      LOCAL_SIZE(pCreateInfo->localSize),
      pushConstant_(pCreateInfo->computeFlag),
      firstInstanceBinding_(pCreateInfo->firstInstanceBinding) {
    assert(instances_.size());
    assert(pUniforms_.size());
    assert(pUniforms_.at(getInstUniformOffsets()[0])->BUFFER_INFO.count % LOCAL_SIZE.x == 0);
}

void Base::draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                const uint8_t frameIndex) const {
    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    if (pPipelineBindData->type != PIPELINE::PRTCL_SHDW_FOUNTAIN_EULER && pushConstant_ != Particle::Euler::FLAG::NONE) {
        vkCmdPushConstants(cmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                           static_cast<uint32_t>(sizeof(pushConstant_)), &pushConstant_);
    }

    vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                            static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                            descSetBindData.descriptorSets[setIndex].data(),
                            static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                            descSetBindData.dynamicOffsets.data());

    auto pInstData = pUniforms_.at(getInstUniformOffsets()[0]);

    // Instance
    vkCmdBindVertexBuffers(                         //
        cmd,                                        // VkCommandBuffer commandBuffer
        firstInstanceBinding_,                      // uint32_t firstBinding
        1,                                          // uint32_t bindingCount
        &pInstData->BUFFER_INFO.bufferInfo.buffer,  // const VkBuffer* pBuffers
        &pInstData->BUFFER_INFO.memoryOffset        // const VkDeviceSize* pOffsets
    );

    if (vertices_.size() && indices_.size()) {
        assert(vertices_.size() && indices_.size());
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(cmd, Vertex::BINDING, 1, &vertexRes_.buffer, offsets);
        vkCmdBindIndexBuffer(cmd, indexRes_.buffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(                            //
            cmd,                                     // VkCommandBuffer commandBuffer
            static_cast<uint32_t>(indices_.size()),  // uint32_t indexCount
            pInstData->BUFFER_INFO.count,            // uint32_t instanceCount
            0,                                       // uint32_t firstIndex
            0,                                       // int32_t vertexOffset
            0                                        // uint32_t firstInstance
        );

    } else {
        // Draw a billboarded quad which does not require a vertex bind
        vkCmdDraw(                         //
            cmd,                           // VkCommandBuffer commandBuffer
            6,                             // uint32_t vertexCount
            pInstData->BUFFER_INFO.count,  // uint32_t instanceCount
            0,                             // uint32_t firstVertex
            0                              // uint32_t firstInstance
        );
    }
}

void Base::dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                    const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                    const uint8_t frameIndex) const {
    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    if (pushConstant_ != Particle::Euler::FLAG::NONE) {
        vkCmdPushConstants(cmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                           static_cast<uint32_t>(sizeof(pushConstant_)), &pushConstant_);
    }

    vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                            static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                            descSetBindData.descriptorSets[setIndex].data(),
                            static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                            descSetBindData.dynamicOffsets.data());

    assert(LOCAL_SIZE.y == 1 && LOCAL_SIZE.z == 1);  // Haven't thought about anything else.

    vkCmdDispatch(cmd, pUniforms_.at(getInstUniformOffsets()[0])->BUFFER_INFO.count / LOCAL_SIZE.x, 1, 1);
}

// TORUS

Torus::Torus(Particle::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo,
             const std::vector<std::shared_ptr<Material::Obj3d::Default>>& pObj3dMaterials,
             std::vector<std::shared_ptr<Descriptor::Base>>&& pUniforms)
    : Euler::Base(handler, std::forward<const index>(offset), pCreateInfo, pObj3dMaterials,
                  std::forward<std::vector<std::shared_ptr<Descriptor::Base>>>(pUniforms),
                  PIPELINE::PRTCL_SHDW_FOUNTAIN_EULER) {
    Mesh::Torus::Info torusInfo = {};
    Mesh::Torus::make(torusInfo, vertices_, indices_);
    for (auto& instance : instances_) {
        if (instance.pObj3dMaterial == nullptr) {
            assert(false);
            continue;
        }
        FlagBits matFlags = instance.pObj3dMaterial->getFlags();
        matFlags |= Material::FLAG::IS_MESH;
        instance.pObj3dMaterial->setFlags(matFlags);
        handler.materialHandler().update(instance.pObj3dMaterial);
    }
    status_ |= PENDING_BUFFERS;
}

}  // namespace Euler

}  // namespace Buffer
}  // namespace Particle
