
#include "Sampler.h"

#include <stb_image.h>

Sampler::Base::Base(const std::string& name, CreateInfo createInfo)
    : BINDING(createInfo.binding),
      FORMAT(createInfo.format),
      NAME(name),
      NUM_CHANNELS(createInfo.channels),
      width(0),
      height(0),
      mipLevels(0),
      image(VK_NULL_HANDLE),
      memory(VK_NULL_HANDLE),
      view(VK_NULL_HANDLE),
      sampler(VK_NULL_HANDLE),
      imgDescInfo{} {}

void Sampler::Base::copyData(void*& pData, size_t& offset) const {
    auto size = layerSize();
    for (auto& p : pPixels) {
        std::memcpy(static_cast<char*>(pData) + offset, p, size);
        offset += size;
        stbi_image_free(p);
    }
}

void Sampler::Base::destory(const VkDevice& dev) {
    vkDestroySampler(dev, sampler, nullptr);
    vkDestroyImageView(dev, view, nullptr);
    vkDestroyImage(dev, image, nullptr);
    vkFreeMemory(dev, memory, nullptr);
}
