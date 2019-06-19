
#include "DescriptorConstants.h"

#include "Constants.h"

namespace Descriptor {

const std::set<DESCRIPTOR> DESCRIPTORS = {
    // UNIFORM
    UNIFORM::CAMERA_PERSPECTIVE_DEFAULT,
    UNIFORM::LIGHT_POSITIONAL_DEFAULT,
    UNIFORM::LIGHT_SPOT_DEFAULT,
    UNIFORM::LIGHT_POSITIONAL_PBR,
    UNIFORM::FOG_DEFAULT,
    UNIFORM::PROJECTOR_DEFAULT,
    // UNIFORM DYNAMIC
    UNIFORM_DYNAMIC::MATERIAL_DEFAULT,
    UNIFORM_DYNAMIC::MATERIAL_PBR,
    // COMBINED SAMPLER
    COMBINED_SAMPLER::MATERIAL,
    COMBINED_SAMPLER::PIPELINE,
};  // namespace Descriptor

void OffsetsMap::insert(const DESCRIPTOR& key, const Uniform::offsets& value) {
    if (std::visit(IsCombinedSampler{}, key)) {
        auto x = offsets_.count(key);
        offsets_.insert({key, value});
    } else {
        // Samplers are the only type allowed to have multiple elements, so
        // remove the existing element for the key if one exists.
        auto it = offsets_.find(key);
        if (it != offsets_.end()) offsets_.erase(it);
        offsets_.insert({key, value});
        assert(offsets_.count(key) == 1);
    }
}

namespace Set {

const std::set<DESCRIPTOR_SET> ALL = {
    // DEFAULT
    DESCRIPTOR_SET::UNIFORM_DEFAULT,
    DESCRIPTOR_SET::SAMPLER_DEFAULT,
    DESCRIPTOR_SET::SAMPLER_CUBE_DEFAULT,
    DESCRIPTOR_SET::PROJECTOR_DEFAULT,
    // PBR
    DESCRIPTOR_SET::UNIFORM_PBR,
    // PARALLAX
    DESCRIPTOR_SET::UNIFORM_PARALLAX,
    DESCRIPTOR_SET::SAMPLER_PARALLAX,
};

}  // namespace Set

}  // namespace Descriptor