
#include "RenderPassConstants.h"

#include <variant>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "RenderPass.h"

namespace RenderPass {

const std::vector<RENDER_PASS> ALL = {
    RENDER_PASS::SAMPLER,
    RENDER_PASS::DEFAULT,
// UI pass needs to always be last since it
// is optional
#ifdef USE_DEBUG_UI
    RENDER_PASS::IMGUI,
#endif
};

void AddDefaultSubpasses(RenderPass::Resources& resources, uint64_t count) {
    VkSubpassDependency dependency = {};
    for (uint32_t i = 0; i < count - 1; i++) {
        dependency = {};
        dependency.srcSubpass = i;
        dependency.dstSubpass = i + 1;
        dependency.dependencyFlags = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
        dependency.dstAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        dependency.srcAccessMask =
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
        resources.dependencies.push_back(dependency);
    }
}

const CreateInfo DEFAULT_CREATE_INFO = {
    RENDER_PASS::DEFAULT,
    "Default",
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

const CreateInfo SAMPLER_CREATE_INFO = {
    RENDER_PASS::SAMPLER,
    "Sampler",
    {
        PIPELINE::TRI_LIST_COLOR,
    },
    false,
    true,
    {
        {{UNIFORM::CAMERA_PERSPECTIVE_DEFAULT, PIPELINE::ALL_ENUM}, {1}},
        {{UNIFORM::LIGHT_POSITIONAL_DEFAULT, PIPELINE::ALL_ENUM}, {0}},
    },
};

// 2D
const Sampler::CreateInfo SAMPLER_2D_CREATE_INFO = {
    "Render Pass 2D Color Sampler",  //
    {{::Sampler::USE::COLOR}},       //
    VK_IMAGE_VIEW_TYPE_2D,           //
    0,
    SAMPLER::CLAMP_TO_BORDER,
    {640, 480},
    false,
};
const Texture::CreateInfo TEXTURE_2D_CREATE_INFO = {
    "Render Pass 2D Texture",
    {SAMPLER_2D_CREATE_INFO},
    false,
};

// 2D ARRAY
const Sampler::CreateInfo SAMPLER_2D_ARRAY_CREATE_INFO = {
    "Render Pass 2D Array Color Sampler",  //
    {{::Sampler::USE::COLOR}},             //
    VK_IMAGE_VIEW_TYPE_2D_ARRAY,           //
    0,
    SAMPLER::CLAMP_TO_BORDER,
    {640, 480},
    false,
};
const Texture::CreateInfo TEXTURE_2D_ARRAY_CREATE_INFO = {
    "Render Pass 2D Array Texture",
    {SAMPLER_2D_ARRAY_CREATE_INFO},
    false,
};

}  // namespace RenderPass