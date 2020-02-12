/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Storage.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>

namespace {

inline float gauss(float x, float sigma2) {
    double coeff = 1.0 / (glm::two_pi<double>() * sigma2);
    double expon = -(x * x) / (2.0 * sigma2);
    return (float)(coeff * exp(expon));
}

}  // namespace

namespace Storage {

// VECTOR4
namespace Vector4 {

void GetInputDescriptions(Pipeline::CreateInfoResources& createInfoRes, const vk::VertexInputRate&& inputRate) {
    const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.bindDescs.push_back({});
    createInfoRes.bindDescs.back().binding = BINDING;
    createInfoRes.bindDescs.back().stride = sizeof(DATA);
    createInfoRes.bindDescs.back().inputRate = inputRate;

    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = vk::Format::eR32G32B32A32Sfloat;  // vec4
    createInfoRes.attrDescs.back().offset = 0;
}

Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::DataItem<DATA>(pData),
      Descriptor::Base(pCreateInfo->descType),
      VECTOR_TYPE(pCreateInfo->type) {
    assert(std::visit(Descriptor::GetStorageBufferDynamic{}, getDescriptorType()) != STORAGE_BUFFER_DYNAMIC::DONT_CARE);
    switch (VECTOR_TYPE) {
        case TYPE::ATTRACTOR_POSITION: {
            assert(BUFFER_INFO.count == pCreateInfo->localSize.x * pCreateInfo->localSize.y * pCreateInfo->localSize.z);
            // Initial positions of the particles
            glm::vec4 p{0.0f, 0.0f, 0.0f, 1.0f};
            float dx = 2.0f / (pCreateInfo->localSize.x - 1), dy = 2.0f / (pCreateInfo->localSize.y - 1),
                  dz = 2.0f / (pCreateInfo->localSize.z - 1);
            // We want to center the particles at (0,0,0)
            glm::mat4 transf = glm::translate(glm::mat4(1.0f), glm::vec3(-1, -1, -1));
            uint32_t idx = 0;
            for (uint32_t i = 0; i < pCreateInfo->localSize.x; i++) {
                for (uint32_t j = 0; j < pCreateInfo->localSize.y; j++) {
                    for (uint32_t k = 0; k < pCreateInfo->localSize.z; k++) {
                        pData[idx].x = dx * i;
                        pData[idx].y = dy * j;
                        pData[idx].z = dz * k;
                        pData[idx].w = 1.0f;
                        pData[idx] = transf * pData[idx];
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

// POST-PROCESS

PostProcess::DATA::DATA()
    : uData0{0u},  //
      weights{},
      edgeThreshold(0.5f),
      logAverageLuminance(0.0f) {
    float sum, sigma2 = 8.0f;

    // Compute and sum the weights
    weights[0] = gauss(0, sigma2);
    sum = weights[0];
    for (int i = 1; i < 5; i++) {
        weights[i] = gauss(float(i), sigma2);
        sum += 2 * weights[i];
    }

    // Normalize the weights and set the uniform
    for (int i = 0; i < 5; i++) {
        weights[i] /= sum;
    }
}

PostProcess::Base::Base(const Buffer::Info&& info, DATA* pData,
                        const Buffer::CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Descriptor::Base(STORAGE_BUFFER::POST_PROCESS),
      Buffer::PerFramebufferDataItem<DATA>(pData) {
    dirty = true;
}

}  // namespace Storage
