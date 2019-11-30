/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "HeightFieldFluid.h"

#include <cmath>

#include "Deferred.h"
#include "Random.h"
// HANDLERS
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "ParticleHandler.h"
#include "PipelineHandler.h"
#include "TextureHandler.h"

namespace HeightFieldFluid {
void VertexData::getInputDescriptions(Pipeline::CreateInfoResources& createInfoRes, const VkVertexInputRate&& inputRate) {
    const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.bindDescs.push_back({});
    createInfoRes.bindDescs.back().binding = BINDING;
    createInfoRes.bindDescs.back().stride = sizeof(::HeightFieldFluid::VertexData);
    createInfoRes.bindDescs.back().inputRate = inputRate;

    // position
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
    createInfoRes.attrDescs.back().offset = offsetof(::HeightFieldFluid::VertexData, position);
    // imageOffset
    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32_SINT;  // ivec2
    createInfoRes.attrDescs.back().offset = offsetof(::HeightFieldFluid::VertexData, imageOffset);
}
}  // namespace HeightFieldFluid

// SHADER
namespace Shader {
const CreateInfo HFF_COMP_CREATE_INFO = {
    SHADER::HFF_HGHT_COMP,
    "Height Field Fluid Height Compute Shader",
    "comp.hff.hght.glsl",
    VK_SHADER_STAGE_COMPUTE_BIT,
};
const CreateInfo HFF_NORM_COMP_CREATE_INFO = {
    SHADER::HFF_NORM_COMP,
    "Height Field Fluid Normal Compute Shader",
    "comp.hff.norm.glsl",
    VK_SHADER_STAGE_COMPUTE_BIT,
};
const CreateInfo HFF_VERT_CREATE_INFO = {
    SHADER::HFF_VERT,
    "Height Field Fluid Vertex Shader",
    "vert.hff.cs.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,
};
const CreateInfo HFF_CLMN_VERT_CREATE_INFO = {
    SHADER::HFF_CLMN_VERT,
    "Height Field Fluid Column Vertex Shader",
    "vert.hff.column.cs.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,
};
}  // namespace Shader

// DESCRIPTOR SET
namespace Descriptor {
namespace Set {
const CreateInfo HFF_CREATE_INFO = {
    DESCRIPTOR_SET::HFF,
    "_DS_HFF",
    {
        {{2, 0}, {UNIFORM_DYNAMIC::HFF}},
        {{3, 0}, {STORAGE_IMAGE::PIPELINE, Texture::HFF_ID}},
        {{4, 0}, {STORAGE_BUFFER_DYNAMIC::NORMAL}},
    },
};
const CreateInfo HFF_DEF_CREATE_INFO = {
    DESCRIPTOR_SET::HFF_DEF,
    "_DS_HFF_DEF",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_DEFAULT}},
        {{2, 0}, {UNIFORM_DYNAMIC::HFF}},
        {{3, 0}, {STORAGE_IMAGE::PIPELINE, Texture::HFF_ID}},
    },
};
}  // namespace Set
}  // namespace Descriptor

// UNIFORM DYNAMIC
namespace UniformDynamic {
namespace HeightFieldFluid {
namespace Simulation {
Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM_DYNAMIC::HFF),
      Buffer::PerFramebufferDataItem<DATA>(pData) {
    c_ = pCreateInfo->c;
    data_.c2 = c_ * c_;
    data_.h = pCreateInfo->info.lengthM / static_cast<float>(pCreateInfo->info.M - 1);
    data_.h2 = data_.h * data_.h;
    data_.dt = ::Particle::BAD_TIME;
    data_.maxSlope = pCreateInfo->maxSlope;
    data_.read = 1;
    data_.write = 0;
    data_.mMinus1 = pCreateInfo->info.M - 1;
    data_.nMinus1 = pCreateInfo->info.N - 1;
    setData();
}
void Base::setWaveSpeed(const float c, const uint32_t frameIndex) {
    c_ = c;
    data_.c2 = c_ * c_;
    setData(frameIndex);
}
void Base::updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) {
    if (frameIndex == Descriptor::PAUSED_UPDATE) {
        setData();
        return;
    }
    // dt < h/c Courant-Friedrichs-Lewy (CFL) condition
    auto h_c = data_.h / c_;
    if (elapsed >= h_c) {
        data_.dt = h_c - 0.00001f;
    } else {
        // c < h / dt
        auto h_dt = data_.h / elapsed;
        if (c_ >= h_dt) {
            data_.c2 = (h_dt * h_dt) - 0.00001f;
        }
        data_.dt = elapsed;
    }
    std::swap(data_.read, data_.write);
    setData(frameIndex);
}
}  // namespace Simulation
}  // namespace HeightFieldFluid
}  // namespace UniformDynamic

