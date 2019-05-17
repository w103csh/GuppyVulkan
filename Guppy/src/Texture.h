
#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Descriptor.h"
#include "Helpers.h"
#include "LoadingHandler.h"
#include "Sampler.h"

namespace Texture {

constexpr float EMPTY_ASPECT = FLT_MAX;

typedef enum TYPE {
    COLOR = 0x00000001,  // DIFFUSE
    // THROUGH 0x00000008
    NORMAL = 0x00000010,
    // THROUGH 0x00000080
    SPECULAR = 0x00000100,
    // THROUGH 0x00000800
    ALPHA = 0x00001000,
    // THROUGH 0x00000800
    HEIGHT = 0x00010000,
    // THROUGH 0x00000800
} TYPE;

struct CreateInfo {
    // COLOR (diffuse)
    std::string colorPath = "";
    Sampler::CHANNELS colorChannels = Sampler::CHANNELS::_4;
    // NORMAL
    std::string normalPath = "";
    Sampler::CHANNELS normalChannels = Sampler::CHANNELS::_4;
    // SPECULAR
    std::string specularPath = "";
    Sampler::CHANNELS specularChannels = Sampler::CHANNELS::_4;
    // ALPHA
    std::string alphaPath = "";
    Sampler::CHANNELS alphaChannels = Sampler::CHANNELS::_1;
    // HEIGHT
    std::string heightPath = "";
    Sampler::CHANNELS heightChannels = Sampler::CHANNELS::_1;
    // GENERAL
    bool makeMipmaps = true;
};

class Base : public Descriptor::Interface {
   public:
    Base(const uint32_t &&offset, CreateInfo *pCreateInfo);

    // DESCRIPTOR
    bool hasSampler(const uint32_t &binding) const;
    void setWriteInfo(VkWriteDescriptorSet &write) const override;

    const std::string NAME;
    const uint32_t OFFSET;
    STATUS status;
    bool makeMipmaps;
    FlagBits flags;
    float aspect;
    std::unique_ptr<Loading::Resources> pLdgRes;
    std::vector<VkDescriptorImageInfo> imageInfos;

    // TODO: move to sampler
    std::string colorPath, normalPath, specularPath, alphaPath, heightPath;
    Sampler::CHANNELS colorChannels, normalChannels, specularChannels, alphaChannels, heightChannels;
    // SAMPLERS
    Sampler::Base sampCh1;
    Sampler::Base sampCh2;
    Sampler::Base sampCh3;
    Sampler::Base sampCh4;
};

}  // namespace Texture

#endif  // TEXTURE_H