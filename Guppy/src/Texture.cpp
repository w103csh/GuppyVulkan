
#include "Texture.h"

Texture::Base::Base(const uint32_t&& offset, const CreateInfo* pCreateInfo)
    : NAME(pCreateInfo->name),  //
      OFFSET(offset),
      status(STATUS::PENDING),
      flags(0),
      aspect(0.0f),
      pLdgRes(nullptr) {}

void Texture::Base::setWriteInfo(VkWriteDescriptorSet& write, uint32_t index) const {
    assert(status == STATUS::READY);
    write.descriptorCount = 1;
    write.pImageInfo = &samplers[index].imgDescInfo;
}
