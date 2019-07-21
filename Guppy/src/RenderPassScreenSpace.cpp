
#include "RenderPassScreenSpace.h"

#include "Plane.h"
#include "ScreenSpace.h"
// HANDLERS
#include "MeshHandler.h"
#include "RenderPassHandler.h"

namespace RenderPass {
namespace ScreenSpace {

// CREATE INFO

const CreateInfo CREATE_INFO = {
    PASS::SCREEN_SPACE,
    "Screen Space Swapchain Render Pass",
    {
        PIPELINE::SCREEN_SPACE_DEFAULT,
    },
    FLAG::SWAPCHAIN,
};

const CreateInfo SAMPLER_CREATE_INFO = {
    // THIS WAS BROKE LAST TIME I TRIED IT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    PASS::SAMPLER_SCREEN_SPACE,
    "Screen Space Sampler Render Pass",
    {
        PIPELINE::SCREEN_SPACE_DEFAULT,
    },
    FLAG::SWAPCHAIN,
    {std::string(Texture::ScreenSpace::DEFAULT_2D_TEXTURE_ID)},
};

// BASE

RenderPass::ScreenSpace::Base::Base(RenderPass::Handler& handler, const uint32_t&& offset, const CreateInfo* pCreateInfo)
    : RenderPass::Base{handler, std::forward<const uint32_t>(offset), pCreateInfo},
      defaultRenderPlaneIndex_(Mesh::BAD_OFFSET) {}

void RenderPass::ScreenSpace::Base::init(bool isFinal) {
    // Default render plane create info.
    Mesh::CreateInfo meshInfo = {};
    meshInfo.pipelineType = PIPELINE::SCREEN_SPACE_DEFAULT;
    meshInfo.selectable = false;
    meshInfo.geometryCreateInfo.transform = helpers::affine(glm::vec3{2.0f});
    meshInfo.geometryCreateInfo.invert = true;

    Instance::Default::CreateInfo defInstInfo = {};
    Material::Default::CreateInfo defMatInfo = {};
    defMatInfo.flags = 0;

    // Create the default render plane.
    defaultRenderPlaneIndex_ =
        handler().meshHandler().makeColorMesh<Plane::Color>(&meshInfo, &defMatInfo, &defInstInfo)->getOffset();
    assert(defaultRenderPlaneIndex_ != Mesh::BAD_OFFSET);

    RenderPass::Base::init(isFinal);
}

void RenderPass::ScreenSpace::Base::record(const uint8_t frameIndex) {
    beginInfo_.framebuffer = data.framebuffers[frameIndex];
    auto& priCmd = data.priCmds[frameIndex];

    // RESET BUFFERS
    vkResetCommandBuffer(priCmd, 0);

    // BEGIN BUFFERS
    VkCommandBufferBeginInfo bufferInfo;
    // PRIMARY
    bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bufferInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vk::assert_success(vkBeginCommandBuffer(priCmd, &bufferInfo));

    beginPass(frameIndex, VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT);

    auto& pRenderPlane = handler().meshHandler().getColorMesh(defaultRenderPlaneIndex_);
    if (pRenderPlane->getStatus() == STATUS::READY) {
        assert(pipelineTypeBindDataMap_.size());
        const auto& pipelineBindData = pipelineTypeBindDataMap_.begin()->second;

        // DRAW #1
        ::ScreenSpace::PushConstant pushConstant = {
            ::ScreenSpace::PASS_FLAG::EDGE,
            //::ScreenSpace::PASS_FLAG::BLUR_1,
            // ::ScreenSpace::PASS_FLAG::HDR_1,
        };
        vkCmdPushConstants(priCmd, pipelineBindData->layout, pipelineBindData->pushConstantStages, 0,
                           static_cast<uint32_t>(sizeof(::ScreenSpace::PushConstant)), &pushConstant);

        pRenderPlane->draw(TYPE, pipelineBindData, priCmd, frameIndex);

        if (false) {
            // DRAW #2
            pushConstant = {
                ::ScreenSpace::PASS_FLAG::BLUR_2,
                // ::ScreenSpace::PASS_FLAG::HDR_2,
            };
            vkCmdPushConstants(priCmd, pipelineBindData->layout, pipelineBindData->pushConstantStages, 0,
                               static_cast<uint32_t>(sizeof(::ScreenSpace::PushConstant)), &pushConstant);
        }
    }

    endPass(frameIndex);
    // vk::assert_success(vkEndCommandBuffer(data.priCmds[frameIndex]));
}

}  // namespace ScreenSpace
}  // namespace RenderPass
