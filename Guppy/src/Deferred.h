/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef DEFERRED_H
#define DEFERRED_H

#include "ConstantsAll.h"
#include "Pipeline.h"

namespace Deferred {

constexpr bool DO_MSAA = true;

// clang-format off
using PASS_FLAG = enum : FlagBits {
    NONE =              0x00000000,
    DONT_SHADE =        0x00000001,
    WIREFRAME =         0x00000002,
};
// clang-format on

struct PushConstant {
    FlagBits flags = 0;
};

}  // namespace Deferred

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

const std::string_view FLAGS_2D_ID = "Deferred 2D Flags Texture";
extern const CreateInfo FLAGS_2D_CREATE_INFO;

const std::string_view SSAO_2D_ID = "Deferred 2D SSAO Texture";
extern const CreateInfo SSAO_2D_CREATE_INFO;

const std::string_view SSAO_RAND_2D_ID = "Deferred 2D SSAO Random Texture";
CreateInfo MakeSSAORandRotationTex();

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

    void init();
};

}  // namespace Deferred
}  // namespace Uniform

namespace Descriptor {
namespace Set {
namespace Deferred {

extern const CreateInfo MRT_UNIFORM_CREATE_INFO;
extern const CreateInfo COMBINE_UNIFORM_CREATE_INFO;
extern const CreateInfo SSAO_UNIFORM_CREATE_INFO;
extern const CreateInfo SAMPLER_CREATE_INFO;
extern const CreateInfo SSAO_RANDOM_SAMPLER_CREATE_INFO;

}  // namespace Deferred
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
struct CreateInfo;
namespace Deferred {

extern const CreateInfo VERT_CREATE_INFO;
extern const CreateInfo FRAG_CREATE_INFO;
extern const CreateInfo FRAG_MS_CREATE_INFO;
extern const CreateInfo MRT_TEX_WS_VERT_CREATE_INFO;
extern const CreateInfo MRT_TEX_CS_VERT_CREATE_INFO;
extern const CreateInfo MRT_TEX_FRAG_CREATE_INFO;
extern const CreateInfo MRT_COLOR_CS_VERT_CREATE_INFO;
extern const CreateInfo MRT_PT_CS_VERT_CREATE_INFO;
extern const CreateInfo MRT_COLOR_FRAG_CREATE_INFO;
extern const CreateInfo MTR_POINT_FRAG_CREATE_INFO;
extern const CreateInfo MTR_COLOR_REF_REF_FRAG_CREATE_INFO;
extern const CreateInfo SSAO_FRAG_CREATE_INFO;

}  // namespace Deferred
}  // namespace Shader

namespace Pipeline {
struct CreateInfo;
class Handler;
namespace Deferred {

void GetBlendInfoResources(CreateInfoResources& createInfoRes, bool blend = false);

// MRT (TEXTURE)
class MRTTexture : public Graphics {
   public:
    MRTTexture(Handler& handler);

   private:
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;

    // protected:
    // MRTTexture(Handler& handler, const CreateInfo& pCreateInfo);
};

//// MRT (TEXTURE WIREFRAME)
// class MRTTextureWireframe : public MRTTexture {
//   public:
//    MRTTextureWireframe(Handler& handler);
//    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
//};

// MRT (COLOR)
class MRTColor : public Graphics {
   public:
    MRTColor(Handler& handler);

   protected:
    MRTColor(Handler& handler, const CreateInfo* pCreateInfo);

   private:
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
};

// MRT (COLOR WIREFRAME)
class MRTColorWireframe : public MRTColor {
   public:
    MRTColorWireframe(Handler& handler);

   private:
    void getRasterizationStateInfoResources(CreateInfoResources& createInfoRes) override;
};

// MRT (POINT)
class MRTPoint : public Graphics {
   public:
    MRTPoint(Handler& handler);

   private:
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
};

// MRT (LINE)
class MRTLine : public Graphics {
   public:
    MRTLine(Handler& handler);

   private:
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
    void getInputAssemblyInfoResources(CreateInfoResources& createInfoRes) override;
};

// MRT (COLOR REFLECT REFRACT)
class MRTColorReflectRefract : public Graphics {
   public:
    MRTColorReflectRefract(Handler& handler);

   private:
    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
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

   private:
    void init() override;
    void getShaderStageInfoResources(CreateInfoResources& createInfoRes) override;

   private:
    bool doMSAA_;
};

}  // namespace Deferred
}  // namespace Pipeline

#endif  // !DEFERRED_H
