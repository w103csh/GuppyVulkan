
#include "Deferred.h"

// HANDLERS
#include "PipelineHandler.h"

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
    BAD_EXTENT_3D,
    {false, true, 1.0f, ::Deferred::DO_MSAA},
    0,
    SAMPLER::DEFAULT,
    (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
    {false, false, 1},
    VK_FORMAT_R16G16B16A16_SFLOAT,
};

const CreateInfo POS_2D_CREATE_INFO = {
    "Deferred 2D Position Sampler",
    {{{::Sampler::USAGE::POSITION}}},
    VK_IMAGE_VIEW_TYPE_2D,
    BAD_EXTENT_3D,
    {false, true, 1.0f, ::Deferred::DO_MSAA},
    0,
    SAMPLER::DEFAULT,
    //(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
    (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT),
    {false, false, 1},
    VK_FORMAT_R16G16B16A16_SFLOAT,
};

const CreateInfo NORM_2D_CREATE_INFO = {
    "Deferred 2D Normal Sampler",
    {{{::Sampler::USAGE::POSITION}}},
    VK_IMAGE_VIEW_TYPE_2D,
    BAD_EXTENT_3D,
    {false, true, 1.0f, ::Deferred::DO_MSAA},
    0,
    SAMPLER::DEFAULT,
    //(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
    (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT),
    {false, false, 1},
    VK_FORMAT_R16G16B16A16_SFLOAT,
};

const CreateInfo DIFFUSE_2D_CREATE_INFO = {
    "Deferred 2D Diffuse Color Sampler",
    {{{::Sampler::USAGE::COLOR}}},
    VK_IMAGE_VIEW_TYPE_2D,
    BAD_EXTENT_3D,
    {false, true, 1.0f, ::Deferred::DO_MSAA},
    0,
    SAMPLER::DEFAULT,
    (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT),
    {false, false, 1},
    VK_FORMAT_R8G8B8A8_UNORM,
};

const CreateInfo AMBIENT_2D_CREATE_INFO = {
    "Deferred 2D Ambient Color Sampler",
    {{{::Sampler::USAGE::COLOR}}},
    VK_IMAGE_VIEW_TYPE_2D,
    BAD_EXTENT_3D,
    {false, true, 1.0f, ::Deferred::DO_MSAA},
    0,
    SAMPLER::DEFAULT,
    (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT),
    {false, false, 1},
    VK_FORMAT_R8G8B8A8_UNORM,
};

const CreateInfo SPECULAR_2D_CREATE_INFO = {
    "Deferred 2D Specular Color Sampler",
    {{{::Sampler::USAGE::COLOR}}},
    VK_IMAGE_VIEW_TYPE_2D,
    BAD_EXTENT_3D,
    {false, true, 1.0f, ::Deferred::DO_MSAA},
    0,
    SAMPLER::DEFAULT,
    (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT),
    {false, false, 1},
    VK_FORMAT_R8G8B8A8_UNORM,
};