// PIPELINE
namespace Pipeline {
namespace HeightFieldFluid {

// HEIGHT (COMPUTE)
const Pipeline::CreateInfo HFF_COMP_CREATE_INFO = {
    COMPUTE::HFF_HGHT,
    "Height Fluid Field Compute Pipeline",
    {SHADER::HFF_HGHT_COMP},
    {DESCRIPTOR_SET::HFF},
};
Height::Height(Pipeline::Handler& handler) : Pipeline::Compute(handler, &HFF_COMP_CREATE_INFO) {}

// NORMAL (COMPUTE)
const Pipeline::CreateInfo HFF_NORM_COMP_CREATE_INFO = {
    COMPUTE::HFF_NORM,
    "Height Fluid Field Normal Compute Pipeline",
    {SHADER::HFF_NORM_COMP},
    {DESCRIPTOR_SET::HFF},
};
Normal::Normal(Pipeline::Handler& handler) : Pipeline::Compute(handler, &HFF_NORM_COMP_CREATE_INFO) {}

// COLUMN
const Pipeline::CreateInfo HFF_CLMN_CREATE_INFO = {
    GRAPHICS::HFF_CLMN_DEFERRED,
    "Height Field Fluid Column (Deferred) Pipeline",
    {SHADER::HFF_CLMN_VERT, SHADER::DEFERRED_MRT_COLOR_FRAG},
    {DESCRIPTOR_SET::HFF_DEF},
    {},
    {PUSH_CONSTANT::HFF_COLUMN},
};
Column::Column(Pipeline::Handler& handler) : Graphics(handler, &HFF_CLMN_CREATE_INFO), DO_BLEND(false), IS_DEFERRED(true) {}

void Column::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}

void Column::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    ::HeightFieldFluid::VertexData::getInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_INSTANCE);

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
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

// WIREFRAME
const Pipeline::CreateInfo HFF_WF_CREATE_INFO = {
    GRAPHICS::HFF_WF_DEFERRED,
    "Height Field Fluid Wireframe (Deferred) Pipeline",
    {SHADER::HFF_VERT, SHADER::DEFERRED_MRT_COLOR_FRAG},
    {DESCRIPTOR_SET::HFF_DEF},
};
Wireframe::Wireframe(Pipeline::Handler& handler)
    : Graphics(handler, &HFF_WF_CREATE_INFO), DO_BLEND(false), IS_DEFERRED(true) {}

void Wireframe::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}

void Wireframe::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    ::HeightFieldFluid::VertexData::getInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_VERTEX);
    Storage::Vector4::GetInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_VERTEX);  // Not used
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
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
}

// OCEAN
const Pipeline::CreateInfo HFF_OCEAN_CREATE_INFO = {
    GRAPHICS::HFF_OCEAN_DEFERRED,
    "Height Field Fluid Ocean (Deferred) Pipeline",
    {SHADER::HFF_VERT, SHADER::DEFERRED_MRT_COLOR_FRAG},
    {DESCRIPTOR_SET::HFF_DEF},
};
Ocean::Ocean(Pipeline::Handler& handler) : Graphics(handler, &HFF_OCEAN_CREATE_INFO), DO_BLEND(false), IS_DEFERRED(true) {}

void Ocean::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}

void Ocean::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    ::HeightFieldFluid::VertexData::getInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_VERTEX);
    Storage::Vector4::GetInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_VERTEX);
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

}  // namespace HeightFieldFluid
}  // namespace Pipeline

Texture::CreateInfo MakeHeightFieldFluidTex(stbi_uc* pData, VkExtent3D extent) {
    Sampler::CreateInfo sampInfo = {
        "HFF Sampler",
        {{{::Sampler::USAGE::HEIGHT}}},
        VK_IMAGE_VIEW_TYPE_3D,
        extent,
        {},
        0,
        SAMPLER::DEFAULT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        {{false, false}, 1},
        VK_FORMAT_R32_SFLOAT,
        Sampler::CHANNELS::_1,
        sizeof(float),
    };

    sampInfo.layersInfo.infos.front().pPixel = pData;
    return {std::string(Texture::HFF_ID), {sampInfo}, false, false, STORAGE_IMAGE::DONT_CARE};
}

