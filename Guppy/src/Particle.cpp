
#include "Particle.h"

#include "Deferred.h"
#include "ParticleBuffer.h"
#include "Shadow.h"
#include "Storage.h"
// HANDLERS
#include "PipelineHandler.h"

// SHADER

namespace Shader {
namespace Particle {
const CreateInfo WAVE_VERT_CREATE_INFO = {
    SHADER::PRTCL_WAVE_VERT,                         //
    "Particle Wave Color (Deferred) Vertex Shader",  //
    "vert.particle.wave.glsl",                       //
    VK_SHADER_STAGE_VERTEX_BIT,                      //
};
const CreateInfo FOUNTAIN_VERT_CREATE_INFO = {
    SHADER::PRTCL_FOUNTAIN_VERT,        //
    "Particle Fountain Vertex Shader",  //
    "vert.particle.fountain.glsl",      //
    VK_SHADER_STAGE_VERTEX_BIT,         //
    {SHADER_LINK::PRTCL_FOUNTAIN},
};
const CreateInfo FOUNTAIN_FRAG_DEFERRED_MRT_CREATE_INFO = {
    SHADER::PRTCL_FOUNTAIN_DEFERRED_MRT_FRAG,        //
    "Particle Fountain (Deferred) Fragment Shader",  //
    "frag.particle.deferred.mrt.fountain.glsl",      //
    VK_SHADER_STAGE_FRAGMENT_BIT,                    //
    {SHADER_LINK::DEFAULT_MATERIAL},
};
// EULER
const CreateInfo EULER_CREATE_INFO = {
    SHADER::PRTCL_EULER_COMP,         //
    "Particle Euler Compute Shader",  //
    "comp.particle.euler.glsl",       //
    VK_SHADER_STAGE_COMPUTE_BIT,      //
    {SHADER_LINK::PRTCL_FOUNTAIN},
};
const CreateInfo FOUNTAIN_EULER_VERT_CREATE_INFO = {
    SHADER::PRTCL_FOUNTAIN_EULER_VERT,        //
    "Particle Fountain Euler Vertex Shader",  //
    "vert.particle.fountainEuler.glsl",       //
    VK_SHADER_STAGE_VERTEX_BIT,               //
    {
        SHADER_LINK::DEFAULT_MATERIAL,
        SHADER_LINK::PRTCL_FOUNTAIN,
    },
};
const CreateInfo SHDW_FOUNTAIN_EULER_VERT_CREATE_INFO = {
    SHADER::PRTCL_SHDW_FOUNTAIN_EULER_VERT,            //
    "Particle Fountain Euler (Shadow) Vertex Shader",  //
    "vert.particle.fountainEuler.shadow.glsl",         //
    VK_SHADER_STAGE_VERTEX_BIT,                        //
    {SHADER_LINK::DEFAULT_MATERIAL},
};
// ATTRACTOR
const CreateInfo ATTR_COMP_CREATE_INFO = {
    SHADER::PRTCL_ATTR_COMP,              //
    "Particle Attractor Compute Shader",  //
    "comp.particle.attractor.glsl",       //
    VK_SHADER_STAGE_COMPUTE_BIT,          //
};
const CreateInfo ATTR_VERT_CREATE_INFO = {
    SHADER::PRTCL_ATTR_VERT,             //
    "Particle Attractor Vertex Shader",  //
    "vert.particle.attractor.glsl",      //
    VK_SHADER_STAGE_VERTEX_BIT,          //
};
}  // namespace Particle
namespace Link {
namespace Particle {
const CreateInfo FOUNTAIN_CREATE_INFO = {
    SHADER_LINK::COLOR_FRAG,        //
    "link.particle.fountain.glsl",  //
};
}  // namespace Particle
}  // namespace Link
}  // namespace Shader

// UNIFORM

