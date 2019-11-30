/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Cloth.h"

#include "Deferred.h"
#include "Random.h"
#include "Storage.h"
// HANDLERS
#include "DescriptorHandler.h"
#include "ParticleHandler.h"
#include "PipelineHandler.h"

// SHADER
namespace Shader {
namespace Particle {
const CreateInfo CLOTH_COMP_CREATE_INFO = {
    SHADER::PRTCL_CLOTH_COMP,         //
    "Particle Cloth Compute Shader",  //
    "comp.particle.cloth.glsl",       //
    VK_SHADER_STAGE_COMPUTE_BIT,      //
};
const CreateInfo CLOTH_NORM_COMP_CREATE_INFO = {
    SHADER::PRTCL_CLOTH_NORM_COMP,           //
    "Particle Cloth Normal Compute Shader",  //
    "comp.particle.cloth.norm.glsl",         //
    VK_SHADER_STAGE_COMPUTE_BIT,             //
};
const CreateInfo CLOTH_VERT_CREATE_INFO = {
    SHADER::PRTCL_CLOTH_VERT,        //
    "Particle Cloth Vertex Shader",  //
    "vert.cloth.cs.glsl",            //
    VK_SHADER_STAGE_VERTEX_BIT,      //
};
}  // namespace Particle
}  // namespace Shader

// UNIFORM DYNAMIC
namespace UniformDynamic {
namespace Particle {
namespace Cloth {

Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM_DYNAMIC::PRTCL_CLOTH),
      Buffer::PerFramebufferDataItem<DATA>(pData),
      deltas_{
          ::Particle::Cloth::INTERGRATION_STEP_PER_FRAME_IDEAL,
          ::Particle::Cloth::INTERGRATION_STEP_PER_FRAME_IDEAL,
          ::Particle::Cloth::INTERGRATION_STEP_PER_FRAME_IDEAL,
      },
      acceleration_(pCreateInfo->gravity) {
    data_.gravity = pCreateInfo->gravity;
    data_.springK = pCreateInfo->springK;
    data_.mass = pCreateInfo->mass;
    data_.dampingConst = pCreateInfo->dampingConst;

    data_.delta = ::Particle::Cloth::INTERGRATION_STEP_PER_FRAME_IDEAL;
    data_.inverseMass = 1.0f / data_.mass;
    // The algorithm doesn't expect divisions it expects number of row and columns.
    data_.restLengthHoriz = pCreateInfo->planeInfo.width / pCreateInfo->planeInfo.horzDivs;
    data_.restLengthVert = pCreateInfo->planeInfo.height / pCreateInfo->planeInfo.vertDivs;
    data_.restLengthDiag =
        sqrtf(data_.restLengthHoriz * data_.restLengthHoriz + data_.restLengthVert * data_.restLengthVert);
    setData();
}

//#include <iostream>
void Base::updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) {
    // delta
    data_.delta = ::Particle::Cloth::INTERGRATION_STEP_PER_FRAME_IDEAL;
    // data_.delta = elapsed * ::Particle::Cloth::INTEGRATION_STEP_PER_FRAME_FACTOR;

    // std::swap(deltas_[0], deltas_[1]);
    // std::swap(deltas_[1], deltas_[2]);
    // deltas_[2] = elapsed;
    // float avg = (deltas_[0] + deltas_[1] + deltas_[2]) / 3.0f;
    // data_.delta = elapsed * ::Particle::Cloth::INTEGRATION_STEP_PER_FRAME_FACTOR;
    // std::cout << data_.delta << std::endl;

    // acceleration
    auto rand = 1.0f - (Random::inst().nextFloatZeroToOne() * 1.0f);
    data_.gravity = acceleration_;
    data_.gravity *= rand;
    setData(frameIndex);
}

}  // namespace Cloth
}  // namespace Particle
}  // namespace UniformDynamic

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
namespace Particle {
const CreateInfo CLOTH_CREATE_INFO = {
    DESCRIPTOR_SET::PRTCL_CLOTH,
    "_DS_PRTCL_CLTH",
    {
        {{0, 0}, {UNIFORM_DYNAMIC::PRTCL_CLOTH}},
        {{1, 0}, {STORAGE_BUFFER_DYNAMIC::PRTCL_POSITION}},
        {{2, 0}, {STORAGE_BUFFER_DYNAMIC::PRTCL_POSITION}},
        {{3, 0}, {STORAGE_BUFFER_DYNAMIC::PRTCL_VELOCITY}},
        {{4, 0}, {STORAGE_BUFFER_DYNAMIC::PRTCL_VELOCITY}},
    },
};
const CreateInfo CLOTH_NORM_CREATE_INFO = {
    DESCRIPTOR_SET::PRTCL_CLOTH_NORM,
    "_DS_PRTCL_CLTH_NRM",
    {
        {{0, 0}, {STORAGE_BUFFER_DYNAMIC::PRTCL_POSITION}},
        {{1, 0}, {STORAGE_BUFFER_DYNAMIC::PRTCL_NORMAL}},
    },
};
}  // namespace Particle
}  // namespace Set
}  // namespace Descriptor

