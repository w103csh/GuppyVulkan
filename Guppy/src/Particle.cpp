
#include "Particle.h"

#include "ParticleBuffer.h"
#include "Deferred.h"
// HANDLERS
#include "PipelineHandler.h"

// SHADER

namespace Shader {

namespace Particle {
const CreateInfo WAVE_COLOR_VERT_DEFERRED_MRT_CREATE_INFO = {
    SHADER::WAVE_COLOR_DEFERRED_MRT_VERT,            //
    "Particle Wave Color (Deferred) Vertex Shader",  //
    "vert.color.deferred.mrt.wave.glsl",             //
    VK_SHADER_STAGE_VERTEX_BIT,                      //
};
const CreateInfo FOUNTAIN_PART_VERT_DEFERRED_MRT_CREATE_INFO = {
    SHADER::FOUNTAIN_PART_DEFERRED_MRT_VERT,       //
    "Particle Fountain (Deferred) Vertex Shader",  //
    "vert.particle.deferred.mrt.fountain.glsl",    //
    VK_SHADER_STAGE_VERTEX_BIT,                    //
};
const CreateInfo FOUNTAIN_PART_FRAG_DEFERRED_MRT_CREATE_INFO = {
    SHADER::FOUNTAIN_PART_DEFERRED_MRT_VERT,         //
    "Particle Fountain (Deferred) Fragment Shader",  //
    "frag.particle.deferred.mrt.fountain.glsl",      //
    VK_SHADER_STAGE_FRAGMENT_BIT,                    //
};

}  // namespace Particle
}  // namespace Shader

// UNIFORM

namespace Uniform {
namespace Particle {

namespace Wave {
Base::Base(const Buffer::Info&& info, DATA* pData, const Buffer::CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),  //
      Buffer::PerFramebufferDataItem<DATA>(pData) {
    setData();
}

void Base::update(const float time, const uint32_t frameIndex) {
    data_.time = time;
    setData(frameIndex);
}
}  // namespace Wave

}  // namespace Particle
}  // namespace Uniform

// MATERIAL

namespace Material {
namespace Particle {
namespace Fountain {

Base::Base(const Buffer::Info&& info, DATA* pData, const CreateInfo* pCreateInfo)
    : Buffer::Item(std::forward<const Buffer::Info>(info)),
      Material::Obj3d::Base(MATERIAL::PARTICLE_FOUNTAIN, pCreateInfo),
      Buffer::PerFramebufferDataItem<DATA>(pData),
      start_(false) {
    // Base
    data_.color = pCreateInfo->color;
    data_.flags = pCreateInfo->flags;
    data_.opacity = pCreateInfo->opacity;
    setTextureData();
    // Obj3d
    data_.model = pCreateInfo->model;
    // Fountain
    data_.acceleration = pCreateInfo->acceleration;
    data_.lifespan = pCreateInfo->lifespan;
    data_.emitterPosition = pCreateInfo->emitterPosition;
    data_.size = pCreateInfo->size;
    data_.emitterBasis = pCreateInfo->emitterBasis;
    setData();
}

void Base::start() {
    assert(!start_);
    start_ = true;
}

void Base::update(const float time, const float lastTimeOfBirth, const uint32_t frameIndex) {
    if (start_) {
        data_.time = time;
        // This should be 0.0f but it needs to be positive since its used as a flag as well.
        data_.delta = 0.0000001f;
        setData(frameIndex);
        start_ = false;
    } else if (data_.time >= 0.0f) {
        data_.delta = time - data_.time;
        if (data_.delta > (lastTimeOfBirth + data_.lifespan)) {
            data_.delta = data_.time = ::Particle::BAD_TIME;
        }
        setData(frameIndex);
    }
}

}  // namespace Fountain
}  // namespace Particle
}  // namespace Material

// DESCRIPTOR SET

namespace Descriptor {
namespace Set {
namespace Particle {

const CreateInfo WAVE_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_PARTICLE_WAVE,
    "_DS_UNI_PRTCL_WV",
    {
        {{0, 0}, {UNIFORM::PARTICLE_WAVE}},
    },
};

const CreateInfo FOUNTAIN_CREATE_INFO = {
    DESCRIPTOR_SET::UNIFORM_PARTICLE_FOUNTAIN,
    "_DS_UNI_PRTCL_FNTN",
    {
        {{0, 0}, {UNIFORM::CAMERA_PERSPECTIVE_DEFAULT}},
        {{1, 0}, {UNIFORM_DYNAMIC::MATERIAL_PARTICLE_FOUNTAIN}},
    },
};

}  // namespace Particle
}  // namespace Set
}  // namespace Descriptor

// PIPELINE

namespace Pipeline {
namespace Particle {

// WAVE
const CreateInfo WAVE_CREATE_INFO = {
    PIPELINE::PARTICLE_WAVE_DEFERRED,
    "Particle Wave Color (Deferred) Pipeline",
    {
        SHADER::WAVE_COLOR_DEFERRED_MRT_VERT,
        SHADER::DEFERRED_MRT_COLOR_FRAG,
    },
    {
        DESCRIPTOR_SET::UNIFORM_DEFAULT,
        DESCRIPTOR_SET::UNIFORM_PARTICLE_WAVE,
    },
};
Wave::Wave(Handler& handler, bool isDeferred) : Graphics(handler, &WAVE_CREATE_INFO), IS_DEFERRED(isDeferred) {}

void Wave::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED)
        Deferred::GetDefaultBlendInfoResources(createInfoRes);
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
    PIPELINE::PARTICLE_FOUNTAIN_DEFERRED,
    "Particle Fountain (Deferred) Pipeline",
    {
        SHADER::FOUNTAIN_PART_DEFERRED_MRT_VERT,
        SHADER::FOUNTAIN_PART_DEFERRED_MRT_FRAG,
    },
    {
        DESCRIPTOR_SET::UNIFORM_PARTICLE_FOUNTAIN,
        DESCRIPTOR_SET::SAMPLER_DEFAULT,
    },
};
Fountain::Fountain(Handler& handler, bool isDeferred) : Graphics(handler, &FOUNTAIN_CREATE_INFO), IS_DEFERRED(isDeferred) {}

void Fountain::getBlendInfoResources(CreateInfoResources& createInfoRes) {
    if (IS_DEFERRED)
        Deferred::GetDefaultBlendInfoResources(createInfoRes);
    else
        Graphics::getBlendInfoResources(createInfoRes);
}

void Fountain::getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) {
    createInfoRes.vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    Vertex::Color::getInputDescriptions(createInfoRes);
    Instance::Particle::Fountain::DATA::getInputDescriptions(createInfoRes);
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

}  // namespace Particle
}  // namespace Pipeline