// BUFFER
namespace HeightFieldFluid {

Buffer::Buffer(Particle::Handler& handler, const Particle::Buffer::index&& offset, const CreateInfo* pCreateInfo,
               std::shared_ptr<Material::Base>& pMaterial,
               const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors,
               std::shared_ptr<::Instance::Obj3d::Base>& pInstanceData)
    : Buffer::Base(handler, std::forward<const Particle::Buffer::index>(offset), pCreateInfo, pMaterial, pDescriptors),
      Obj3d::InstanceDraw(pInstanceData),
      normalOffset_(Particle::Buffer::BAD_OFFSET),
      indexWFRes_{},
      drawMode_(GRAPHICS::HFF_OCEAN_DEFERRED) {
    for (uint32_t i = 0; i < static_cast<uint32_t>(pDescriptors_.size()); i++)
        if (pDescriptors[i]->getDescriptorType() == DESCRIPTOR{STORAGE_BUFFER_DYNAMIC::NORMAL}) normalOffset_ = i;
    assert(normalOffset_ != Particle::Buffer::BAD_OFFSET);

    workgroupSize_.x = pCreateInfo->info.M;
    workgroupSize_.y = pCreateInfo->info.N;
    assert(workgroupSize_.y > 1 && workgroupSize_.x > 1);

    /**
     * Height field image layers
     *  u (water column height 0)
     *  u (water column height 1)
     *  v (velocity)
     *  r (displacement)
     */
    uint32_t numImgLayers = 4, idx;
    size_t numHeights = static_cast<size_t>(workgroupSize_.x) * static_cast<size_t>(workgroupSize_.y);

    float* pData = (float*)calloc(numHeights * static_cast<size_t>(numImgLayers), sizeof(float));
    verticesHFF_.reserve(numHeights);

    float dx, dz, z;
    float left = pCreateInfo->info.lengthM * 0.5f;
    float right = left * -1.0f;
    float top = (pCreateInfo->info.lengthM / static_cast<float>(workgroupSize_.x - 1)) * (workgroupSize_.y - 1) * 0.5f;
    float bottom = top * -1.0f;

    for (uint32_t j = 0; j < workgroupSize_.y; j++) {
        dz = static_cast<float>(j) / static_cast<float>(workgroupSize_.y - 1);
        z = glm::mix(top, bottom, dz);

        for (uint32_t i = 0; i < workgroupSize_.x; i++) {
            dx = static_cast<float>(i) / static_cast<float>(workgroupSize_.x - 1);
            auto x = glm::mix(left, right, dx);

            // VERTEX
            verticesHFF_.push_back({
                // position
                {x, 0.0f, z},
                // image offset
                {static_cast<int>(i), static_cast<int>(j)},
            });

            // HEIGHT
            idx = (i * pCreateInfo->info.M) + j;
            // if (i == 6 || j == 6) {
            //    pData[(i * info.M) + j] = 2.0f;
            //}
            pData[idx] = ((cos(static_cast<float>(0.1f * x)) *  //
                           sin(static_cast<float>(0.3f * z)))   //
                          / 1.0f /* glm::two_pi<float>()*/);
            pData[idx] += (2.0f * cos(static_cast<float>(0.01f * x))) *  //
                          (glm::pi<float>() + (2.0f * sin(static_cast<float>(0.01f * z))));
            pData[idx] += pow(glm::one_over_pi<float>(), 3) * (1.0f - Random::inst().nextFloatZeroToOne());
        }
    }

    // Test to make sure "h" is the same in both directions.
    {
        auto x0 = verticesHFF_[1].position.x - verticesHFF_[0].position.x;
        auto x1 = verticesHFF_[workgroupSize_.x - 1].position.x - verticesHFF_[workgroupSize_.x - 2].position.x;
        assert(glm::epsilonEqual(x0, x1, 0.00001f));
        if (workgroupSize_.y > 1) {
            auto z0 = verticesHFF_[0].position.z - verticesHFF_[workgroupSize_.x].position.z;
            auto z1 =
                verticesHFF_[workgroupSize_.x - 2].position.z -
                verticesHFF_[static_cast<size_t>(workgroupSize_.x - 2) + static_cast<size_t>(workgroupSize_.x)].position.z;
            assert(glm::epsilonEqual(z0, z1, 0.000001f));
        }
    }

    // IMAGE
    auto texInfo = MakeHeightFieldFluidTex((stbi_uc*)pData, {pCreateInfo->info.M, pCreateInfo->info.N, numImgLayers});
    handler.textureHandler().make(&texInfo);

    // INDEX
    {
        size_t numIndices = static_cast<size_t>(static_cast<size_t>(workgroupSize_.x) *
                                                    (2 + ((static_cast<size_t>(workgroupSize_.y) - 2) * 2)) +
                                                static_cast<size_t>(workgroupSize_.y) - 1);
        indices_.resize(numIndices);

        size_t index = 0, indexWF = 0;
        for (uint32_t row = 0; row < workgroupSize_.y - 1; row++) {
            for (uint32_t col = 0; col < workgroupSize_.x; col++) {
                // Triangle strip indices (surface)
                indices_[index + 1] = (row + 1) * workgroupSize_.x + col;
                indices_[index] = row * workgroupSize_.x + col;
                index += 2;
                // Line strip indices (wireframe)
                indicesWF_.push_back(row * workgroupSize_.x + col + workgroupSize_.x);
                indicesWF_.push_back(indicesWF_[indicesWF_.size() - 1] - workgroupSize_.x);
                if (col + 1 < workgroupSize_.x) {
                    indicesWF_.push_back(indicesWF_[indicesWF_.size() - 1] + 1);
                    indicesWF_.push_back(indicesWF_[indicesWF_.size() - 3]);
                }
            }
            indices_[index++] = VB_INDEX_PRIMITIVE_RESTART;
            indicesWF_.push_back(VB_INDEX_PRIMITIVE_RESTART);
        }
    }

    status_ |= STATUS::PENDING_BUFFERS;
    draw_ = true;
    paused_ = false;
}

void Buffer::loadBuffers() {
    const auto& ctx = handler().shell().context();
    pLdgRes_ = handler().loadingHandler().createLoadingResources();

    // VERTEX
    BufferResource stgRes = {};
    helpers::createBuffer(
        ctx.dev, ctx.memProps, ctx.debugMarkersEnabled, pLdgRes_->transferCmd,
        static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
        sizeof(VertexData) * verticesHFF_.size(), NAME + " vertex", stgRes, verticesHFFRes_, verticesHFF_.data());
    pLdgRes_->stgResources.push_back(std::move(stgRes));

    // INDEX (SURFACE)
    assert(indices_.size());
    stgRes = {};
    helpers::createBuffer(
        ctx.dev, ctx.memProps, ctx.debugMarkersEnabled, pLdgRes_->transferCmd,
        static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
        sizeof(VB_INDEX_TYPE) * indices_.size(), NAME + " index (surface)", stgRes, indexRes_, indices_.data());
    pLdgRes_->stgResources.push_back(std::move(stgRes));

    // INDEX (WIREFRAME)
    assert(indicesWF_.size());
    stgRes = {};
    helpers::createBuffer(
        ctx.dev, ctx.memProps, ctx.debugMarkersEnabled, pLdgRes_->transferCmd,
        static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT),
        sizeof(VB_INDEX_TYPE) * indicesWF_.size(), NAME + " index (wireframe)", stgRes, indexWFRes_, indicesWF_.data());
    pLdgRes_->stgResources.push_back(std::move(stgRes));
}

