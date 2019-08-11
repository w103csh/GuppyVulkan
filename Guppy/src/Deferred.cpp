
#include "Deferred.h"

namespace Sampler {
namespace Deferred {

const CreateInfo POS_NORM_2D_ARRAY_CREATE_INFO = {
    "Deferred 2D Array Position/Normal Sampler",
    {
        {
            {::Sampler::USAGE::POSITION, "", true, false},
            {::Sampler::USAGE::NORMAL, "", true, false},
        },
        true,
        true,
    },
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
    BAD_EXTENT_2D,
    {false, true},
    0,
    SAMPLER::DEFAULT,
    (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
    {false, false, 1},
    VK_FORMAT_R16G16B16A16_SFLOAT,
};

const CreateInfo POS_2D_CREATE_INFO = {
    "Deferred 2D Array Position Sampler",
    {{{::Sampler::USAGE::POSITION}}},
    VK_IMAGE_VIEW_TYPE_2D,
    BAD_EXTENT_2D,
    {false, true},
    0,
    SAMPLER::DEFAULT,
    //(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
    (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT),
    {false, false, 1},
    VK_FORMAT_R16G16B16A16_SFLOAT,
};

const CreateInfo NORM_2D_CREATE_INFO = {
    "Deferred 2D Array Normal Sampler",
    {{{::Sampler::USAGE::POSITION}}},
    VK_IMAGE_VIEW_TYPE_2D,
    BAD_EXTENT_2D,
    {false, true},
    0,
    SAMPLER::DEFAULT,
    //(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
    (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT),
    {false, false, 1},
    VK_FORMAT_R16G16B16A16_SFLOAT,
};

const CreateInfo COLOR_2D_CREATE_INFO = {
    "Deferred 2D Color Sampler",
    {{{::Sampler::USAGE::COLOR}}},
    VK_IMAGE_VIEW_TYPE_2D,
    BAD_EXTENT_2D,
    {false, true},
    0,
    SAMPLER::DEFAULT,
    //(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
    (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT),
    {false, false, 1},
};

}  // namespace Deferred
}  // namespace Sampler

namespace Texture {
namespace Deferred {

const CreateInfo POS_NORM_2D_ARRAY_CREATE_INFO = {
    std::string(POS_NORM_2D_ARRAY_ID),
    {Sampler::Deferred::POS_NORM_2D_ARRAY_CREATE_INFO},
    false,
    false,
};

const CreateInfo POS_2D_CREATE_INFO = {
    std::string(POS_2D_ID),  //
    {Sampler::Deferred::POS_2D_CREATE_INFO},
    false,
    false,
    INPUT_ATTACHMENT::DONT_CARE,
};

const CreateInfo NORM_2D_CREATE_INFO = {
    std::string(NORM_2D_ID),  //
    {Sampler::Deferred::NORM_2D_CREATE_INFO},
    false,
    false,
    INPUT_ATTACHMENT::DONT_CARE,
};

const CreateInfo COLOR_2D_CREATE_INFO = {
    std::string(COLOR_2D_ID),  //
    {Sampler::Deferred::COLOR_2D_CREATE_INFO},
    false,
    false,
    INPUT_ATTACHMENT::DONT_CARE,
};

}  // namespace Deferred
}  // namespace Texture

namespace Descriptor {
namespace Set {
namespace Deferred {

const CreateInfo MRT_UNIFORM_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_DEFERRED_MRT,
    "_DS_UNI_DFR_MRT",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_DEFAULT}},
    },
};

const CreateInfo COMBINE_UNIFORM_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_DEFERRED_COMBINE,
    "_DS_UNI_DFR_COMB",
    {
        {{0, 0}, {UNIFORM::FOG_DEFAULT}},
        {{1, 0}, {UNIFORM::LIGHT_POSITIONAL_DEFAULT}},
        {{2, 0}, {UNIFORM::LIGHT_SPOT_DEFAULT}},
    },
};

const CreateInfo POS_NORM_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_DEFERRED_POS_NORM,
    "_DS_SMP_DFR_POS_NORM",
    {
        {{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Deferred::POS_NORM_2D_ARRAY_ID}},
    },
};

