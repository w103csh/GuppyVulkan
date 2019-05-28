
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

struct CreateInfo {
    std::string name;
    std::vector<Sampler::CreateInfo> samplerCreateInfos;
};

class Base : public Descriptor::Interface {
   public:
    Base(const uint32_t &&offset, const CreateInfo *pCreateInfo);

    // DESCRIPTOR
    void setWriteInfo(VkWriteDescriptorSet &write) const override;

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