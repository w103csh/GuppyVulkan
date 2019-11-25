
#include "HeightFieldFluid.h"

#include "Deferred.h"
#include "Random.h"
// HANDLERS
#include "LoadingHandler.h"  // Remove me!!!
#include "ParticleHandler.h"
#include "PipelineHandler.h"
#include "TextureHandler.h"

// SHADER
namespace Shader {
const CreateInfo HFF_COMP_CREATE_INFO = {
    SHADER::HFF_COMP,
    "Height Field Fluid Compute Shader",
    "comp.hff.glsl",
    VK_SHADER_STAGE_COMPUTE_BIT,
};
const CreateInfo HFF_CLMN_VERT_CREATE_INFO = {
    SHADER::HFF_CLMN_VERT,
    "Height Field Fluid Vertex Shader",
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
        {{3, 0}, {UNIFORM_DYNAMIC::HFF}},
        {{4, 0}, {STORAGE_IMAGE::PIPELINE, Texture::HFF_ID}},
    },
};
const CreateInfo HFF_CLMN_CREATE_INFO = {
    DESCRIPTOR_SET::HFF_CLMN,
    "_DS_HFF_CLMN",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_DEFAULT}},
        {{2, 0}, {UNIFORM_DYNAMIC::MATRIX_4}},
        {{3, 0}, {UNIFORM_DYNAMIC::HFF}},
        {{4, 0}, {STORAGE_IMAGE::PIPELINE, Texture::HFF_ID}},
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

// HEIGHT FIELD FLUID (COMPUTE)
const Pipeline::CreateInfo HFF_COMP_CREATE_INFO = {
    COMPUTE::HFF,
    "Height Fluid Field Compute Pipeline",
    {SHADER::HFF_COMP},
    {DESCRIPTOR_SET::HFF},
};
HeightFieldFluidCompute::HeightFieldFluidCompute(Pipeline::Handler& handler) : Compute(handler, &HFF_COMP_CREATE_INFO) {}

// HEIGHT FIELD FLUID (COLUMN)
const Pipeline::CreateInfo HFF_CLMN_CREATE_INFO = {
    GRAPHICS::HFF_CLMN_DEFERRED,
    "Height Field Fluid Columns (Deferred) Pipeline",
    {SHADER::HFF_CLMN_VERT, SHADER::DEFERRED_MRT_COLOR_FRAG},
    {DESCRIPTOR_SET::HFF_CLMN},
};
HeightFieldFluidColumn::HeightFieldFluidColumn(Pipeline::Handler& handler)
    : Graphics(handler, &HFF_CLMN_CREATE_INFO), DO_BLEND(false), IS_DEFERRED(true) {}

void HeightFieldFluidColumn::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}

void HeightFieldFluidColumn::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    {
        const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
        createInfoRes.bindDescs.push_back({});
        createInfoRes.bindDescs.back().binding = BINDING;
        createInfoRes.bindDescs.back().stride = sizeof(InstanceData);
        createInfoRes.bindDescs.back().inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

        // position
        createInfoRes.attrDescs.push_back({});
        createInfoRes.attrDescs.back().binding = BINDING;
        createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
        createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32_SFLOAT;  // vec3
        createInfoRes.attrDescs.back().offset = offsetof(InstanceData, position);
        // imageOffset
        createInfoRes.attrDescs.push_back({});
        createInfoRes.attrDescs.back().binding = BINDING;
        createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
        createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32_SINT;  // ivec2
        createInfoRes.attrDescs.back().offset = offsetof(InstanceData, imageOffset);
    }

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

}  // namespace Pipeline

Texture::CreateInfo MakeHeightFieldFluidTex(const HeightFieldFluid::Info info) {
    /**
     * u (water column height 0)
     * u (water column height 1)
     * v (velocity)
     * r (displacement)
     */
    uint32_t numLayers = 6;

    float* pData = (float*)calloc(static_cast<size_t>(info.M) * static_cast<size_t>(info.N) * static_cast<size_t>(numLayers),
                                  sizeof(float));

    for (uint32_t i = 0; i < info.N; i++) {
        for (uint32_t j = 0; j < info.M; j++) {
            // pData[(i * info.M) + j] = Random::inst().nextFloatZeroToOne();
            if (i == 6 || j == 6) {
                pData[(i * info.M) + j] = 2.0f;
            } else if (i == 5 || j == 5) {
                pData[(i * info.M) + j] = 1.66f;
            } else if (i == 7 || j == 7) {
                pData[(i * info.M) + j] = 1.66f;
            } else if (i == 4 || j == 4) {
                pData[(i * info.M) + j] = 1.33f;
            } else if (i == 8 || j == 8) {
                pData[(i * info.M) + j] = 1.33f;
            } else {
                pData[(i * info.M) + j] = 1.0f;
            }
        }
    }

    Sampler::CreateInfo sampInfo = {
        "HFF Sampler",
        {{{::Sampler::USAGE::HEIGHT}}},
        VK_IMAGE_VIEW_TYPE_3D,
        {info.M, info.N, numLayers},
        {},
        0,
        SAMPLER::DEFAULT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        {{false, false}, 1},
        VK_FORMAT_R32_SFLOAT,
        Sampler::CHANNELS::_1,
        sizeof(float),
    };

    sampInfo.layersInfo.infos.front().pPixel = (stbi_uc*)pData;

    return {std::string(Texture::HFF_ID), {sampInfo}, false, false, STORAGE_IMAGE::DONT_CARE};
}