namespace Uniform {
namespace Particle {

namespace Wave {
Base::Base(const Buffer::Info&& info, DATA* pData, const Buffer::CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Descriptor::Base(UNIFORM::PRTCL_WAVE),
      Buffer::PerFramebufferDataItem<DATA>(pData) {
    setData();
}

void Base::updatePerFrame(const float time, const float, const uint32_t frameIndex) {
    data_.time = time;
    setData(frameIndex);
}
}  // namespace Wave

}  // namespace Particle
}  // namespace Uniform

// UNIFORM DYNAMIC

namespace UniformDynamic {
namespace Particle {

// FOUNTAIN
namespace Fountain {

Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM_DYNAMIC::PRTCL_FOUNTAIN),
      Buffer::PerFramebufferDataItem<DATA>(pData),
      INSTANCE_TYPE(pCreateInfo->type) {
    data_.acceleration = pCreateInfo->acceleration;
    data_.lifespan = pCreateInfo->lifespan;
    data_.emitterBasis = pCreateInfo->emitterBasis;
    data_.emitterPosition = pCreateInfo->emitterBasis * glm::vec4(.0f, 0.0f, 0.0f, 1.0f);
    data_.minParticleSize = pCreateInfo->minParticleSize;
    data_.maxParticleSize = pCreateInfo->maxParticleSize;
    data_.velocityLowerBound = pCreateInfo->velocityLowerBound;
    data_.velocityUpperBound = pCreateInfo->velocityUpperBound;
    setData();
}

void Base::updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) {
    switch (INSTANCE_TYPE) {
        case INSTANCE::DEFAULT: {
            if (data_.time < 0.0) {
                data_.time = time;
                // This should be 0.0f but it needs to be positive since its used as a flag as well.
                data_.delta = 0.0000001f;
            } else {
                data_.delta = time - data_.time;
            }
        } break;
        case INSTANCE::EULER: {
            data_.time = time;
            data_.delta = elapsed;
        } break;
        default: {
            assert(false && "Unhandled type");
        } break;
    }
    setData(frameIndex);
}

void Base::reset() {
    data_.delta = data_.time = ::Particle::BAD_TIME;
    setData();
}

}  // namespace Fountain

// ATTRACTOR
namespace Attractor {

Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Descriptor::Base(UNIFORM_DYNAMIC::PRTCL_ATTRACTOR),
      Buffer::PerFramebufferDataItem<DATA>(pData) {
    data_.attractorPosition0 = pCreateInfo->attractorPosition0;
    data_.gravity0 = pCreateInfo->gravity0;
    data_.attractorPosition1 = pCreateInfo->attractorPosition1;
    data_.gravity1 = pCreateInfo->gravity1;
    data_.inverseMass = pCreateInfo->inverseMass;
    data_.delta = ::Particle::BAD_TIME;
    data_.maxDistance = pCreateInfo->maxDistance;
}

void Base::updatePerFrame(const float time, const float elapsed, const uint32_t frameIndex) {
    data_.delta = elapsed;
    setData(frameIndex);
}

}  // namespace Attractor

}  // namespace Particle
}  // namespace UniformDynamic

// DESCRIPTOR SET

namespace Descriptor {
namespace Set {
namespace Particle {
const CreateInfo WAVE_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_PRTCL_WAVE,
    "_DS_UNI_PRTCL_WV",
    {
        {{0, 0}, {UNIFORM::PRTCL_WAVE}},
    },
};
const CreateInfo FOUNTAIN_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_PRTCL_FOUNTAIN,
    "_DS_UNI_PRTCL_FNTN",
    {
        {{0, 0}, {UNIFORM_DYNAMIC::PRTCL_FOUNTAIN}},
    },
};
const CreateInfo EULER_CREATE_INFO = {
    DESCRIPTOR_SET::PRTCL_EULER,
    "_DS_PRTCL_EULER",
    {
        {{0, 0}, {COMBINED_SAMPLER::PIPELINE, Texture::Particle::RAND_1D_ID}},
        {{1, 0}, {STORAGE_BUFFER_DYNAMIC::PRTCL_EULER}},
    },
};
const CreateInfo ATTRACTOR_CREATE_INFO = {
    DESCRIPTOR_SET::PRTCL_ATTRACTOR,
    "_DS_PRTCL_ATTR",
    {
        {{0, 0}, {UNIFORM_DYNAMIC::PRTCL_ATTRACTOR}},
        {{1, 0}, {STORAGE_BUFFER_DYNAMIC::PRTCL_POSITION}},
        {{2, 0}, {STORAGE_BUFFER_DYNAMIC::PRTCL_VELOCITY}},
    },
};
}  // namespace Particle
}  // namespace Set
}  // namespace Descriptor

// PIPELINE

