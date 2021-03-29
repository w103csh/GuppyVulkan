/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef RENDER_PASS_CONSTANTS_H
#define RENDER_PASS_CONSTANTS_H

#include <array>
#include <glm/glm.hpp>
#include <list>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

#include <Common/Types.h>

#include "DescriptorConstants.h"
#include "Enum.h"
#include "PassConstants.h"

enum class DESCRIPTOR_SET;
enum class SHADER;
// clang-format off
namespace Sampler { struct CreateInfo; }
namespace Texture { struct CreateInfo; }
// clang-format on

// const glm::vec4 CLEAR_COLOR = {190.0f / 256.0f, 223.0f / 256.0f, 246.0f / 256.0f, 1.0f};
// const glm::vec4 CLEAR_COLOR = {0.69f, 0.84f, 1.0f, 1.0f};
const glm::vec4 CLEAR_COLOR = {};
const vk::ClearColorValue DEFAULT_CLEAR_COLOR_VALUE =
    std::array<float, 4>{CLEAR_COLOR.x, CLEAR_COLOR.y, CLEAR_COLOR.z, CLEAR_COLOR.w};

const vk::ClearDepthStencilValue DEFAULT_CLEAR_DEPTH_STENCIL_VALUE = {1.0f, 0};

namespace RenderPass {

using index = uint32_t;

constexpr uint8_t RESOURCE_SIZE = 20;
constexpr index BAD_OFFSET = UINT32_MAX;

using SubmitResource = ::SubmitResource<RESOURCE_SIZE>;
using SubmitResources = std::array<SubmitResource, RESOURCE_SIZE>;

using descriptorPipelineOffsetsMap = std::map<Uniform::offsetsMapKey, Uniform::offsets>;

using orderedPassTypeOffsetPairs = insertion_ordered_unique_list<std::pair<RENDER_PASS, index>>;

extern const std::set<RENDER_PASS> ALL;

// clang-format off
using FLAG = enum : FlagBits {
    NONE =                      0x00000000,
    SWAPCHAIN =                 0x00000001,
    DEPTH =                     0x00000002,
    MULTISAMPLE =               0x00000004,
    SECONDARY_COMMANDS =        0x00000008,
    DEPTH_INPUT_ATTACHMENT =    0x00000010,
};
// clang-format off

struct SwapchainResources {
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> views;
};

struct Data {
    std::vector<vk::Framebuffer> framebuffers;
    std::vector<vk::CommandBuffer> priCmds;
    std::vector<vk::CommandBuffer> secCmds;
    std::vector<vk::Semaphore> semaphores;
    vk::PipelineStageFlags signalSrcStageMask;
};

struct Resources {
    // storage
    std::vector<vk::AttachmentReference> inputAttachments;
    std::vector<vk::AttachmentReference> colorAttachments;
    std::vector<vk::AttachmentReference> resolveAttachments;
    vk::AttachmentReference depthStencilAttachment = {};
    std::vector<uint32_t> preserveAttachments;
    // render pass create info
    std::vector<vk::SubpassDescription> subpasses;
    std::vector<vk::AttachmentDescription> attachments;
    std::vector<vk::SubpassDependency> dependencies;
};

struct CreateInfo {
    RENDER_PASS type;
    std::string name;
    std::list<PIPELINE> pipelineTypes;
    FlagBits flags = (FLAG::SWAPCHAIN | FLAG::DEPTH | FLAG::MULTISAMPLE);
    std::vector<std::string> textureIds;
    std::vector<RENDER_PASS> prePassTypes;
    std::vector<RENDER_PASS> postPassTypes;
    vk::ImageLayout initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
    vk::ImageLayout finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
    descriptorPipelineOffsetsMap descPipelineOffsets;
};

// TEXTURE
constexpr std::string_view SWAPCHAIN_TARGET_ID = "Swapchain Target Texture";
extern const Texture::CreateInfo SWAPCHAIN_TARGET_TEXTURE_CREATE_INFO;
constexpr std::string_view DEFAULT_2D_TEXTURE_ID = "Render Pass Default 2D Texture";
extern const Texture::CreateInfo DEFAULT_2D_TEXTURE_CREATE_INFO;
constexpr std::string_view PROJECT_2D_TEXTURE_ID = "Render Pass Project 2D Texture";
extern const Texture::CreateInfo PROJECT_2D_TEXTURE_CREATE_INFO;
constexpr std::string_view PROJECT_2D_ARRAY_TEXTURE_ID = "Render Pass Project 2D Array Texture";
extern const Texture::CreateInfo PROJECT_2D_ARRAY_TEXTURE_CREATE_INFO;

// RENDER PASS
extern const CreateInfo DEFAULT_CREATE_INFO;
extern const CreateInfo SAMPLER_DEFAULT_CREATE_INFO;
extern const CreateInfo PROJECT_CREATE_INFO;

}  // namespace RenderPass

namespace Descriptor {
namespace Set {
namespace RenderPass {
extern const CreateInfo SWAPCHAIN_IMAGE_CREATE_INFO;
}  // namespace RenderPass
}  // namespace Set
}  // namespace Descriptor

#endif  //! RENDER_PASS_CONSTANTS_H