const CreateInfo SSAO_2D_CREATE_INFO = {
    "Deferred 2D SSAO Sampler",
    {{{::Sampler::USAGE::COLOR}}},  // COLOR ???
    VK_IMAGE_VIEW_TYPE_2D,
    BAD_EXTENT_3D,
    {false, true},
    0,
    SAMPLER::DEFAULT,
    (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT),
    {false, false, 1},
    VK_FORMAT_R16_SFLOAT,
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

const CreateInfo DIFFUSE_2D_CREATE_INFO = {
    std::string(DIFFUSE_2D_ID),  //
    {Sampler::Deferred::DIFFUSE_2D_CREATE_INFO},
    false,
    false,
    INPUT_ATTACHMENT::DONT_CARE,
};

const CreateInfo AMBIENT_2D_CREATE_INFO = {
    std::string(AMBIENT_2D_ID),  //
    {Sampler::Deferred::AMBIENT_2D_CREATE_INFO},
    false,
    false,
    INPUT_ATTACHMENT::DONT_CARE,
};

const CreateInfo SPECULAR_2D_CREATE_INFO = {
    std::string(SPECULAR_2D_ID),  //
    {Sampler::Deferred::SPECULAR_2D_CREATE_INFO},
    false,
    false,
    INPUT_ATTACHMENT::DONT_CARE,
};

const CreateInfo SSAO_2D_CREATE_INFO = {
    std::string(SSAO_2D_ID),  //
    {Sampler::Deferred::SSAO_2D_CREATE_INFO},
    false,
    false,
    INPUT_ATTACHMENT::DONT_CARE,
};

CreateInfo MakeSSAORandRotationTex(Random& rand) {
    uint32_t size = 4;
    uint32_t channels = 4;  // VK_FORMAT_R16G16B16A16_SFLOAT

    float* pData = new float[static_cast<size_t>(channels) * static_cast<size_t>(size) * static_cast<size_t>(size)]();
    for (uint32_t i = 0; i < size * size; i++) {
        glm::vec3 v = rand.uniformCircle();
        pData[i * channels + 0] = v.x;
        pData[i * channels + 1] = v.y;
        pData[i * channels + 2] = v.z;
        pData[i * channels + 3] = 0.0;
    }

    Sampler::CreateInfo sampInfo = {
        "Deferred 2D SSAO Random Sampler",
        {{{::Sampler::USAGE::POSITION}}},
        VK_IMAGE_VIEW_TYPE_2D,
        {size, size, 1},
        {},
        0,
        SAMPLER::DEFAULT,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        {false, false, 1},
        VK_FORMAT_R32G32B32A32_SFLOAT,
        Sampler::CHANNELS::_4,
        sizeof(float),
    };

    sampInfo.layersInfo.infos.front().pPixel = (stbi_uc*)pData;

    return {std::string(SSAO_RAND_2D_ID), {sampInfo}, false};
}

}  // namespace Deferred
}  // namespace Texture

namespace Uniform {
namespace Deferred {

SSAO::SSAO(const Buffer::Info&& info, DATA* pData)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::DataItem<DATA>(pData) {}

void SSAO::init(Random& rand) {
    for (int i = 0; i < KERNEL_SIZE; i++) {
        glm::vec3 randDir = rand.uniformHemisphere();
        float scale = ((float)(i * i)) / (KERNEL_SIZE * KERNEL_SIZE);
        randDir *= glm::mix(0.1f, 1.0f, scale);

        pData_->kern[i * 3 + 0] = randDir.x;
        pData_->kern[i * 3 + 1] = randDir.y;
        pData_->kern[i * 3 + 2] = randDir.z;
    }
    dirty = true;
}

}  // namespace Deferred
}  // namespace Uniform

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
        {{3, 0}, {UNIFORM::LIGHT_POSITIONAL_SHADOW}},
        {{4, 0}, {UNIFORM::SHADOW_DATA}},
    },
};

const CreateInfo SSAO_UNIFORM_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_DEFERRED_SSAO,
    "_DS_UNI_DFR_SSAO",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM::DEFERRED_SSAO}},
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

const CreateInfo DIFFUSE_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_DEFERRED_DIFFUSE,
    "_DS_SMP_DFR_DIFF",
    {
        {{0, 0}, {INPUT_ATTACHMENT::COLOR, Texture::Deferred::DIFFUSE_2D_ID}},
    },
};

const CreateInfo AMBIENT_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_DEFERRED_AMBIENT,
    "_DS_SMP_DFR_AMB",
    {
        {{0, 0}, {INPUT_ATTACHMENT::COLOR, Texture::Deferred::AMBIENT_2D_ID}},
    },
};

const CreateInfo SPECULAR_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_DEFERRED_SPECULAR,
    "_DS_SMP_DFR_SPEC",
    {
        {{0, 0}, {INPUT_ATTACHMENT::COLOR, Texture::Deferred::SPECULAR_2D_ID}},
    },
};

const CreateInfo SSAO_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_DEFERRED_SSAO,
    "_DS_SMP_DFR_SSAO",
    {{{0, 0}, {INPUT_ATTACHMENT::SSAO, Texture::Deferred::SSAO_2D_ID}}},
};

