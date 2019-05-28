
#include "RenderPassConstants.h"

#include <vulkan/vulkan.h>

#include "Constants.h"

namespace RenderPass {

void AddDefaultSubpasses(RenderPass::SubpassResources& resources, uint64_t count) {
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
    {
        // Order of the subpasses
        PIPELINE::TRI_LIST_COLOR,  //
        PIPELINE::PBR_COLOR,
        PIPELINE::LINE,
        PIPELINE::TRI_LIST_TEX,
        PIPELINE::PARALLAX_SIMPLE,
        PIPELINE::PARALLAX_STEEP,
        // This needs to come second becuase it has transparent textures.
        // It looks to be like its blending where the transparent edges meet
        // the fragment behind it. In the case I was seeing when BP_TEX_CULL_NONE
        // came before the ground plane pass it was blending the black clear color.
        PIPELINE::BP_TEX_CULL_NONE,
        PIPELINE::PBR_TEX,
        /* TODO: come up with a clever way to render the skybox last.
         *   Some of the logic in cube.vert requires the skybox to be rendered
         *   last in order for it to be efficient (This is why gl_Position takes
         *   .xyww swizzle so that it defaults the z value to 1.0 in depth tests).
         */
        PIPELINE::CUBE,
    },
    true,
    true,
};

const CreateInfo SAMPLER_CREATE_INFO = {
    //
};

}  // namespace RenderPass