// BUFFER
namespace Particle {
namespace Buffer {
namespace HeightFieldFluid {

Base::Base(Particle::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo,
           std::shared_ptr<Material::Base>& pMaterial, const std::vector<std::shared_ptr<Descriptor::Base>>& pDescriptors)
    : Buffer::Base(handler, std::forward<const index>(offset), pCreateInfo, pMaterial, pDescriptors) {
    workgroupSize_.x = pCreateInfo->info.M;
    workgroupSize_.y = pCreateInfo->info.N;
    assert(workgroupSize_.y > 1 && workgroupSize_.x > 1);

    auto texInfo = MakeHeightFieldFluidTex(pCreateInfo->info);
    handler.textureHandler().make(&texInfo);

    // Column data
    {
        columnData_.reserve(static_cast<size_t>(workgroupSize_.x) * static_cast<size_t>(workgroupSize_.y));

        float dx, dz, z;
        float right = pCreateInfo->info.lengthM * 0.5f;
        float left = right * -1.0f;
        float top = (pCreateInfo->info.lengthM / static_cast<float>(workgroupSize_.x - 1)) * (workgroupSize_.y - 1) * 0.5f;
        float bottom = top * -1.0f;

        for (uint32_t j = 0; j < workgroupSize_.y; j++) {
            dz = static_cast<float>(j) / static_cast<float>(workgroupSize_.y - 1);
            z = glm::mix(top, bottom, dz);

            for (uint32_t i = 0; i < workgroupSize_.x; i++) {
                dx = static_cast<float>(i) / static_cast<float>(workgroupSize_.x - 1);

                columnData_.push_back({
                    // position
                    {
                        glm::mix(left, right, dx),  // x
                        0.0f,                       // y
                        z,                          // z
                    },
                    // image offset
                    {static_cast<int>(i), static_cast<int>(j)},
                });
            }
        }

        // Test to make sure "h" is the same in both directions.
        auto x0 = columnData_[1].position.x - columnData_[0].position.x;
        auto x1 = columnData_[workgroupSize_.x - 1].position.x - columnData_[workgroupSize_.x - 2].position.x;
        assert(glm::epsilonEqual(x0, x1, 0.000001f));
        if (workgroupSize_.y > 1) {
            auto z0 = columnData_[0].position.z - columnData_[workgroupSize_.x].position.z;
            auto z1 =
                columnData_[workgroupSize_.x - 2].position.z -
                columnData_[static_cast<size_t>(workgroupSize_.x - 2) + static_cast<size_t>(workgroupSize_.x)].position.z;
            assert(glm::epsilonEqual(z0, z1, 0.000001f));
        }

        const auto& ctx = handler.shell().context();
        pLdgRes2_ = handler.loadingHandler().createLoadingResources();
        BufferResource stgRes = {};
        helpers::createBuffer(
            ctx.dev, ctx.memProps, ctx.debugMarkersEnabled, pLdgRes2_->transferCmd,
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT),
            sizeof(Pipeline::HeightFieldFluidColumn::InstanceData) * columnData_.size(), NAME + " vertex", stgRes,
            columnRes_, columnData_.data());
        pLdgRes2_->stgResources.push_back(std::move(stgRes));

        // Submit vertex loading commands...
        handler.loadingHandler().loadSubmit(std::move(pLdgRes2_));
    }

    paused_ = false;
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
        columnRes_.buffer,
    };
    const VkDeviceSize offsets[] = {
        0,
    };

    vkCmdBindVertexBuffers(  //
        cmd,                 // VkCommandBuffer commandBuffer
        0,                   // uint32_t firstBinding
        1,                   // uint32_t bindingCount
        buffers,             // const VkBuffer* pBuffers
        offsets              // const VkDeviceSize* pOffsets
    );

    vkCmdDraw(                                //
        cmd,                                  // VkCommandBuffer commandBuffer
        36,                                   // uint32_t vertexCount
        workgroupSize_.x * workgroupSize_.y,  // uint32_t instanceCount
        0,                                    // uint32_t firstVertex
        0                                     // uint32_t firstInstance
    );
}

void Base::dispatch(const PASS& passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                    const Descriptor::Set::BindData& descSetBindData, const VkCommandBuffer& cmd,
                    const uint8_t frameIndex) const {
    assert(LOCAL_SIZE.z == 1 && workgroupSize_.z == 1);
    assert(workgroupSize_.x % LOCAL_SIZE.x == 0 && workgroupSize_.y % LOCAL_SIZE.y == 0);

    auto setIndex = (std::min)(static_cast<uint8_t>(descSetBindData.descriptorSets.size() - 1), frameIndex);

    vkCmdBindPipeline(cmd, pPipelineBindData->bindPoint, pPipelineBindData->pipeline);

    vkCmdBindDescriptorSets(cmd, pPipelineBindData->bindPoint, pPipelineBindData->layout, descSetBindData.firstSet,
                            static_cast<uint32_t>(descSetBindData.descriptorSets[setIndex].size()),
                            descSetBindData.descriptorSets[setIndex].data(),
                            static_cast<uint32_t>(descSetBindData.dynamicOffsets.size()),
                            descSetBindData.dynamicOffsets.data());

    vkCmdDispatch(cmd, workgroupSize_.x / LOCAL_SIZE.x, workgroupSize_.y / LOCAL_SIZE.y, 1);
}

}  // namespace HeightFieldFluid
}  // namespace Buffer
}  // namespace Particle
