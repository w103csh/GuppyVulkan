
#include "Light.h"

// POSITIONAL

Light::Default::Positional::Base::Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Light::Base<DATA>(pData, pCreateInfo),
      position(getWorldSpacePosition()) {}

void Light::Default::Positional::Base::update(glm::vec3 &&position, const uint32_t frameIndex) {
    data_.position = position;
    setData(frameIndex);
}

// SPOT

Light::Default::Spot::Base::Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo)  //
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Light::Base<DATA>(pData, pCreateInfo),
      direction(getWorldSpaceDirection()),
      position(getWorldSpacePosition()) {
    data_.cutoff = pCreateInfo->cutoff;
    data_.exponent = pCreateInfo->exponent;
}

void Light::Default::Spot::Base::update(glm::vec3 &&direction, glm::vec3 &&position, const uint32_t frameIndex) {
    data_.direction = direction;
    data_.position = position;
    setData(frameIndex);
}