const CreateInfo POS_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_DEFERRED_POS,
    "_DS_SMP_DFR_POS",
    {
        //{{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Deferred::POS_2D_ID}},
        {{0, 0}, {INPUT_ATTACHMENT::POSITION, Texture::Deferred::POS_2D_ID}},
    },
};

const CreateInfo NORM_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_DEFERRED_NORM,
    "_DS_SMP_DFR_NORM",
    {
        //{{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Deferred::NORM_2D_ID}},
        {{0, 0}, {INPUT_ATTACHMENT::NORMAL, Texture::Deferred::NORM_2D_ID}},
    },
};

const CreateInfo COLOR_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_DEFERRED_COLOR,
    "_DS_SMP_DFR_COLOR",
    {
        //{{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Deferred::COLOR_2D_ID}},
        {{0, 0}, {INPUT_ATTACHMENT::COLOR, Texture::Deferred::COLOR_2D_ID}},
    },
};

}  // namespace Deferred
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
namespace Deferred {

const CreateInfo VERT_CREATE_INFO = {
    SHADER::DEFERRED_VERT,
    "Deferred Vertex Shader",
    "deferred.vert.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,
};

const CreateInfo FRAG_CREATE_INFO = {
    SHADER::DEFERRED_FRAG,
    "Deferred Fragment Shader",
    "deferred.frag.glsl",
    VK_SHADER_STAGE_FRAGMENT_BIT,
};
const CreateInfo MRT_VERT_CREATE_INFO = {
    SHADER::DEFERRED_MRT_VERT,
    "Deferred Multiple Render Target Vertex Shader",
    "deferred.mrt.vert.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,
};

const CreateInfo MRT_FRAG_CREATE_INFO = {
    SHADER::DEFERRED_MRT_FRAG,
    "Deferred Multiple Render Target Fragment Shader",
    "deferred.mrt.frag.glsl",
    VK_SHADER_STAGE_FRAGMENT_BIT,
};

}  // namespace Deferred
}  // namespace Shader

namespace Pipeline {
namespace Deferred {

// MRT
const Pipeline::CreateInfo MRT_CREATE_INFO = {
    PIPELINE::DEFERRED_MRT,
    "Deferred Multiple Render Target Pipeline",
    {SHADER::DEFERRED_MRT_VERT, SHADER::DEFERRED_MRT_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_DEFERRED_MRT,
        DESCRIPTOR_SET::SAMPLER_DEFAULT,
    },
};
MRT::MRT(Pipeline::Handler& handler) : Graphics(handler, &MRT_CREATE_INFO) {}

void MRT::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    VkPipelineColorBlendAttachmentState state = {VK_FALSE};
    state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // Position/Normal/Color
    createInfoRes.blendAttachmentStates.assign(3, state);

    createInfoRes.colorBlendStateInfo = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr};
    createInfoRes.colorBlendStateInfo.attachmentCount = static_cast<uint32_t>(createInfoRes.blendAttachmentStates.size());
    createInfoRes.colorBlendStateInfo.pAttachments = createInfoRes.blendAttachmentStates.data();
    createInfoRes.colorBlendStateInfo.logicOpEnable = VK_FALSE;
}

// COMBINE
const Pipeline::CreateInfo COMBINE_CREATE_INFO = {
    PIPELINE::DEFERRED_COMBINE,
    "Deferred Combine Pipeline",
    {SHADER::DEFERRED_VERT, SHADER::DEFERRED_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_DEFERRED_COMBINE,
        // DESCRIPTOR_SET::SAMPLER_DEFERRED_POS_NORM,
        DESCRIPTOR_SET::SAMPLER_DEFERRED_POS,
        DESCRIPTOR_SET::SAMPLER_DEFERRED_NORM,
        DESCRIPTOR_SET::SAMPLER_DEFERRED_COLOR,
    },
};
Combine::Combine(Pipeline::Handler& handler) : Graphics(handler, &COMBINE_CREATE_INFO) {}

}  // namespace Deferred
}  // namespace Pipeline