void Buffer::destroy() {
    Base::destroy();
    auto& dev = handler().shell().context().dev;
    if (verticesHFF_.size()) {
        vkDestroyBuffer(dev, verticesHFFRes_.buffer, nullptr);
        vkFreeMemory(dev, verticesHFFRes_.memory, nullptr);
    }
    if (indicesWF_.size()) {
        vkDestroyBuffer(dev, indexWFRes_.buffer, nullptr);
        vkFreeMemory(dev, indexWFRes_.memory, nullptr);
    }
}

void Buffer::draw(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                  const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                  const uint8_t frameIndex) const {
    if (pPipelineBindData->type != PIPELINE{drawMode_}) return;

    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    switch (drawMode_) {
        case GRAPHICS::HFF_CLMN_DEFERRED: {
            vkCmdPushConstants(cmd, pPipelineBindData->layout, pPipelineBindData->pushConstantStages, 0,
                               static_cast<uint32_t>(sizeof(Pipeline::HeightFieldFluid::Column::PushConstant)),
                               &pInstObj3d_->model());
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);
            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());
            const VkBuffer buffers[] = {vertexRes_.buffer};
            const VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(cmd, 0, 1, buffers, offsets);
            vkCmdDraw(cmd, 36, workgroupSize_.x * workgroupSize_.y, 0, 0);
        } break;
        case GRAPHICS::HFF_WF_DEFERRED: {
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);
            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());
            const VkBuffer buffers[] = {
                verticesHFFRes_.buffer,
                pDescriptors_[normalOffset_]->BUFFER_INFO.bufferInfo.buffer,  // Not used
                pInstObj3d_->BUFFER_INFO.bufferInfo.buffer,

            };
            const VkDeviceSize offsets[] = {
                0,
                pDescriptors_[normalOffset_]->BUFFER_INFO.memoryOffset,  // Not used
                pInstObj3d_->BUFFER_INFO.memoryOffset,
            };
            vkCmdBindVertexBuffers(cmd, 0, 3, buffers, offsets);
            vkCmdBindIndexBuffer(cmd, indexWFRes_.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(                              //
                cmd,                                       // VkCommandBuffer commandBuffer
                static_cast<uint32_t>(indicesWF_.size()),  // uint32_t indexCount
                pInstObj3d_->BUFFER_INFO.count,            // uint32_t instanceCount
                0,                                         // uint32_t firstIndex
                0,                                         // int32_t vertexOffset
                0                                          // uint32_t firstInstance
            );
        } break;
        case GRAPHICS::HFF_OCEAN_DEFERRED: {
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);
            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());
            const VkBuffer buffers[] = {
                verticesHFFRes_.buffer,
                pDescriptors_[normalOffset_]->BUFFER_INFO.bufferInfo.buffer,
                pInstObj3d_->BUFFER_INFO.bufferInfo.buffer,
            };
            const VkDeviceSize offsets[] = {
                0,
                pDescriptors_[normalOffset_]->BUFFER_INFO.memoryOffset,
                pInstObj3d_->BUFFER_INFO.memoryOffset,
            };
            vkCmdBindVertexBuffers(cmd, 0, 3, buffers, offsets);
            vkCmdBindIndexBuffer(cmd, indexRes_.buffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(                            //
                cmd,                                     // VkCommandBuffer commandBuffer
                static_cast<uint32_t>(indices_.size()),  // uint32_t indexCount
                pInstObj3d_->BUFFER_INFO.count,          // uint32_t instanceCount
                0,                                       // uint32_t firstIndex
                0,                                       // int32_t vertexOffset
                0                                        // uint32_t firstInstance
            );
        } break;
        default: {
            assert(false);
        } break;
    }
}

