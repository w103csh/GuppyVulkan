/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Light.h"

// DIRECTIONAL

Light::Default::Directional::Base::Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::PerFramebufferDataItem<DATA>(pData),
      Descriptor::Base(UNIFORM::LIGHT_DIRECTIONAL_DEFAULT),
      direction(pCreateInfo->direction) {}

void Light::Default::Directional::Base::update(const glm::vec3 direction, const uint32_t frameIndex) {
    data_.direction = direction;
    setData(frameIndex);
}

// POSITIONAL

Light::Default::Positional::Base::Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Light::Base<DATA>(UNIFORM::LIGHT_POSITIONAL_DEFAULT, pData, pCreateInfo),
      position_(getWorldSpacePosition()) {}

void Light::Default::Positional::Base::update(glm::vec3 &&position, const uint32_t frameIndex) {
    data_.position = position;
    setData(frameIndex);
}

// SPOT

Light::Default::Spot::Base::Base(const Buffer::Info &&info, DATA *pData, const CreateInfo *pCreateInfo)  //
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Light::Base<DATA>(UNIFORM::LIGHT_SPOT_DEFAULT, pData, pCreateInfo),
      direction_(getWorldSpaceDirection()),
      position_(getWorldSpacePosition()) {
    data_.cutoff = pCreateInfo->cutoff;
    data_.exponent = pCreateInfo->exponent;
}

void Light::Default::Spot::Base::update(glm::vec3 &&direction, glm::vec3 &&position, const uint32_t frameIndex) {
    data_.direction = direction;
    data_.position = position;
    setData(frameIndex);
}