namespace Pipeline {

void GetFountainEulerInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    Vertex::Color::getInputDescriptions(createInfoRes);
    ::Particle::FountainEuler::DATA::getInputDescriptions(createInfoRes);
    // bindings
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = createInfoRes.bindDescs.data();
    // attributes
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = {};
    createInfoRes.inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    createInfoRes.inputAssemblyStateInfo.pNext = nullptr;
    createInfoRes.inputAssemblyStateInfo.flags = 0;
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

namespace Particle {

// WAVE
const CreateInfo WAVE_CREATE_INFO = {
    GRAPHICS::PRTCL_WAVE_DEFERRED,
    "Particle Wave Color (Deferred) Pipeline",
    {
        SHADER::PRTCL_WAVE_VERT,
        SHADER::DEFERRED_MRT_COLOR_FRAG,
    },
    {
        DESCRIPTOR_SET::UNIFORM_DEFAULT,
        DESCRIPTOR_SET::UNIFORM_PRTCL_WAVE,
    },
};
Wave::Wave(Handler& handler, bool isDeferred) : Graphics(handler, &WAVE_CREATE_INFO), IS_DEFERRED(isDeferred) {}

void Wave::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED)
        Deferred::GetBlendInfoResources(createInfoRes);
    else
        Graphics::getBlendInfoResources(createInfoRes);
}

void Wave::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    Graphics::getInputAssemblyInfoResources(createInfoRes);
    // assert(createInfoRes.inputAssemblyStateInfo.topology == VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    // createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
}

// FOUNTAIN
const CreateInfo FOUNTAIN_CREATE_INFO = {
    GRAPHICS::PRTCL_FOUNTAIN_DEFERRED,
    "Particle Fountain (Deferred) Pipeline",
    {
        SHADER::PRTCL_FOUNTAIN_VERT,
        SHADER::PRTCL_FOUNTAIN_DEFERRED_MRT_FRAG,
    },
    {
        DESCRIPTOR_SET::UNIFORM_DEFCAM_DEFMAT_MX4,
        DESCRIPTOR_SET::UNIFORM_PRTCL_FOUNTAIN,
        DESCRIPTOR_SET::SAMPLER_DEFAULT,
    },
};
Fountain::Fountain(Handler& handler, bool isDeferred)
    : Graphics(handler, &FOUNTAIN_CREATE_INFO), DO_BLEND(true), IS_DEFERRED(isDeferred) {}

void Fountain::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}

void Fountain::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    ::Particle::Fountain::DATA::getInputDescriptions(createInfoRes);
    // bindings
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = createInfoRes.bindDescs.data();
    // attributes
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
    //// divisor
    // createInfoRes.vertexInputBindDivDescs.push_back({::Instance::Obj3d::BINDING, 8000});
    // createInfoRes.vertexInputBindDivDescs.push_back({::Instance::Particle::Fountain::BINDING, 1});
    // topology
    createInfoRes.inputAssemblyStateInfo = {};
    createInfoRes.inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    createInfoRes.inputAssemblyStateInfo.pNext = nullptr;
    createInfoRes.inputAssemblyStateInfo.flags = 0;
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
}

// EULER (COMPUTE)
const Pipeline::CreateInfo EULER_CREATE_INFO = {
    COMPUTE::PRTCL_EULER,
    "Particle Euler Compute Pipeline",
    {SHADER::PRTCL_EULER_COMP},
    {
        DESCRIPTOR_SET::PRTCL_EULER,
        DESCRIPTOR_SET::UNIFORM_PRTCL_FOUNTAIN,
    },
    {},
    {PUSH_CONSTANT::PRTCL_EULER},
};
Euler::Euler(Pipeline::Handler& handler) : Compute(handler, &EULER_CREATE_INFO) {}

// FOUNTAIN EULER
const CreateInfo FOUNTAIN_EULER_CREATE_INFO = {
    GRAPHICS::PRTCL_FOUNTAIN_EULER_DEFERRED,
    "Particle Fountain Euler (Deferred) Pipeline",
    {
        SHADER::PRTCL_FOUNTAIN_EULER_VERT,
        SHADER::PRTCL_FOUNTAIN_DEFERRED_MRT_FRAG,
    },
    {
        DESCRIPTOR_SET::UNIFORM_DEFCAM_DEFMAT_MX4,
        DESCRIPTOR_SET::UNIFORM_PRTCL_FOUNTAIN,
        DESCRIPTOR_SET::SAMPLER_DEFAULT,
    },
    {},
    {PUSH_CONSTANT::PRTCL_EULER},
};
FountainEuler::FountainEuler(Handler& handler, const bool doBlend, bool isDeferred)
    : Graphics(handler, &FOUNTAIN_EULER_CREATE_INFO), DO_BLEND(doBlend), IS_DEFERRED(isDeferred) {}

