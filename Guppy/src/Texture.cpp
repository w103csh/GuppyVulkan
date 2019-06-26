
#include "Texture.h"

Texture::Base::Base(const uint32_t&& offset, const CreateInfo* pCreateInfo)
    : HAS_DATA(pCreateInfo->hasData),
      NAME(pCreateInfo->name.data(), pCreateInfo->name.size()),
      OFFSET(offset),
      status(STATUS::PENDING),
      flags(0),
      aspect(0.0f),
      pLdgRes(nullptr) {}

void Texture::Base::destroy(const VkDevice& dev) {
    for (auto& sampler : samplers) {
        sampler.destroy(dev);
    }
    status = STATUS::DESTROYED;
}

void Texture::Base::setWriteInfo(VkWriteDescriptorSet& write, uint32_t index) const {
    assert(status == STATUS::READY);
    write.descriptorCount = 1;
    write.pImageInfo = &samplers[index].imgDescInfo;
}