// PIPELINE
namespace Pipeline {
namespace Particle {

// CLOTH (COMPUTE)
const Pipeline::CreateInfo CLOTH_COMP_CREATE_INFO = {
    COMPUTE::PRTCL_CLOTH,
    "Particle Cloth Compute Pipeline",
    {SHADER::PRTCL_CLOTH_COMP},
    {DESCRIPTOR_SET::PRTCL_CLOTH},
};
ClothCompute::ClothCompute(Pipeline::Handler& handler) : Compute(handler, &CLOTH_COMP_CREATE_INFO) {}

// CLOTH NORMAL (COMPUTE)
const Pipeline::CreateInfo CLOTH_NORM_COMP_CREATE_INFO = {
    COMPUTE::PRTCL_CLOTH_NORM,
    "Particle Cloth Normal Compute Pipeline",
    {SHADER::PRTCL_CLOTH_NORM_COMP},
    {DESCRIPTOR_SET::PRTCL_CLOTH_NORM},
};
ClothNormalCompute::ClothNormalCompute(Pipeline::Handler& handler) : Compute(handler, &CLOTH_NORM_COMP_CREATE_INFO) {}

// CLOTH
const Pipeline::CreateInfo CLOTH_CREATE_INFO = {
    GRAPHICS::PRTCL_CLOTH_DEFERRED,
    "Particle Cloth (Deferred) Pipeline",
    {SHADER::PRTCL_CLOTH_VERT, SHADER::DEFERRED_MRT_TEX_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_DEFAULT,
        DESCRIPTOR_SET::SAMPLER_DEFAULT,
    },
};
Cloth::Cloth(Pipeline::Handler& handler) : Graphics(handler, &CLOTH_CREATE_INFO), DO_BLEND(false), IS_DEFERRED(true) {}

void Cloth::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}

void Cloth::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    Storage::Vector4::GetInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_VERTEX);  // Position
    Storage::Vector4::GetInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_VERTEX);  // Normal
    // Texture coordinate (TODO: if this ever gets reused move it somewhere appropriate)
    {
        const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
        createInfoRes.bindDescs.push_back({});
        createInfoRes.bindDescs.back().binding = BINDING;
        createInfoRes.bindDescs.back().stride = sizeof(::Particle::TEXCOORD);
        createInfoRes.bindDescs.back().inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        createInfoRes.attrDescs.push_back({});
        createInfoRes.attrDescs.back().binding = BINDING;
        createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
        createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32_SFLOAT;  // vec2
        createInfoRes.attrDescs.back().offset = 0;
    }
    Instance::Obj3d::DATA::getInputDescriptions(createInfoRes);

    // bindings
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = createInfoRes.bindDescs.data();
    // attributes
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = {};
    createInfoRes.inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    createInfoRes.inputAssemblyStateInfo.pNext = nullptr;
    createInfoRes.inputAssemblyStateInfo.flags = 0;
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_TRUE;
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
}

void Cloth::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    Graphics::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.cullMode = VK_CULL_MODE_NONE;
}

}  // namespace Particle
}  // namespace Pipeline