const CreateInfo SSAO_RANDOM_SAMPLER_CREATE_INFO = {
    DESCRIPTOR_SET::SAMPLER_DEFERRED_SSAO_RANDOM,
    "_DS_SMP_DFR_SSAO_RAND",
    {
        {{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Deferred::SSAO_RAND_2D_ID}},
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
    "vert.deferred.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,
};

const CreateInfo FRAG_CREATE_INFO = {
    SHADER::DEFERRED_FRAG,
    "Deferred Fragment Shader",
    "frag.deferred.glsl",
    VK_SHADER_STAGE_FRAGMENT_BIT,
};

const CreateInfo FRAG_MS_CREATE_INFO = {
    SHADER::DEFERRED_FRAG,
    "Deferred Multi-Sample Fragment Shader",
    "frag.deferred.ms.glsl",
    VK_SHADER_STAGE_FRAGMENT_BIT,
};

const CreateInfo MRT_TEX_WS_VERT_CREATE_INFO = {
    SHADER::DEFERRED_MRT_TEX_WS_VERT,
    "Deferred Multiple Render Target Texture World Space Vertex Shader",
    "vert.texture.deferred.mrt.ws.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,
};

const CreateInfo MRT_TEX_CS_VERT_CREATE_INFO = {
    SHADER::DEFERRED_MRT_TEX_CS_VERT,
    "Deferred Multiple Render Target Texture Camera Space Vertex Shader",
    "vert.texture.deferred.mrt.cs.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,
};

const CreateInfo MRT_TEX_FRAG_CREATE_INFO = {
    SHADER::DEFERRED_MRT_TEX_FRAG,
    "Deferred Multiple Render Target Texture Fragment Shader",
    "frag.texture.deferred.mrt.glsl",
    VK_SHADER_STAGE_FRAGMENT_BIT,
    {
        SHADER_LINK::TEX_FRAG,
        SHADER_LINK::DEFAULT_MATERIAL,
    },
};

const CreateInfo MRT_COLOR_CS_VERT_CREATE_INFO = {
    SHADER::DEFERRED_MRT_COLOR_CS_VERT,
    "Deferred Multiple Render Target Color Camera Space Vertex Shader",
    "vert.color.deferred.mrt.cs.glsl",
    VK_SHADER_STAGE_VERTEX_BIT,
};

const CreateInfo MRT_COLOR_FRAG_CREATE_INFO = {
    SHADER::DEFERRED_MRT_COLOR_FRAG,
    "Deferred Multiple Render Target Color Fragment Shader",
    "frag.color.deferred.mrt.glsl",
    VK_SHADER_STAGE_FRAGMENT_BIT,
    {
        SHADER_LINK::COLOR_FRAG,
        SHADER_LINK::DEFAULT_MATERIAL,
    },
};

const CreateInfo SSAO_FRAG_CREATE_INFO = {
    SHADER::DEFERRED_MRT_COLOR_FRAG,
    "Deferred SSAO Fragment Shader",
    "frag.deferred.ssao.glsl",
    VK_SHADER_STAGE_FRAGMENT_BIT,
};

}  // namespace Deferred
}  // namespace Shader

namespace Pipeline {
namespace Deferred {

void GetDefaultBlendInfoResources(CreateInfoResources& createInfoRes) {
    VkPipelineColorBlendAttachmentState state = {VK_FALSE};
    state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    // Position/Normal/Diffuse/Ambient/Specular
    createInfoRes.blendAttachmentStates.assign(5, state);

    createInfoRes.colorBlendStateInfo = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, nullptr};
    createInfoRes.colorBlendStateInfo.attachmentCount = static_cast<uint32_t>(createInfoRes.blendAttachmentStates.size());
    createInfoRes.colorBlendStateInfo.pAttachments = createInfoRes.blendAttachmentStates.data();
    createInfoRes.colorBlendStateInfo.logicOpEnable = VK_FALSE;
}

// MRT (TEXTURE)
const Pipeline::CreateInfo MRT_TEX_CREATE_INFO = {
    PIPELINE::DEFERRED_MRT_TEX,
    "Deferred Multiple Render Target Texture Pipeline",
    {
        // SHADER::DEFERRED_MRT_TEX_WS_VERT,
        SHADER::DEFERRED_MRT_TEX_CS_VERT,
        SHADER::DEFERRED_MRT_TEX_FRAG,
    },
    {
        // DESCRIPTOR_SET::UNIFORM_DEFERRED_MRT,
        DESCRIPTOR_SET::UNIFORM_DEFAULT,
        DESCRIPTOR_SET::SAMPLER_DEFAULT,
    },
};
MRTTexture::MRTTexture(Pipeline::Handler& handler) : Graphics(handler, &MRT_TEX_CREATE_INFO) {}

void MRTTexture::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    GetDefaultBlendInfoResources(createInfoRes);  //
}

