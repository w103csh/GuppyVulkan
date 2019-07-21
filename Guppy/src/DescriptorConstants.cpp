
#include "DescriptorConstants.h"

#include "ConstantsAll.h"

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
    // MISCELLANEOUS
    // DESCRIPTOR_SET::SWAPCHAIN_IMAGE,
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
    // SCREEN SPACE
    DESCRIPTOR_SET::UNIFORM_SCREEN_SPACE_DEFAULT,
    DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_DEFAULT,
    DESCRIPTOR_SET::STORAGE_SCREEN_SPACE_COMPUTE_POST_PROCESS,
    DESCRIPTOR_SET::STORAGE_IMAGE_SCREEN_SPACE_COMPUTE_DEFAULT,
};

void ResourceInfo::setWriteInfo(const uint32_t index, VkWriteDescriptorSet& write) const {
    auto offset = (std::min)(index, uniqueDataSets - 1);

    if (imageInfos.size()) {
        // IMAGE
        assert(bufferInfos.empty() && texelBufferView == VK_NULL_HANDLE);

        assert(descCount == 1);  // I haven't thought about this yet.

        write.descriptorCount = descCount;
        write.pImageInfo = &imageInfos.at(offset).descInfo;

    } else if (bufferInfos.size()) {
        // BUFFER
        assert(imageInfos.empty() && texelBufferView == VK_NULL_HANDLE);

        offset *= descCount;
        assert((offset + descCount) <= bufferInfos.size());

        write.descriptorCount = descCount;
        write.pBufferInfo = &bufferInfos.at(offset);

    } else if (texelBufferView != VK_NULL_HANDLE) {
        // TEXEL BUFFER
        assert(bufferInfos.empty() && imageInfos.empty());

        write.descriptorCount = 1;
        write.pTexelBufferView = &texelBufferView;
    }

    assert(write.pBufferInfo != VK_NULL_HANDLE || write.pImageInfo != VK_NULL_HANDLE ||
           write.pTexelBufferView != VK_NULL_HANDLE);
}

void ResourceInfo::reset() {
    uniqueDataSets = 0;
    descCount = 0;
    imageInfos.clear();
    bufferInfos.clear();
    texelBufferView = VK_NULL_HANDLE;
}

}  // namespace Set

}  // namespace Descriptor