void FountainEuler::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}

void FountainEuler::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    GetFountainEulerInputAssemblyInfoResources(createInfoRes);
}

// SHADOW FOUNTAIN EULER
const Pipeline::CreateInfo SHADOW_FOUNTAIN_EULER_CREATE_INFO = {
    GRAPHICS::PRTCL_SHDW_FOUNTAIN_EULER,
    "Particle Fountain Euler (Shadow) Pipeline",
    {SHADER::PRTCL_SHDW_FOUNTAIN_EULER_VERT, SHADER::SHADOW_FRAG},
    {
        DESCRIPTOR_SET::UNIFORM_DEFCAM_DEFMAT_MX4,
        DESCRIPTOR_SET::UNIFORM_PRTCL_FOUNTAIN,
    },
};
ShadowFountainEuler::ShadowFountainEuler(Pipeline::Handler& handler)
    : Graphics(handler, &SHADOW_FOUNTAIN_EULER_CREATE_INFO) {}

void ShadowFountainEuler::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    GetFountainEulerInputAssemblyInfoResources(createInfoRes);
}

void ShadowFountainEuler::getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) {
    GetShadowRasterizationStateInfoResources(createInfoRes);
}

// ATTRACTOR (COMPUTE)
const Pipeline::CreateInfo ATTR_CREATE_INFO = {
    COMPUTE::PRTCL_ATTR,
    "Particle Attractor Compute Pipeline",
    {SHADER::PRTCL_ATTR_COMP},
    {DESCRIPTOR_SET::PRTCL_ATTRACTOR},
};
AttractorCompute::AttractorCompute(Pipeline::Handler& handler) : Compute(handler, &ATTR_CREATE_INFO) {}

// ATTRACTOR (POINT)
const Pipeline::CreateInfo ATTR_PT_CREATE_INFO = {
    GRAPHICS::PRTCL_ATTR_PT_DEFERRED,
    "Particle Attractor Point (Deferred) Pipeline",
    {SHADER::PRTCL_ATTR_VERT, SHADER::DEFERRED_MRT_POINT_FRAG},
    {DESCRIPTOR_SET::UNIFORM_DEFCAM_DEFMAT_MX4},
};
AttractorPoint::AttractorPoint(Pipeline::Handler& handler, const bool doBlend, const bool isDeferred)
    : Graphics(handler, &ATTR_PT_CREATE_INFO), DO_BLEND(doBlend), IS_DEFERRED(isDeferred) {}

void AttractorPoint::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED) {
        if (DO_BLEND) assert(handler().shell().context().independentBlendEnabled);
        Deferred::GetBlendInfoResources(createInfoRes, DO_BLEND);
    } else {
        Graphics::getBlendInfoResources(createInfoRes);
    }
}

void AttractorPoint::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    Storage::Vector4::GetInputDescriptions(createInfoRes, VK_VERTEX_INPUT_RATE_INSTANCE);
    // bindings
    createInfoRes.vertexInputStateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(createInfoRes.bindDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexBindingDescriptions = createInfoRes.bindDescs.data();
    // attributes
    createInfoRes.vertexInputStateInfo.vertexAttributeDescriptionCount =
        static_cast<uint32_t>(createInfoRes.attrDescs.size());
    createInfoRes.vertexInputStateInfo.pVertexAttributeDescriptions = createInfoRes.attrDescs.data();
    // topology
    createInfoRes.inputAssemblyStateInfo = {};
    createInfoRes.inputAssemblyStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    createInfoRes.inputAssemblyStateInfo.pNext = nullptr;
    createInfoRes.inputAssemblyStateInfo.flags = 0;
    createInfoRes.inputAssemblyStateInfo.primitiveRestartEnable = VK_FALSE;
    createInfoRes.inputAssemblyStateInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
}

}  // namespace Particle
}  // namespace Pipeline
