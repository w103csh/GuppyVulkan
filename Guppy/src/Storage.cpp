
#include "Storage.h"

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

void GetInputDescriptions(Pipeline::CreateInfoResources& createInfoRes, const VkVertexInputRate&& inputRate) {
    const auto BINDING = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.bindDescs.push_back({});
    createInfoRes.bindDescs.back().binding = BINDING;
    createInfoRes.bindDescs.back().stride = sizeof(DATA);
    createInfoRes.bindDescs.back().inputRate = inputRate;

    createInfoRes.attrDescs.push_back({});
    createInfoRes.attrDescs.back().binding = BINDING;
    createInfoRes.attrDescs.back().location = static_cast<uint32_t>(createInfoRes.attrDescs.size() - 1);
    createInfoRes.attrDescs.back().format = VK_FORMAT_R32G32B32A32_SFLOAT;  // vec4
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
            Particle::Attractor::SetDataPosition(this, pData_, pCreateInfo);
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
      Buffer::PerFramebufferDataItem<DATA>(pData),
      Descriptor::Base(STORAGE_BUFFER::POST_PROCESS) {
    dirty = true;
}

}  // namespace Storage