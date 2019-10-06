
#ifndef TEXTURE_H
#define TEXTURE_H

#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Sampler.h"

// clang-format off
namespace Loading { struct Resources; }
// clang-format on

namespace Texture {

class Base {
   public:
    Base(const uint32_t &&offset, const CreateInfo *pCreateInfo);
    virtual ~Base() = default;

    void destroy(const VkDevice &dev);

    const DESCRIPTOR DESCRIPTOR_TYPE;  // TODO: this should probably be detemined by the pipeline/set
    const bool HAS_DATA;
    const std::string NAME;
    const uint32_t OFFSET;
    const bool PER_FRAMEBUFFER;

    STATUS status;
    FlagBits flags;  // TODO: Still needed?
    bool usesSwapchain;
    float aspect;  // TODO: Set per sampler passing a dynamic list to shaders?
    std::unique_ptr<LoadingResource> pLdgRes;

    std::vector<Sampler::Base> samplers;
};

}  // namespace Texture

#endif  // TEXTURE_H