// BUFFER
namespace Particle {
namespace Buffer {
namespace Cloth {

Base::Base(Particle::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo,
           std::shared_ptr<Material::Base>& pMaterial, const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors,
           std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData)
    : Buffer::Base(handler, std::forward<const index>(offset), pCreateInfo, pMaterial, pDescriptors, GRAPHICS::SHADOW_TEX),
      Obj3d::InstanceDraw(pInstanceData),
      pPosBuffs_{nullptr, nullptr},
      pVelBuffs_{nullptr, nullptr},
      pNormBuff_(nullptr) {
    /**
     * The algorithm doesn't expect divisions it expects number of row and columns, but I don't want to change the name of
     * the variable atm. This means that for the simplest shape to be made you need at least two rows and two columns.
     */
    workgroupSize_.x = pCreateInfo->planeInfo.vertDivs;
    workgroupSize_.y = pCreateInfo->planeInfo.horzDivs;
    assert(workgroupSize_.y > 1 && workgroupSize_.x > 1);
    assert(pCreateInfo->planeInfo.width > 0.0f && pCreateInfo->planeInfo.height > 0.0f);

    // Initial positions of the particles
    std::vector<Storage::Vector4::DATA> posData;
    posData.reserve(static_cast<size_t>(workgroupSize_.y) * static_cast<size_t>(workgroupSize_.x));
    texCoords_.reserve(static_cast<size_t>(workgroupSize_.y) * static_cast<size_t>(workgroupSize_.x));

    float dx, dy, y, t;

    float right = pCreateInfo->planeInfo.width * 0.5f;
    float left = right * -1.0f;
    float top = pCreateInfo->planeInfo.height * 0.5f;
    float bottom = top * -1.0f;

    for (uint32_t i = 0; i < workgroupSize_.x; i++) {
        dy = static_cast<float>(i) / static_cast<float>(workgroupSize_.x - 1);
        y = glm::mix(top, bottom, dy);
        t = 0.0f + dy;

        for (uint32_t j = 0; j < workgroupSize_.y; j++) {
            dx = static_cast<float>(j) / static_cast<float>(workgroupSize_.y - 1);

            // Position
            posData.push_back({
                glm::mix(left, right, dx),  // x
                y,                          // y
                0.0f,                       // z
                1.0f,                       // w
            });

            // Transform
            posData.back() = pCreateInfo->geometryInfo.transform * posData.back();

            // Texture coordinate
            texCoords_.push_back({
                glm::mix(0.0f, 1.0f, dx),  // s
                t,                         // t
            });
        }
    }

    assert(indices_.empty());
    assert(!pCreateInfo->geometryInfo.doubleSided && "This won't work unless the normals & tex coord creation is changed");

    size_t numIndices =
        static_cast<size_t>(static_cast<size_t>(workgroupSize_.y) * (2 + ((static_cast<size_t>(workgroupSize_.x) - 2) * 2)) +
                            static_cast<size_t>(workgroupSize_.x) - 1);
    indices_.resize(numIndices);

    // Every row is one triangle strip
    size_t index = 0;
    for (uint32_t row = 0; row < workgroupSize_.x - 1; row++) {
        for (uint32_t col = 0; col < workgroupSize_.y; col++) {
            indices_[index + 1] = (row + 1) * workgroupSize_.y + col;
            indices_[index] = row * workgroupSize_.y + col;
            index += 2;
        }
        indices_[index] = VB_INDEX_PRIMITIVE_RESTART;
        index++;
    }

    // BUFFERS

    // POSITION
    Storage::Vector4::CreateInfo vec4Info({
        workgroupSize_.x,
        workgroupSize_.y,
        1,
    });
    assert(vec4Info.dataCount == posData.size());
    vec4Info.type = Storage::Vector4::TYPE::DONT_CARE;
    vec4Info.descType = STORAGE_BUFFER_DYNAMIC::PRTCL_POSITION;
    // 0
    handler.vec4Mgr.insert(handler.shell().context().dev, &vec4Info);
    pPosBuffs_[0] = std::static_pointer_cast<Storage::Vector4::Base>(handler.vec4Mgr.pItems.back());
    // 1
    handler.vec4Mgr.insert(handler.shell().context().dev, &vec4Info);
    pPosBuffs_[1] = std::static_pointer_cast<Storage::Vector4::Base>(handler.vec4Mgr.pItems.back());
    // TODO: Get rid of all these copies all over the place...
    for (VkDeviceSize i = 0; i < pPosBuffs_[0]->BUFFER_INFO.count; i++) {
        pPosBuffs_[0]->set(posData[i], i);
        // pPosBuffs_[1]->set(posData[i], i);
    }
    pPosBuffs_[0]->dirty = true;
    // pPosBuffs_[1]->dirty = true;
    handler.vec4Mgr.updateData(handler.shell().context().dev, pPosBuffs_[0]->BUFFER_INFO, -1);
    // handler.vec4Mgr.updateData(handler.shell().context().dev, pPosBuffs_[1]->BUFFER_INFO, -1);

    // VELOCITY
    vec4Info.type = Storage::Vector4::TYPE::DONT_CARE;
    vec4Info.descType = STORAGE_BUFFER_DYNAMIC::PRTCL_VELOCITY;
    // 0
    handler.vec4Mgr.insert(handler.shell().context().dev, &vec4Info);
    pVelBuffs_[0] = std::static_pointer_cast<Storage::Vector4::Base>(handler.vec4Mgr.pItems.back());
    // 1
    handler.vec4Mgr.insert(handler.shell().context().dev, &vec4Info);
    pVelBuffs_[1] = std::static_pointer_cast<Storage::Vector4::Base>(handler.vec4Mgr.pItems.back());

    // NORMAL
    vec4Info.descType = STORAGE_BUFFER_DYNAMIC::PRTCL_NORMAL;
    handler.vec4Mgr.insert(handler.shell().context().dev, &vec4Info);
    pNormBuff_ = std::static_pointer_cast<Storage::Vector4::Base>(handler.vec4Mgr.pItems.back());

    assert(pPosBuffs_[0]->BUFFER_INFO.count == pPosBuffs_[1]->BUFFER_INFO.count &&
           pVelBuffs_[0]->BUFFER_INFO.count == pVelBuffs_[1]->BUFFER_INFO.count &&
           pPosBuffs_[0]->BUFFER_INFO.count == pVelBuffs_[0]->BUFFER_INFO.count &&
           pPosBuffs_[0]->BUFFER_INFO.count == pNormBuff_->BUFFER_INFO.count);

    status_ |= STATUS::PENDING_BUFFERS;
    paused_ = false;
    draw_ = true;
}

void Base::draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                const uint8_t frameIndex) const {
    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                            static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                            descSetBindData.descriptorSets[setIndex].data(),
                            static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                            descSetBindData.dynamicOffsets.data());

