#ifndef SCREEN_SPACE_H
#define SCREEN_SPACE_H

#include "DescriptorSet.h"
#include "Pipeline.h"
#include "Texture.h"
#include "Uniform.h"

namespace ScreenSpace {

// clang-format off
using PASS_FLAG = enum : FlagBits {
    HDR_1 =         0x00000001,
    HDR_2 =         0x00000002,
    EDGE =          0x00010000,
    BLUR_A =        0x00020000,
    BLUR_B =        0x00040000,
    BRIGHT =        0x00080000,
    BLOOM_BRIGHT =  0x00100000,
    BLOOM_BLUR_A =  0x00200000,
    BLOOM_BLUR_B =  0x00400000,
    BLOOM =         0x00800000,
};
// clang-format on

struct PushConstant {
    FlagBits flags = 0;
};

}  // namespace ScreenSpace

namespace Uniform {
namespace ScreenSpace {

struct alignas(64) DATA {
    DATA();
    float weights[10];
    float edgeThreshold;
    float luminanceThreshold;
};

class Default : public Descriptor::Base, public Buffer::DataItem<DATA> {
   public:
    Default(const Buffer::Info&& info, DATA* pData);
};

}  // namespace ScreenSpace
}  // namespace Uniform

namespace Descriptor {
namespace Set {
namespace ScreenSpace {
extern const CreateInfo DEFAULT_UNIFORM_CREATE_INFO;
extern const CreateInfo DEFAULT_SAMPLER_CREATE_INFO;
extern const CreateInfo HDR_BLIT_SAMPLER_CREATE_INFO;
extern const CreateInfo BLUR_A_SAMPLER_CREATE_INFO;
extern const CreateInfo BLUR_B_SAMPLER_CREATE_INFO;
extern const CreateInfo STORAGE_COMPUTE_POST_PROCESS_CREATE_INFO;
extern const CreateInfo STORAGE_IMAGE_COMPUTE_DEFAULT_CREATE_INFO;
}  // namespace ScreenSpace
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
struct CreateInfo;
namespace ScreenSpace {

extern const CreateInfo VERT_CREATE_INFO;
extern const CreateInfo FRAG_CREATE_INFO;
extern const CreateInfo FRAG_HDR_LOG_CREATE_INFO;
extern const CreateInfo FRAG_BRIGHT_CREATE_INFO;
extern const CreateInfo FRAG_BLUR_CREATE_INFO;
extern const CreateInfo COMP_CREATE_INFO;

}  // namespace ScreenSpace
}  // namespace Shader

namespace Texture {
struct CreateInfo;
namespace ScreenSpace {

const std::string_view DEFAULT_2D_TEXTURE_ID = "Screen Space 2D Color Texture";
extern const CreateInfo DEFAULT_2D_TEXTURE_CREATE_INFO;

const std::string_view COMPUTE_2D_TEXTURE_ID = "Screen Space Compute 2D Color Texture";
extern const CreateInfo COMPUTE_2D_TEXTURE_CREATE_INFO;

const std::string_view HDR_LOG_2D_TEXTURE_ID = "Screen Space HDR Log 2D Texture";
extern const CreateInfo HDR_LOG_2D_TEXTURE_CREATE_INFO;

const std::string_view HDR_LOG_BLIT_A_2D_TEXTURE_ID = "Screen Space HDR Log Blit A 2D Texture";
extern const CreateInfo HDR_LOG_BLIT_A_2D_TEXTURE_CREATE_INFO;
const std::string_view HDR_LOG_BLIT_B_2D_TEXTURE_ID = "Screen Space HDR Log Blit B 2D Texture";
extern const CreateInfo HDR_LOG_BLIT_B_2D_TEXTURE_CREATE_INFO;

const std::string_view BLUR_A_2D_TEXTURE_ID = "Screen Space Blur A 2D Texture";
extern const CreateInfo BLUR_A_2D_TEXTURE_CREATE_INFO;
const std::string_view BLUR_B_2D_TEXTURE_ID = "Screen Space Blur B 2D Texture";
extern const CreateInfo BLUR_B_2D_TEXTURE_CREATE_INFO;

}  // namespace ScreenSpace
}  // namespace Texture

namespace Pipeline {
struct CreateInfo;
namespace ScreenSpace {

// DEFAULT
extern const CreateInfo DEFAULT_CREATE_INFO;
class Default : public Graphics {
   public:
    Default(Handler& handler);
};

// HDR LOG
extern const CreateInfo HDR_LOG_CREATE_INFO;
class HdrLog : public Graphics {
   public:
    HdrLog(Handler& handler);
};

// BRIGHT
extern const CreateInfo BRIGHT_CREATE_INFO;
class Bright : public Graphics {
   public:
    Bright(Handler& handler);
};

// BLUR
extern const CreateInfo BLUR_CREATE_INFO;
class BlurA : public Graphics {
   public:
    BlurA(Handler& handler);
};
class BlurB : public Graphics {
   public:
    BlurB(Handler& handler);
};

// COMPUTE DEFAULT
class ComputeDefault : public Compute {
   public:
    ComputeDefault(Handler& handler);
};

}  // namespace ScreenSpace
}  // namespace Pipeline

#endif  // !SCREEN_SPACE_H
