
#include "DescriptorSet.h"

Descriptor::Set::Base::Base(const DESCRIPTOR_SET&& type, const Descriptor::bindingMap&& bindingMap)
    : TYPE(type), BINDING_MAP(bindingMap), layout(VK_NULL_HANDLE), stages(0) {}

Descriptor::Set::Resource& Descriptor::Set::Base::getResource(const uint32_t& offset) {
    for (auto& resource : resources_)
        if (resource.offset == offset) return resource;
    // Add a resource if one for the offset doesn't exist.
    resources_.push_back({offset, {}});
    return resources_.back();
}

Descriptor::Set::Default::Uniform::Uniform()
    : Set::Base{DESCRIPTOR_SET::UNIFORM_DEFAULT,  //
                {
                    {{0, 0, 0}, {DESCRIPTOR::CAMERA_PERSPECTIVE_DEFAULT, {OFFSET_ALL}}},
                    {{0, 1, 0}, {DESCRIPTOR::MATERIAL_DEFAULT, {0}}},
                    {{0, 2, 0}, {DESCRIPTOR::FOG_DEFAULT, {0}}},
                    {{0, 3, 0}, {DESCRIPTOR::LIGHT_POSITIONAL_DEFAULT, {OFFSET_ALL}}},
                    {{0, 4, 0}, {DESCRIPTOR::LIGHT_SPOT_DEFAULT, {OFFSET_ALL}}}  //
                }} {}

Descriptor::Set::Default::Sampler::Sampler()
    : Set::Base{DESCRIPTOR_SET::SAMPLER_DEFAULT,  //
                {
                    {{1, 0, 0}, {DESCRIPTOR::SAMPLER_4CH_DEFAULT, {0}}},
                    {{1, 1, 0}, {DESCRIPTOR::SAMPLER_1CH_DEFAULT, {0}}},
                    {{1, 2, 0}, {DESCRIPTOR::SAMPLER_2CH_DEFAULT, {0}}},
                    {{1, 3, 0}, {DESCRIPTOR::SAMPLER_3CH_DEFAULT, {0}}}  //
                }} {}