    // VERTEX
    const VkBuffer buffers[] = {
        pPosBuffs_[0]->BUFFER_INFO.bufferInfo.buffer,
        pNormBuff_->BUFFER_INFO.bufferInfo.buffer,
        texCoordRes_.buffer,
        pInstObj3d_->BUFFER_INFO.bufferInfo.buffer,
    };
    const VkDeviceSize offsets[] = {
        pPosBuffs_[0]->BUFFER_INFO.memoryOffset,
        pNormBuff_->BUFFER_INFO.memoryOffset,
        0,
        pInstObj3d_->BUFFER_INFO.memoryOffset,
    };

    vkCmdBindVertexBuffers(  //
        cmd,                 // VkCommandBuffer commandBuffer
        0,                   // uint32_t firstBinding
        4,                   // uint32_t bindingCount
        buffers,             // const VkBuffer* pBuffers
        offsets              // const VkDeviceSize* pOffsets
    );

    // INDEX
    assert(indices_.size());
    vkCmdBindIndexBuffer(cmd, indexRes_.buffer, 0, VK_INDEX_TYPE_UINT32);

    // DRAW
    vkCmdDrawIndexed(                            //
        cmd,                                     // VkCommandBuffer commandBuffer
        static_cast<uint32_t>(indices_.size()),  // uint32_t indexCount
        getInstanceCount(),                      // uint32_t instanceCount
        0,                                       // uint32_t firstIndex
        0,                                       // int32_t vertexOffset
        0                                        // uint32_t firstInstance
    );
}

void Base::dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                    const Descriptor::Set::BindData&, const VkCommandBuffer& cmd, const uint8_t frameIndex) const {
    assert(LOCAL_SIZE.z == 1 && workgroupSize_.z == 1);

    if (pPipelineBindData->type == PIPELINE{COMPUTE::PRTCL_CLOTH}) {
        // POSITION / VELOCITY

        auto* pDescSetBindData0 = &computeDescSetBindDataMaps_[0].at({passType});
        auto* pDescSetBindData1 = &computeDescSetBindDataMaps_[1].at({passType});
        auto setIndex = (std::min)(static_cast<uint8_t>(pDescSetBindData0->descriptorSets.size() - 1), frameIndex);

        VkMemoryBarrier memoryBarrier = {
            VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
            VK_ACCESS_SHADER_WRITE_BIT,  // srcAccessMask
            VK_ACCESS_SHADER_READ_BIT,   // dstAccessMask
        };

        vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

        const int numIterations = ::Particle::Cloth::ITERATIONS_PER_FRAME_IDEAL;
        //    helpers::nearestOdd(handler().shell().getElapsedTime<float>() * ::Cloth::ITERATIONS_PER_FRAME_FACTOR);
        // std::cout << numIterations << std::endl;
        for (int i = 0; i < numIterations; i++) {
            vkCmdBindDescriptorSets(
                cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, pDescSetBindData0->firstSet,
                static_cast<uint32_t>(pDescSetBindData0->descriptorSets[setIndex].size()),
                pDescSetBindData0->descriptorSets[setIndex].data(),
                static_cast<uint32_t>(pDescSetBindData0->dynamicOffsets.size()), pDescSetBindData0->dynamicOffsets.data());

            vkCmdDispatch(cmd, workgroupSize_.x / LOCAL_SIZE.x, workgroupSize_.y / LOCAL_SIZE.y, 1);

            vkCmdPipelineBarrier(cmd,                                   //
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
                                 0,                                     // dependencyFlags
                                 1,                                     // memoryBarrierCount
                                 &memoryBarrier,                        // pMemoryBarriers
                                 0, nullptr, 0, nullptr);

            std::swap(pDescSetBindData0, pDescSetBindData1);
        }
    } else if (pPipelineBindData->type == PIPELINE{COMPUTE::PRTCL_CLOTH_NORM}) {
        // NORMAL

        auto* pDescSetBindData = &computeDescSetBindDataMaps_[2].at({passType});
        auto setIndex = (std::min)(static_cast<uint8_t>(pDescSetBindData->descriptorSets.size() - 1), frameIndex);

        vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

        vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, pDescSetBindData->firstSet,
                                static_cast<uint32_t>(pDescSetBindData->descriptorSets[setIndex].size()),
                                pDescSetBindData->descriptorSets[setIndex].data(),
                                static_cast<uint32_t>(pDescSetBindData->dynamicOffsets.size()),
                                pDescSetBindData->dynamicOffsets.data());

        vkCmdDispatch(cmd, workgroupSize_.x / LOCAL_SIZE.x, workgroupSize_.y / LOCAL_SIZE.y, 1);
    } else {
        assert(false && "Unhandled pipeline type");
    }
}

