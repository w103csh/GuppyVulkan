/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "RenderPassConstants.h"

#include <variant>
#include <vulkan/vulkan.hpp>

#include "ConstantsAll.h"
#include "RenderPass.h"
#include "Sampler.h"

#define PASS_CONSTANTS

namespace RenderPass {

// clang-format off

const std::set<RENDER_PASS> ALL = {
    RENDER_PASS::DEFAULT,
    RENDER_PASS::SAMPLER_DEFAULT,
    RENDER_PASS::SAMPLER_PROJECT,
    RENDER_PASS::SCREEN_SPACE,
    RENDER_PASS::SCREEN_SPACE_HDR_LOG,
    RENDER_PASS::SCREEN_SPACE_BRIGHT,
    RENDER_PASS::SCREEN_SPACE_BLUR_A,
    RENDER_PASS::SCREEN_SPACE_BLUR_B,
    RENDER_PASS::DEFERRED,
    RENDER_PASS::SHADOW,
    RENDER_PASS::SKYBOX_NIGHT,
#ifdef USE_DEBUG_UI
    RENDER_PASS::IMGUI,
#endif
};

// clang-format on

// SAMPLER
const Sampler::CreateInfo DEFAULT_2D_SAMPLER_CREATE_INFO = {
    "Render Pass Default 2D Color Sampler",
    {{{::Sampler::USAGE::COLOR}}},
    vk::ImageViewType::e2D,
    BAD_EXTENT_3D,
    {false, true},
    {},
    SAMPLER::CLAMP_TO_BORDER,
    (vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment),
    {{false, false}, 1},
    vk::Format::eR32G32B32A32Sfloat,  // vk::Format::eR8G8B8A8Unorm,
};

const Sampler::CreateInfo PROJECT_2D_SAMPLER_CREATE_INFO = {
    "Render Pass Project 2D Color Sampler",
    {{{::Sampler::USAGE::COLOR}}},
    vk::ImageViewType::e2D,
    {640, 480, 1},
    {},
    {},
    SAMPLER::CLAMP_TO_BORDER,
    (vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment),
    {{false, false}, 1},
};
const Sampler::CreateInfo PROJECT_2D_ARRAY_SAMPLER_CREATE_INFO = {
    "Render Pass Project 2D Array Color Sampler",
    {{{::Sampler::USAGE::COLOR}}},
    vk::ImageViewType::e2DArray,
    {1024, 768, 1},
    {},
    {},
    SAMPLER::CLAMP_TO_BORDER,
    (vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eColorAttachment),
    {{false, false}, 1},
};
const Sampler::CreateInfo SWAPCHAIN_TARGET_SAMPLER_CREATE_INFO = {
    "Swapchain Target Sampler",     //
    {{{::Sampler::USAGE::COLOR}}},  //
    vk::ImageViewType::e2D,         //
    BAD_EXTENT_3D,
    {true, true},
    {},
    SAMPLER::CLAMP_TO_BORDER,
    vk::ImageUsageFlagBits::eSampled,
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
    RENDER_PASS::DEFAULT,
    "Default Render Pass",
    {
        // Order of the subpasses
        GRAPHICS::TRI_LIST_COLOR,  //
        GRAPHICS::PBR_COLOR,
        GRAPHICS::LINE,
        GRAPHICS::TRI_LIST_TEX,
        GRAPHICS::PARALLAX_SIMPLE,
        GRAPHICS::PARALLAX_STEEP,
        /* This needs to come second becuase it has transparent textures.
         *  It looks to be like its blending where the transparent edges meet
         *  the fragment behind it. In the case I was seeing when BP_TEX_CULL_NONE
         *  came before the ground plane pass it was blending the black clear color.
         */
        GRAPHICS::BP_TEX_CULL_NONE,
        GRAPHICS::PBR_TEX,
        /* TODO: come up with a clever way to render the skybox last.
         *   Some of the logic in cube.vert requires the skybox to be rendered
         *   last in order for it to be efficient (This is why gl_Position takes
         *   .xyww swizzle so that it defaults the z value to 1.0 in depth tests).
         */
        GRAPHICS::CUBE,
    },
};

// SAMPLER DEFAULT
const CreateInfo SAMPLER_DEFAULT_CREATE_INFO = {
    RENDER_PASS::SAMPLER_DEFAULT,
    "Sampler Default Render Pass",
    {
        // Order of the subpasses
        GRAPHICS::TRI_LIST_COLOR,  //
        GRAPHICS::PBR_COLOR,
        GRAPHICS::LINE,
        GRAPHICS::TRI_LIST_TEX,
        GRAPHICS::PARALLAX_SIMPLE,
        GRAPHICS::PARALLAX_STEEP,
        GRAPHICS::BP_TEX_CULL_NONE,
        GRAPHICS::PBR_TEX,
        GRAPHICS::CUBE,
    },
    (FLAG::SWAPCHAIN | FLAG::DEPTH | FLAG::MULTISAMPLE),
    {std::string(DEFAULT_2D_TEXTURE_ID)},
    {},
    {},
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageLayout::eShaderReadOnlyOptimal,
};

// PROJECT
const CreateInfo PROJECT_CREATE_INFO = {
    RENDER_PASS::SAMPLER_PROJECT,
    "Project Render Pass",
    {
        GRAPHICS::TRI_LIST_COLOR,
    },
    FLAG::DEPTH,
    {std::string(PROJECT_2D_ARRAY_TEXTURE_ID)},
    {},
    {},
    vk::ImageLayout::eColorAttachmentOptimal,
    vk::ImageLayout::eShaderReadOnlyOptimal,
    {
        {{UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, GRAPHICS::ALL_ENUM}, {1}},
        //{{UNIFORM::LIGHT_SPOT_DEFAULT, GRAPHICS::ALL_ENUM}, {1}},
    },
};

}  // namespace RenderPass

namespace Descriptor {
namespace Set {
namespace RenderPass {
const CreateInfo SWAPCHAIN_IMAGE_CREATE_INFO = {
    DESCRIPTOR_SET::SWAPCHAIN_IMAGE,
    "_DS_SWAPCHAIN_IMAGE",
    {{{0, 0}, {STORAGE_IMAGE::SWAPCHAIN, ::RenderPass::SWAPCHAIN_TARGET_ID}}},
};
}  // namespace RenderPass
}  // namespace Set
}  // namespace Descriptor