//// MRT (TEXTURE WIREFRAME)
// MRTTextureWireFrame::MRTTextureWireFrame(Handler& handler) : MRTTexture(handler) {}
// void MRTTextureWireFrame::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
//    MRTTexture::getRasterizationStateInfoResources(createInfoRes);
//    createInfoRes.rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_LINE;
//    createInfoRes.rasterizationStateInfo.cullMode = VK_CULL_MODE_NONE;
//}

// MRT (COLOR)
const Pipeline::CreateInfo MRT_COLOR_CREATE_INFO = {
    PIPELINE::DEFERRED_MRT_COLOR,
    "Deferred Multiple Render Target Color Pipeline",
    {SHADER::DEFERRED_MRT_COLOR_CS_VERT, SHADER::DEFERRED_MRT_COLOR_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_DEFAULT,
        // DESCRIPTOR_SET::SAMPLER_DEFAULT,
    },
};
MRTColor::MRTColor(Pipeline::Handler& handler) : Graphics(handler, &MRT_COLOR_CREATE_INFO) {}
MRTColor::MRTColor(Pipeline::Handler& handler, const CreateInfo* pCreateInfo) : Graphics(handler, pCreateInfo) {}

void MRTColor::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    GetDefaultBlendInfoResources(createInfoRes);  //
}

// MRT (COLOR WIREFRAME)
const Pipeline::CreateInfo MRT_COLOR_WF_CREATE_INFO = {
    PIPELINE::DEFERRED_MRT_WF_COLOR,
    "Deferred Multiple Render Target Color Wireframe Pipeline",
    {SHADER::DEFERRED_MRT_COLOR_CS_VERT, SHADER::DEFERRED_MRT_COLOR_FRAG},
    {DESCRIPTOR_SET::UNIFORM_DEFAULT},
};
MRTColorWireFrame::MRTColorWireFrame(Handler& handler) : MRTColor(handler, &MRT_COLOR_WF_CREATE_INFO) {}
void MRTColorWireFrame::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    MRTColor::getRasterizationStateInfoResources(createInfoRes);
    createInfoRes.rasterizationStateInfo.polygonMode = VK_POLYGON_MODE_LINE;
    createInfoRes.rasterizationStateInfo.cullMode = VK_CULL_MODE_NONE;
}

// MRT (LINE)
const Pipeline::CreateInfo MRT_LINE_CREATE_INFO = {
    PIPELINE::DEFERRED_MRT_LINE,
    "Deferred Multiple Render Target Line Pipeline",
    {SHADER::DEFERRED_MRT_COLOR_CS_VERT, SHADER::DEFERRED_MRT_COLOR_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_DEFAULT,
        // DESCRIPTOR_SET::SAMPLER_DEFAULT,
    },
};
MRTLine::MRTLine(Pipeline::Handler& handler) : Graphics(handler, &MRT_LINE_CREATE_INFO) {}

void MRTLine::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    GetDefaultBlendInfoResources(createInfoRes);  //
}

void MRTLine::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    GetDefaultColorInputAssemblyInfoResources(createInfoRes);
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
}

