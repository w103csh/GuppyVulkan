#ifndef SCREEN_SPACE_H
#define SCREEN_SPACE_H

#include "DescriptorSet.h"
#include "Pipeline.h"
#include "Texture.h"
#include "Uniform.h"

namespace ScreenSpace {
struct PushConstant {
    FlagBits flags;
};
}  // namespace ScreenSpace

namespace Uniform {
namespace ScreenSpace {

struct alignas(32) DATA {
    DATA();
    float weights[5];
    float edgeThreshold;
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

class DefaultUniform : public Set::Base {
   public:
    DefaultUniform(Handler& handler);
};
class DefaultSampler : public Set::Base {
   public:
    DefaultSampler(Handler& handler);
};

// COMPUTE
class StorageComputePostProcess : public Set::Base {
   public:
    StorageComputePostProcess(Handler& handler);
};
class StorageImageComputeDefault : public Set::Base {
   public:
    StorageImageComputeDefault(Handler& handler);
};

}  // namespace ScreenSpace
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
struct CreateInfo;
namespace ScreenSpace {

extern const CreateInfo VERT_CREATE_INFO;
extern const CreateInfo FRAG_CREATE_INFO;
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

// COMPUTE DEFAULT
extern const CreateInfo COMPUTE_DEFAULT_CREATE_INFO;
class ComputeDefault : public Compute {
   public:
    ComputeDefault(Handler& handler);
};

}  // namespace ScreenSpace
}  // namespace Pipeline

namespace RenderPass {

struct CreateInfo;

namespace ScreenSpace {

extern const CreateInfo CREATE_INFO;
extern const CreateInfo SAMPLER_CREATE_INFO;  // TODO: Add sampler offset overrides.

}  // namespace ScreenSpace
}  // namespace RenderPass

#endif  // !SCREEN_SPACE_H
