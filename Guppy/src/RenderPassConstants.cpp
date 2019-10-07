
#include "RenderPassConstants.h"

#include <variant>
#include <vulkan/vulkan.h>

#include "ConstantsAll.h"
#include "RenderPass.h"
#include "Sampler.h"

#define PASS_CONSTANTS

namespace RenderPass {

// clang-format off

const std::set<PASS> ALL = {
    PASS::DEFAULT,
    PASS::SAMPLER_DEFAULT,
    PASS::SAMPLER_PROJECT,
    PASS::SCREEN_SPACE,
    PASS::SCREEN_SPACE_HDR_LOG,
    PASS::SCREEN_SPACE_BRIGHT,
    PASS::SCREEN_SPACE_BLUR_A,
    PASS::SCREEN_SPACE_BLUR_B,
    PASS::DEFERRED,
    PASS::SHADOW,
#ifdef USE_DEBUG_UI
    PASS::IMGUI,
#endif
};

// clang-format on

// SAMPLER
const Sampler::CreateInfo DEFAULT_2D_SAMPLER_CREATE_INFO = {
    "Render Pass Default 2D Color Sampler",
    {{{::Sampler::USAGE::COLOR}}},
    VK_IMAGE_VIEW_TYPE_2D,
    BAD_EXTENT_3D,
    {false, true},
    0,
    SAMPLER::CLAMP_TO_BORDER,
    (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
    {{false, false}, 1},
    VK_FORMAT_R32G32B32A32_SFLOAT,  // VK_FORMAT_R8G8B8A8_UNORM,
};

const Sampler::CreateInfo PROJECT_2D_SAMPLER_CREATE_INFO = {
    "Render Pass Project 2D Color Sampler",
    {{{::Sampler::USAGE::COLOR}}},
    VK_IMAGE_VIEW_TYPE_2D,
    {640, 480, 1},
    {},
    0,
    SAMPLER::CLAMP_TO_BORDER,
    (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
    {{false, false}, 1},
};
const Sampler::CreateInfo PROJECT_2D_ARRAY_SAMPLER_CREATE_INFO = {
    "Render Pass Project 2D Array Color Sampler",
    {{{::Sampler::USAGE::COLOR}}},
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,
    {1024, 768, 1},
    {},
    0,
    SAMPLER::CLAMP_TO_BORDER,
    (VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
    {{false, false}, 1},
};
const Sampler::CreateInfo SWAPCHAIN_TARGET_SAMPLER_CREATE_INFO = {
    "Swapchain Target Sampler",     //
    {{{::Sampler::USAGE::COLOR}}},  //
    VK_IMAGE_VIEW_TYPE_2D,          //
    BAD_EXTENT_3D,
    {true, true},
    0,
    SAMPLER::CLAMP_TO_BORDER,
    VK_IMAGE_USAGE_SAMPLED_BIT,
    {{false, false}, 1},
};

// TEXTURE
const Texture::CreateInfo DEFAULT_2D_TEXTURE_CREATE_INFO = {
    std::string(DEFAULT_2D_TEXTURE_ID),
    {DEFAULT_2D_SAMPLER_CREATE_INFO},
    false,
    false,  // TODO: Not sure if this actually helps. Need to test subpass dependencies. I also think this might be affected
            // by depth/resolve not being per framebuffer.
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
const Texture::CreateInfo SWAPCHAIN_TARGET_TEXTURE_CREATE_INFO = {
    std::string(SWAPCHAIN_TARGET_ID),        //
    {SWAPCHAIN_TARGET_SAMPLER_CREATE_INFO},  //
    false,
    true,
    STORAGE_IMAGE::SWAPCHAIN,
};

// DEFAULT
const CreateInfo DEFAULT_CREATE_INFO = {
    PASS::DEFAULT,
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
    PASS::SAMPLER_DEFAULT,
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
    {},
    {},
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
};

// PROJECT
const CreateInfo PROJECT_CREATE_INFO = {
    PASS::SAMPLER_PROJECT,
    "Project Render Pass",
    {
        PIPELINE::TRI_LIST_COLOR,
    },
    FLAG::DEPTH,
    {std::string(PROJECT_2D_ARRAY_TEXTURE_ID)},
    {},
    {},
    VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    {
        {{UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, PIPELINE::ALL_ENUM}, {1}},
        //{{UNIFORM::LIGHT_SPOT_DEFAULT, PIPELINE::ALL_ENUM}, {1}},
    },
};

}  // namespace RenderPass

namespace Descriptor {
namespace Set {
namespace RenderPass {
const CreateInfo SWAPCHAIN_IMAGE_CREATE_INFO = {
    DESCRIPTOR_SET::SWAPCHAIN_IMAGE,
    "_DS_SWAPCHAIN_IMAGE",
    {
        {{0, 0}, {STORAGE_IMAGE::SWAPCHAIN, ::RenderPass::SWAPCHAIN_TARGET_ID}},
    },
};
}  // namespace RenderPass
}  // namespace Set
}  // namespace Descriptor