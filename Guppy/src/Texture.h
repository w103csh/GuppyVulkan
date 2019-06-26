
#ifndef TEXTURE_H
#define TEXTURE_H

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Descriptor.h"
#include "Sampler.h"
// HANDLERS
#include "LoadingHandler.h"

namespace Texture {

class Base : public Descriptor::Interface {
   public:
    Base(const uint32_t &&offset, const CreateInfo *pCreateInfo);
    virtual ~Base() = default;

    void destroy(const VkDevice &dev);

    // DESCRIPTOR
    void setWriteInfo(VkWriteDescriptorSet &write, uint32_t index = 0) const override;

    const bool HAS_DATA;
    const std::string NAME;
    const uint32_t OFFSET;

    STATUS status;
    FlagBits flags;  // TODO: Still needed?
    float aspect;    // TODO: Set per sampler passing a dynamic list to shaders?
    std::unique_ptr<Loading::Resources> pLdgRes;

    std::vector<Sampler::Base> samplers;
};

}  // namespace Texture

#endif  // TEXTURE_H