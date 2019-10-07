
#include "Storage.h"

#include <glm/gtc/constants.hpp>

namespace {

inline float gauss(float x, float sigma2) {
    double coeff = 1.0 / (glm::two_pi<double>() * sigma2);
    double expon = -(x * x) / (2.0 * sigma2);
    return (float)(coeff * exp(expon));
}

}  // namespace

Storage::PostProcess::DATA::DATA()
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

Storage::PostProcess::Base::Base(const Buffer::Info&& info, DATA* pData,
                                 const Buffer::CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::PerFramebufferDataItem<DATA>(pData) {
    dirty = true;
}
