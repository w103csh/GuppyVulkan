
#include "Texture.h"

Texture::Base::Base(const uint32_t&& offset, const CreateInfo* pCreateInfo)
    : NAME(pCreateInfo->name),  //
      OFFSET(offset),
      status(STATUS::PENDING),
      flags(0),
      aspect(0.0f),
      pLdgRes(nullptr) {}

void Texture::Base::setWriteInfo(VkWriteDescriptorSet& write) const {
    assert(write.dstBinding < samplers.size());
    write.descriptorCount = 1;
    write.pImageInfo = &samplers[write.dstBinding].imgDescInfo;
}
