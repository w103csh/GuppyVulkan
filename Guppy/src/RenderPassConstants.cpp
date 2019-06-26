
#include "RenderPassConstants.h"

#include <variant>
#include <vulkan/vulkan.h>

#include "ConstantsAll.h"
#include "RenderPass.h"

#define RENDER_PASS_CONSTANTS

namespace RenderPass {

// clang-format off
const std::vector<RENDER_PASS> ALL = {
    RENDER_PASS::PROJECT,
    //RENDER_PASS::DEFAULT,
    RENDER_PASS::SAMPLER_DEFAULT,
    //RENDER_PASS::SCREEN_SPACE_SAMPLER,
    RENDER_PASS::SCREEN_SPACE,
// UI pass needs to always be last since it
// is optional
#ifdef USE_DEBUG_UI
    RENDER_PASS::IMGUI,
#endif
};
// clang-format on

// SAMPLER
const Sampler::CreateInfo DEFAULT_2D_SAMPLER_CREATE_INFO = {
    "Render Pass Default 2D Color Sampler",  //
    {{::Sampler::USAGE::COLOR}},             //
    VK_IMAGE_VIEW_TYPE_2D,                   //
    0,
    SAMPLER::CLAMP_TO_BORDER,
    BAD_EXTENT_2D,
    false,
};
const Sampler::CreateInfo PROJECT_2D_SAMPLER_CREATE_INFO = {
    "Render Pass Project 2D Color Sampler",  //
    {{::Sampler::USAGE::COLOR}},             //
    VK_IMAGE_VIEW_TYPE_2D,                   //
    0,
    SAMPLER::CLAMP_TO_BORDER,
    {640, 480},
    false,
};
const Sampler::CreateInfo PROJECT_2D_ARRAY_SAMPLER_CREATE_INFO = {
    "Render Pass Project 2D Array Color Sampler",  //
    {{::Sampler::USAGE::COLOR}},                   //
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,                   //
    0,
    SAMPLER::CLAMP_TO_BORDER,
    {1024, 768},
    false,
};

// TEXTURE
const Texture::CreateInfo DEFAULT_2D_TEXTURE_CREATE_INFO = {
    std::string(DEFAULT_2D_TEXTURE_ID),
    {DEFAULT_2D_SAMPLER_CREATE_INFO},
    false,
};
const Texture::CreateInfo PROJECT_2D_TEXTURE_CREATE_INFO = {
    std::string(PROJECT_2D_TEXTURE_ID),
    {PROJECT_2D_SAMPLER_CREATE_INFO},
    false,
};
const Texture::CreateInfo PROJECT_2D_ARRAY_TEXTURE_CREATE_INFO = {
    std::string(PROJECT_2D_ARRAY_TEXTURE_ID),
    {PROJECT_2D_ARRAY_SAMPLER_CREATE_INFO},
    false,
};

// DEFAULT
const CreateInfo DEFAULT_CREATE_INFO = {
    RENDER_PASS::DEFAULT,
    "Default Render Pass",
    {
        // Order of the subpasses
        PIPELINE::TRI_LIST_COLOR,  //
        PIPELINE::PBR_COLOR,
        PIPELINE::LINE,
        PIPELINE::TRI_LIST_TEX,
        PIPELINE::PARALLAX_SIMPLE,
        PIPELINE::PARALLAX_STEEP,
        /* This needs to come second becuase it has transparent textures.
         *  It looks to be like its blending where the transparent edges meet
         *  the fragment behind it. In the case I was seeing when BP_TEX_CULL_NONE
         *  came before the ground plane pass it was blending the black clear color.
         */
        PIPELINE::BP_TEX_CULL_NONE,
        PIPELINE::PBR_TEX,
        /* TODO: come up with a clever way to render the skybox last.
         *   Some of the logic in cube.vert requires the skybox to be rendered
         *   last in order for it to be efficient (This is why gl_Position takes
         *   .xyww swizzle so that it defaults the z value to 1.0 in depth tests).
         */
        PIPELINE::CUBE,
    },
};

// SAMPLER DEFAULT
const CreateInfo SAMPLER_DEFAULT_CREATE_INFO = {
    RENDER_PASS::SAMPLER_DEFAULT,
    "Sampler Default Render Pass",
    {
        // Order of the subpasses
        PIPELINE::TRI_LIST_COLOR,  //
        PIPELINE::PBR_COLOR,
        PIPELINE::LINE,
        PIPELINE::TRI_LIST_TEX,
        PIPELINE::PARALLAX_SIMPLE,
        PIPELINE::PARALLAX_STEEP,
        PIPELINE::BP_TEX_CULL_NONE,
        PIPELINE::PBR_TEX,
        PIPELINE::CUBE,
    },
    (FLAG::SWAPCHAIN | FLAG::DEPTH | FLAG::MULTISAMPLE),
    {std::string(DEFAULT_2D_TEXTURE_ID)},
};

// PROJECT
const CreateInfo PROJECT_CREATE_INFO = {
    RENDER_PASS::PROJECT,
    "Project Render Pass",
    {
        PIPELINE::TRI_LIST_COLOR,
    },
    FLAG::DEPTH,
    {PROJECT_2D_ARRAY_TEXTURE_CREATE_INFO.name},
    {
        {{UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, PIPELINE::ALL_ENUM}, {1}},
        //{{UNIFORM::LIGHT_SPOT_DEFAULT, PIPELINE::ALL_ENUM}, {1}},
    },
};

}  // namespace RenderPass