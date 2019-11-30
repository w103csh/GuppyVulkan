/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "DescriptorConstants.h"

#include <sstream>

#include "BufferItem.h"
#include "ConstantsAll.h"

namespace Descriptor {

void OffsetsMap::insert(const DESCRIPTOR& key, const Uniform::offsets& value) {
    if (std::visit(IsCombinedSampler{}, key)) {
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
    DESCRIPTOR_SET::UNIFORM_CAMERA_ONLY,
    DESCRIPTOR_SET::UNIFORM_DEFCAM_DEFMAT_MX4,
    DESCRIPTOR_SET::UNIFORM_CAM_MATOBJ3D,
    DESCRIPTOR_SET::UNIFORM_OBJ3D,
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
    DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_HDR_LOG_BLIT,
    DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_BLUR_A,
    DESCRIPTOR_SET::SAMPLER_SCREEN_SPACE_BLUR_B,
    DESCRIPTOR_SET::STORAGE_SCREEN_SPACE_COMPUTE_POST_PROCESS,
    DESCRIPTOR_SET::STORAGE_IMAGE_SCREEN_SPACE_COMPUTE_DEFAULT,
    // DEFERRED
    DESCRIPTOR_SET::UNIFORM_DEFERRED_MRT,
    DESCRIPTOR_SET::UNIFORM_DEFERRED_SSAO,
    DESCRIPTOR_SET::UNIFORM_DEFERRED_COMBINE,
    DESCRIPTOR_SET::SAMPLER_DEFERRED,
    DESCRIPTOR_SET::SAMPLER_DEFERRED_SSAO_RANDOM,
    // SHADOW
    DESCRIPTOR_SET::UNIFORM_SHADOW,
    DESCRIPTOR_SET::SAMPLER_SHADOW,
    DESCRIPTOR_SET::SAMPLER_SHADOW_OFFSET,
    // TESSELLATION
    DESCRIPTOR_SET::UNIFORM_TESSELLATION_DEFAULT,
    // GEOMETRY
    DESCRIPTOR_SET::UNIFORM_GEOMETRY_DEFAULT,
    // PARTICLE
    DESCRIPTOR_SET::UNIFORM_PRTCL_WAVE,
    DESCRIPTOR_SET::UNIFORM_PRTCL_FOUNTAIN,
    DESCRIPTOR_SET::PRTCL_EULER,
    DESCRIPTOR_SET::PRTCL_ATTRACTOR,
    DESCRIPTOR_SET::PRTCL_CLOTH,
    DESCRIPTOR_SET::PRTCL_CLOTH_NORM,
    // WATER
    DESCRIPTOR_SET::HFF,
    DESCRIPTOR_SET::HFF_DEF,
};

void ResourceInfo::setWriteInfo(const uint32_t index, VkWriteDescriptorSet& write) const {
    auto offset = (std::min)(index, uniqueDataSets - 1);

    if (imageInfos.size()) {
        // IMAGE
        assert(bufferInfos.empty() && texelBufferView == VK_NULL_HANDLE);

        assert(descCount == 1);  // I haven't thought about this yet.

        write.descriptorCount = descCount;
        write.pImageInfo = &imageInfos.at(offset).descInfoMap.at(Sampler::IMAGE_ARRAY_LAYERS_ALL);

    } else if (bufferInfos.size()) {
        // BUFFER
        assert(imageInfos.empty() && texelBufferView == VK_NULL_HANDLE);

        offset *= descCount;
        assert((offset + descCount) <= static_cast<uint32_t>(bufferInfos.size()));

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

namespace Default {

const CreateInfo UNIFORM_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_DEFAULT,
    "_DS_UNI_DEF",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_DEFAULT}},
        {{2, 0}, {UNIFORM::FOG_DEFAULT}},
        {{3, 0}, {UNIFORM::LIGHT_POSITIONAL_DEFAULT}},
        {{4, 0}, {UNIFORM::LIGHT_SPOT_DEFAULT}},
    },
};

const CreateInfo UNIFORM_CAMERA_ONLY_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_CAMERA_ONLY,
    "_DS_UNI_CAM_ONLY",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
    },
};

const CreateInfo UNIFORM_DEFCAM_DEFMAT_MX4 = {
    DESCRIPTOR_SET::UNIFORM_DEFCAM_DEFMAT_MX4,
    "_DS_UNI_DEFCAM_DEFMAT_MX4",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_DEFAULT}},
        {{2, 0}, {UNIFORM_DYNAMIC::MATRIX_4}},
    },
};

const CreateInfo UNIFORM_CAM_MATOBJ3D_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_CAM_MATOBJ3D,
    "_DS_UNI_CAM_MAT",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_OBJ3D}},
    },
};

const CreateInfo UNIFORM_OBJ3D_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_OBJ3D,
    "_DS_UNI_OBJ3D",
    {
        {{0, 0}, {UNIFORM::OBJ3D}},
    },
};

const CreateInfo SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_DEFAULT,
    "_DS_SMP_DEF",
    {
        {{0, 0}, {COMBINED_SAMPLER::MATERIAL}},
        //{{1, 0}, {DESCRIPTOR::SAMPLER_MATERIAL_COMBINED}},
        //{{2, 0}, {DESCRIPTOR::SAMPLER_MATERIAL_COMBINED}},
        //{{3, 0}, {DESCRIPTOR::SAMPLER_MATERIAL_COMBINED}},
    },
};

const CreateInfo CUBE_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_CUBE_DEFAULT,
    "_DS_CBE_DEF",
    {
        {{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::SKYBOX_ID}},
    },
};

const CreateInfo PROJECTOR_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::PROJECTOR_DEFAULT,
    "_DS_PRJ_DEF",
    {
        //{{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::STATUE_ID}},
        {{0, 0}, {COMBINED_SAMPLER::PIPELINE, ::RenderPass::PROJECT_2D_TEXTURE_ID}},
        {{1, 0}, {UNIFORM::PROJECTOR_DEFAULT}},
    },
};

}  // namespace Default

}  // namespace Set

std::string GetPerframeBufferWarning(const DESCRIPTOR descType, const Buffer::Info& buffInfo,
                                     const Set::ResourceInfo& resInfo, bool doAssert) {
    if (buffInfo.count != resInfo.uniqueDataSets) {
        std::stringstream sMsg;
        sMsg << "Descriptor with type (" << std::visit(Descriptor::GetDescriptorTypeString{}, descType);
        sMsg << ") has BUFFER_INFO count of (" << buffInfo.count;
        sMsg << ") and a uniqueDataSets count of (" << resInfo.uniqueDataSets << ").";
        if (doAssert) assert(false && "Did you mean to add the type to Descriptor::HasPerFramebufferData?");
        return sMsg.str();
    }
    return "";
}

}  // namespace Descriptor