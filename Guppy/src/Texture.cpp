/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Texture.h"

// HANDLERS
#include "LoadingHandler.h"

Texture::Base::Base(const uint32_t&& offset, const CreateInfo* pCreateInfo)
    : DESCRIPTOR_TYPE(pCreateInfo->descriptorType),
      HAS_DATA(pCreateInfo->needsData),
      NAME(pCreateInfo->name.data(), pCreateInfo->name.size()),
      OFFSET(offset),
      PER_FRAMEBUFFER(pCreateInfo->perFramebuffer),
      status(STATUS::PENDING),
      flags(0),
      usesSwapchain(false),
      aspect(BAD_ASPECT),
      pLdgRes(nullptr) {}

void Texture::Base::destroy(const vk::Device& dev) {
    for (auto& sampler : samplers) {
        sampler.destroy(dev);
    }
    status = STATUS::DESTROYED;
}