void Base::getComputeDescSetBindData() {
    assert(computeDescSetBindDataMaps_.empty());
    std::vector<Descriptor::Base*> pDynamicItems;

    // CLOTH
    assert(COMPUTE_PIPELINE_TYPES.at(0) == COMPUTE::PRTCL_CLOTH);
    for (const auto& pDesc : pDescriptors_) pDynamicItems.push_back(pDesc.get());
    // Read from pos/vel 0 write to pos/vel 1
    pDynamicItems.push_back(pPosBuffs_[0].get());
    pDynamicItems.push_back(pPosBuffs_[1].get());
    pDynamicItems.push_back(pVelBuffs_[0].get());
    pDynamicItems.push_back(pVelBuffs_[1].get());
    computeDescSetBindDataMaps_.emplace_back();
    handler().descriptorHandler().getBindData(COMPUTE_PIPELINE_TYPES.at(0), computeDescSetBindDataMaps_.back(),
                                              pDynamicItems);

    // Read from pos/vel 1 write to pos/vel 0
    std::swap(pDynamicItems[1], pDynamicItems[2]);
    std::swap(pDynamicItems[3], pDynamicItems[4]);
    computeDescSetBindDataMaps_.emplace_back();
    handler().descriptorHandler().getBindData(COMPUTE_PIPELINE_TYPES.at(0), computeDescSetBindDataMaps_.back(),
                                              pDynamicItems);

    // CLOTH NORMAL
    assert(COMPUTE_PIPELINE_TYPES.at(1) == COMPUTE::PRTCL_CLOTH_NORM && computeDescSetBindDataMaps_.size() == 2);
    pDynamicItems.clear();
    // Read from pos 0 write to norm
    pDynamicItems.push_back(pPosBuffs_[0].get());
    pDynamicItems.push_back(pNormBuff_.get());
    computeDescSetBindDataMaps_.emplace_back();
    handler().descriptorHandler().getBindData(COMPUTE_PIPELINE_TYPES.at(1), computeDescSetBindDataMaps_.back(),
                                              pDynamicItems);
}

}  // namespace Cloth
}  // namespace Buffer
}  // namespace Particle
