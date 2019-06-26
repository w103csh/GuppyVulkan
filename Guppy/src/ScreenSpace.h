#ifndef SCREEN_SPACE_H
#define SCREEN_SPACE_H

#include "DescriptorSet.h"
#include "Pipeline.h"
#include "Texture.h"
#include "Uniform.h"

namespace Uniform {
namespace ScreenSpace {
struct alignas(32) DATA {
    DATA();
    float weights[5];
    float edgeThreshold;
};
class Default : public Uniform::Base, public Buffer::DataItem<DATA> {
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
    DefaultUniform();
};
class DefaultSampler : public Set::Base {
   public:
    DefaultSampler();
};
}  // namespace ScreenSpace
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
struct CreateInfo;
namespace ScreenSpace {
extern const CreateInfo VERT_CREATE_INFO;
extern const CreateInfo FRAG_CREATE_INFO;
}  // namespace ScreenSpace
}  // namespace Shader

namespace Pipeline {
struct CreateInfo;
namespace ScreenSpace {
extern const CreateInfo DEFAULT_CREATE_INFO;
class Default : public Pipeline::Base {
   public:
    Default(Handler& handler);
};
}  // namespace ScreenSpace
}  // namespace Pipeline

namespace Texture {
struct CreateInfo;
namespace ScreenSpace {
const std::string_view DEFAULT_2D_TEXTURE_ID = "Screen Space 2D Color Texture";
extern const CreateInfo DEFAULT_2D_TEXTURE_CREATE_INFO;
}  // namespace ScreenSpace
}  // namespace Texture

namespace RenderPass {
struct CreateInfo;
namespace ScreenSpace {
extern const CreateInfo CREATE_INFO;
// extern const CreateInfo SAMPLER_CREATE_INFO; // TODO: Add sampler offset overrides.
}  // namespace ScreenSpace
}  // namespace RenderPass

#endif  // !SCREEN_SPACE_H