void Buffer::dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                      const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                      const uint8_t frameIndex) const {
    assert(LOCAL_SIZE.z == 1 && workgroupSize_.z == 1);
    assert(workgroupSize_.x % LOCAL_SIZE.x == 0 && workgroupSize_.y % LOCAL_SIZE.y == 0);

    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    switch (std::visit(Pipeline::GetCompute{}, pPipelineBindData->type)) {
        case COMPUTE::HFF_HGHT: {
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());

            vkCmdDispatch(cmd, workgroupSize_.x / LOCAL_SIZE.x, workgroupSize_.y / LOCAL_SIZE.y, 1);

            VkMemoryBarrier memoryBarrier = {
                VK_STRUCTURE_TYPE_MEMORY_BARRIER, nullptr,
                VK_ACCESS_SHADER_WRITE_BIT,  // srcAccessMask
                VK_ACCESS_SHADER_READ_BIT,   // dstAccessMask
            };

            vkCmdPipelineBarrier(cmd,                                   //
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // srcStageMask
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,  // dstStageMask
                                 0,                                     // dependencyFlags
                                 1,                                     // memoryBarrierCount
                                 &memoryBarrier,                        // pMemoryBarriers
                                 0, nullptr, 0, nullptr);
        } break;
        case COMPUTE::HFF_NORM: {
            vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

            vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                                    static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                                    descSetBindData.descriptorSets[setIndex].data(),
                                    static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                                    descSetBindData.dynamicOffsets.data());

            // vkCmdDispatch(cmd, workgroupSize_.x / 10, workgroupSize_.y / 10, 1);
            vkCmdDispatch(cmd, workgroupSize_.x / LOCAL_SIZE.x, workgroupSize_.y / LOCAL_SIZE.y, 1);
        } break;
        default: {
            assert(false);
        } break;
    }
}

}  // namespace HeightFieldFluid
