#ifndef DEFERRED_H
#define DEFERRED_H

#include "ConstantsAll.h"
#include "Pipeline.h"

namespace Texture {
struct CreateInfo;
namespace Deferred {

const std::string_view POS_NORM_2D_ARRAY_ID = "Deferred 2D Array Position/Normal Texture";
extern const CreateInfo POS_NORM_2D_ARRAY_CREATE_INFO;

const std::string_view POS_2D_ID = "Deferred 2D Position Texture";
extern const CreateInfo POS_2D_CREATE_INFO;

const std::string_view NORM_2D_ID = "Deferred 2D Normal Texture";
extern const CreateInfo NORM_2D_CREATE_INFO;

const std::string_view COLOR_2D_ID = "Deferred 2D Color Texture";
extern const CreateInfo COLOR_2D_CREATE_INFO;

}  // namespace Deferred
}  // namespace Texture

namespace Descriptor {
namespace Set {
namespace Deferred {

extern const CreateInfo MRT_UNIFORM_CREATE_INFO;
extern const CreateInfo COMBINE_UNIFORM_CREATE_INFO;
extern const CreateInfo POS_NORM_SAMPLER_CREATE_INFO;
extern const CreateInfo POS_SAMPLER_CREATE_INFO;
extern const CreateInfo NORM_SAMPLER_CREATE_INFO;
extern const CreateInfo COLOR_SAMPLER_CREATE_INFO;

}  // namespace Deferred
}  // namespace Set
}  // namespace Descriptor

namespace Shader {
struct CreateInfo;
namespace Deferred {

extern const CreateInfo VERT_CREATE_INFO;
extern const CreateInfo FRAG_CREATE_INFO;
extern const CreateInfo MRT_VERT_CREATE_INFO;
extern const CreateInfo MRT_FRAG_CREATE_INFO;

}  // namespace Deferred
}  // namespace Shader

namespace Pipeline {
struct CreateInfo;
class Handler;
namespace Deferred {

// MRT
class MRT : public Graphics {
   public:
    MRT(Handler& handler);

    void getBlendInfoResources(CreateInfoResources& createInfoRes) override;
};

// COMBINE
class Combine : public Graphics {
   public:
    Combine(Handler& handler);
};

}  // namespace Deferred
}  // namespace Pipeline

#endif  // !DEFERRED_H
