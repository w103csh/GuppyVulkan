#ifndef DEFERRED_H
#define DEFERRED_H

#include "ConstantsAll.h"
#include "Pipeline.h"
#include "Random.h"

namespace Texture {
struct CreateInfo;
namespace Deferred {

const std::string_view POS_NORM_2D_ARRAY_ID = "Deferred 2D Array Position/Normal Texture";
extern const CreateInfo POS_NORM_2D_ARRAY_CREATE_INFO;

const std::string_view POS_2D_ID = "Deferred 2D Position Texture";
extern const CreateInfo POS_2D_CREATE_INFO;

const std::string_view NORM_2D_ID = "Deferred 2D Normal Texture";
extern const CreateInfo NORM_2D_CREATE_INFO;

const std::string_view DIFFUSE_2D_ID = "Deferred 2D Diffuse Texture";
extern const CreateInfo DIFFUSE_2D_CREATE_INFO;

const std::string_view AMBIENT_2D_ID = "Deferred 2D Ambient Texture";
extern const CreateInfo AMBIENT_2D_CREATE_INFO;

const std::string_view SPECULAR_2D_ID = "Deferred 2D Specular Texture";
extern const CreateInfo SPECULAR_2D_CREATE_INFO;

const std::string_view SSAO_2D_ID = "Deferred 2D SSAO Texture";
extern const CreateInfo SSAO_2D_CREATE_INFO;

const std::string_view SSAO_RAND_2D_ID = "Deferred 2D SSAO Random Texture";
CreateInfo MakeSSAORandRotationTex(Random& rand);

}  // namespace Deferred
}  // namespace Texture

namespace Uniform {
namespace Deferred {

constexpr uint32_t KERNEL_SIZE = 64;

struct alignas(256) DATA {
    float kern[KERNEL_SIZE] = {};
};

class SSAO : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    SSAO(const Buffer::Info&& info, DATA* pData);

    void init(Random& rand);
};

}  // namespace Deferred
}  // namespace Uniform

namespace Descriptor {
namespace Set {
namespace Deferred {

extern const CreateInfo MRT_UNIFORM_CREATE_INFO;
extern const CreateInfo SSAO_UNIFORM_CREATE_INFO;
extern const CreateInfo COMBINE_UNIFORM_CREATE_INFO;
extern const CreateInfo POS_NORM_SAMPLER_CREATE_INFO;
extern const CreateInfo POS_SAMPLER_CREATE_INFO;
extern const CreateInfo NORM_SAMPLER_CREATE_INFO;
extern const CreateInfo DIFFUSE_SAMPLER_CREATE_INFO;
extern const CreateInfo AMBIENT_SAMPLER_CREATE_INFO;
extern const CreateInfo SPECULAR_SAMPLER_CREATE_INFO;
extern const CreateInfo SSAO_SAMPLER_CREATE_INFO;
extern const CreateInfo SSAO_RANDOM_SAMPLER_CREATE_INFO;

}  // namespace Deferred
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
struct CreateInfo;
namespace Deferred {

extern const CreateInfo VERT_CREATE_INFO;
extern const CreateInfo FRAG_CREATE_INFO;
extern const CreateInfo MRT_TEX_WS_VERT_CREATE_INFO;
extern const CreateInfo MRT_TEX_CS_VERT_CREATE_INFO;
extern const CreateInfo MRT_TEX_FRAG_CREATE_INFO;
extern const CreateInfo MRT_COLOR_CS_VERT_CREATE_INFO;
extern const CreateInfo MRT_COLOR_FRAG_CREATE_INFO;
extern const CreateInfo SSAO_FRAG_CREATE_INFO;

}  // namespace Deferred
}  // namespace Shader

namespace Pipeline {
struct CreateInfo;
class Handler;
namespace Deferred {

// MRT (TEXTURE)
class MRTTexture : public Graphics {
   public:
    MRTTexture(Handler& handler);
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
};

// MRT (COLOR)
class MRTColor : public Graphics {
   public:
    MRTColor(Handler& handler);
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
};

// MRT (LINE)
class MRTLine : public Graphics {
   public:
    MRTLine(Handler& handler);
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes);
};

// SSAO
class SSAO : public Graphics {
   public:
    SSAO(Handler& handler);
};

// COMBINE
class Combine : public Graphics {
   public:
    Combine(Handler& handler);
};

}  // namespace Deferred
}  // namespace Pipeline

#endif  // !DEFERRED_H
