
#include "Light.h"

// **********************
//      Positional
// **********************

Light::Default::Positional::Base::Base(const Buffer::Info &&info, DATA *pData, CreateInfo *pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Light::Base<DATA>(pData, pCreateInfo),
      position(getWorldSpacePosition()) {}

void Light::Default::Positional::Base::update(glm::vec3 &&position) {
    pData_->position = position;
    DIRTY = true;
}

// **********************
//      Spot
// **********************

Light::Default::Spot::Base::Base(const Buffer::Info &&info, DATA *pData, CreateInfo *pCreateInfo)  //
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Light::Base<DATA>(pData, pCreateInfo),
      direction(getWorldSpaceDirection()),
      position(getWorldSpacePosition()) {
    pData_->cutoff = pCreateInfo->cutoff;
    pData_->exponent = pCreateInfo->exponent;
}

void Light::Default::Spot::Base::update(glm::vec3 &&direction, glm::vec3 &&position) {
    pData_->direction = direction;
    pData_->position = position;
    DIRTY = true;
}