// SSAO
const Pipeline::CreateInfo AO_CREATE_INFO = {
    PIPELINE::DEFERRED_SSAO,
    "Deferred SSAO Pipeline",
    {SHADER::DEFERRED_VERT, SHADER::DEFERRED_SSAO_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_DEFERRED_SSAO,
        DESCRIPTOR_SET::SAMPLER_DEFERRED_SSAO_RANDOM,
        DESCRIPTOR_SET::SAMPLER_DEFERRED_POS,
        DESCRIPTOR_SET::SAMPLER_DEFERRED_NORM,
    },
};
SSAO::SSAO(Pipeline::Handler& handler) : Graphics(handler, &AO_CREATE_INFO) {}

// COMBINE
const Pipeline::CreateInfo COMBINE_CREATE_INFO = {
    PIPELINE::DEFERRED_COMBINE,
    "Deferred Combine Pipeline",
    {
        SHADER::DEFERRED_VERT,
        SHADER::DEFERRED_MS_FRAG,
    },
    {
        DESCRIPTOR_SET::UNIFORM_DEFERRED_COMBINE,  //
        DESCRIPTOR_SET::SAMPLER_SHADOW,            //
        DESCRIPTOR_SET::SAMPLER_SHADOW_OFFSET,
        // DESCRIPTOR_SET::SAMPLER_DEFERRED_POS_NORM,
        DESCRIPTOR_SET::SAMPLER_DEFERRED_POS,      //
        DESCRIPTOR_SET::SAMPLER_DEFERRED_NORM,     //
        DESCRIPTOR_SET::SAMPLER_DEFERRED_DIFFUSE,  //
        DESCRIPTOR_SET::SAMPLER_DEFERRED_AMBIENT,  //
        DESCRIPTOR_SET::SAMPLER_DEFERRED_SPECULAR,
        // DESCRIPTOR_SET::SAMPLER_DEFERRED_SSAO,
    },
};
Combine::Combine(Pipeline::Handler& handler) : Graphics(handler, &COMBINE_CREATE_INFO), doMSAA_(::Deferred::DO_MSAA) {}

void Combine::init() {
    if (!doMSAA_) {
        replaceShaderType(SHADER::DEFERRED_MS_FRAG, SHADER::DEFERRED_FRAG);
    } else {
        assert(handler().shell().context().samples != VK_SAMPLE_COUNT_1_BIT);
    }
    Graphics::init();
}

void Combine::getShaderStageInfoResources(CreateInfoResources& createInfoRes) {
    /* This was never used because I couldn't figure out a way to manually resolve the multi-sample
     *  input attachments without doing it twice. This might be possible if you can manually do the
     *  the "resolve attachment" step. There is another possibility I saw as well where you can enable
     *  device feature for both nvidia, and amd for "mixed attachment samples" or something like that.
     *
     * Note: the initial reason for trying this was to address aliasing in the shadows by sampling the
     *  depth attachment base on the angle of the surface. This is probably possible without doing the
     *  multi-sample manually.
     */
    if (doMSAA_) {
        createInfoRes.specializationMapEntries.push_back({{}});

        // Use specialization constants to pass number of samples to the shader (used for MSAA resolve)
        createInfoRes.specializationMapEntries.back().back().constantID = 0;
        createInfoRes.specializationMapEntries.back().back().offset = 0;
        createInfoRes.specializationMapEntries.back().back().size = sizeof(VkSampleCountFlagBits);

        createInfoRes.specializationInfo.push_back({});
        createInfoRes.specializationInfo.back().mapEntryCount =
            static_cast<uint32_t>(createInfoRes.specializationMapEntries.back().size());
        createInfoRes.specializationInfo.back().pMapEntries = createInfoRes.specializationMapEntries.back().data();
        createInfoRes.specializationInfo.back().dataSize = sizeof(handler().shell().context().samples);
        createInfoRes.specializationInfo.back().pData = &handler().shell().context().samples;

        assert(createInfoRes.shaderStageInfos.size() == 2 &&
               createInfoRes.shaderStageInfos[1].stage == VK_SHADER_STAGE_FRAGMENT_BIT);
        // Add the specialization to the fragment shader info.
        createInfoRes.shaderStageInfos[1].pSpecializationInfo = &createInfoRes.specializationInfo.back();
    }
}

}  // namespace Deferred
}  // namespace Pipeline
