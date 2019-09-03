
#include "Shadow.h"

namespace Sampler {
namespace Shadow {

const CreateInfo MAP_2D_ARRAY_CREATE_INFO = {
    "Shadow 2D Array Map Sampler",
    {
        {
            {::Sampler::USAGE::DEPTH, "", true, true},
            //{::Sampler::USAGE::NORMAL, "", true, false},
        },
        true,
        true,
    },
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
    {1024, 768},  // BAD_EXTENT_2D,
    {},
    0,
    SAMPLER::CLAMP_TO_BORDER_DEPTH,
    (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
};

}  // namespace Shadow
}  // namespace Sampler

namespace Texture {
namespace Shadow {

const CreateInfo MAP_2D_ARRAY_CREATE_INFO = {
    std::string(MAP_2D_ARRAY_ID),  //
    {Sampler::Shadow::MAP_2D_ARRAY_CREATE_INFO},
    false,
    false,
    COMBINED_SAMPLER::PIPELINE_DEPTH,
};

}  // namespace Shadow
}  // namespace Texture

namespace Light {
namespace Shadow {

Positional::Positional(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::PerFramebufferDataItem<DATA>(pData),           //
      proj_(pCreateInfo->proj)                               //
{
    update(pCreateInfo->mainCameraSpaceToWorldSpace);
}

void Positional::update(const glm::mat4& m, const uint32_t frameIndex) {
    data_.proj = proj_ * m;
    setData(frameIndex);
}

}  // namespace Shadow
}  // namespace Light

namespace Descriptor {
namespace Set {
namespace Shadow {

const CreateInfo UNIFORM_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_SHADOW,
    "_DS_UNI_SHDW",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
    },
};

const CreateInfo SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_SHADOW,
    "_DS_SMP_SHDW",
    {{{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Shadow::MAP_2D_ARRAY_ID}}},
};

}  // namespace Shadow
}  // namespace Set
}  // namespace Descriptor

namespace Pipeline {

void GetShadowRasterizationStateInfoResources(Pipeline::CreateInfoResources& createInfoRes) {
    createInfoRes.rasterizationStateInfo = {};
    createInfoRes.rasterizationStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    createInfoRes.rasterizationStateInfo.lineWidth = 1.0f;
    createInfoRes.rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    createInfoRes.rasterizationStateInfo.cullMode = VK_CULL_MODE_FRONT_BIT;  // This worked great !!!
    createInfoRes.rasterizationStateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    /* If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far
     *  planes are clamped to them as opposed to discarding them. This is useful in some special
     *  cases like shadow maps. Using this requires enabling a GPU feature.
     */
    createInfoRes.rasterizationStateInfo.depthClampEnable = VK_FALSE;
    createInfoRes.rasterizationStateInfo.rasterizerDiscardEnable = VK_FALSE;
    /* I've seen literally no results with changing the constant factor. I probably just dont understand
     *  how it works. The slope factor on the other hand seemed to help tremendously. Ultimately the most
     *  success came from setting the cullMode to VK_CULL_MODE_FRONT_BIT which eliminated almost all of
     *  the "shadow acne".
     */
    createInfoRes.rasterizationStateInfo.depthBiasEnable = VK_TRUE;
    createInfoRes.rasterizationStateInfo.depthBiasConstantFactor = 0.0f;
    createInfoRes.rasterizationStateInfo.depthBiasClamp = 0.0f;
    createInfoRes.rasterizationStateInfo.depthBiasSlopeFactor = 0.0f;
}

namespace Shadow {

// COLOR
const Pipeline::CreateInfo COLOR_CREATE_INFO = {
    PIPELINE::SHADOW_COLOR,
    "Shadow Color Pipeline",
    {SHADER::SHADOW_COLOR_VERT, SHADER::SHADOW_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_SHADOW,
    },
};
Color::Color(Pipeline::Handler& handler) : Graphics(handler, &COLOR_CREATE_INFO) {}

void Color::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    GetShadowRasterizationStateInfoResources(createInfoRes);
}

// TEXTURE
const Pipeline::CreateInfo TEX_CREATE_INFO = {
    PIPELINE::SHADOW_TEX,
    "Shadow Texture Pipeline",
    {SHADER::SHADOW_TEX_VERT, SHADER::SHADOW_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_SHADOW,
    },
};
Texture::Texture(Pipeline::Handler& handler) : Graphics(handler, &TEX_CREATE_INFO) {}

void Texture::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    GetShadowRasterizationStateInfoResources(createInfoRes);
}

}  // namespace Shadow
}  // namespace Pipeline
