
#ifndef SAMPLER_CONSTANTS_H
#define SAMPLER_CONSTANTS_H

enum class SAMPLER {
    //
    DEFAULT = 0,
    CUBE = 1,
};

namespace Sampler {
struct CreateInfo;

// CREATE INFOS

extern const CreateInfo STATUE_CREATE_INFO;
extern const CreateInfo VULKAN_CREATE_INFO;
extern const CreateInfo HARDWOOD_CREATE_INFO;
extern const CreateInfo MEDIEVAL_HOUSE_CREATE_INFO;
extern const CreateInfo WOOD_CREATE_INFO;
extern const CreateInfo MYBRICK_COLOR_CREATE_INFO;
extern const CreateInfo MYBRICK_NORMAL_CREATE_INFO;
extern const CreateInfo PISA_HDR_CREATE_INFO;
extern const CreateInfo SKYBOX_CREATE_INFO;

}  // namespace Sampler

#endif  // !SAMPLER_CONSTANTS_H
