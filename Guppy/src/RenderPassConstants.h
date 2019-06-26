#ifndef RENDER_PASS_CONSTANTS_H
#define RENDER_PASS_CONSTANTS_H

#include <array>
#include <glm/glm.hpp>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

#include "DescriptorConstants.h"
#include "Types.h"

enum class DESCRIPTOR_SET;
enum class PIPELINE : uint32_t;
enum class SHADER;
// clang-format off
namespace Sampler { struct CreateInfo; }
namespace Texture { struct CreateInfo; }
// clang-format on

// const glm::vec4 CLEAR_COLOR = {190.0f / 256.0f, 223.0f / 256.0f, 246.0f / 256.0f, 1.0f};
const glm::vec4 CLEAR_COLOR = {};
const VkClearColorValue DEFAULT_CLEAR_COLOR_VALUE = {{CLEAR_COLOR.x, CLEAR_COLOR.y, CLEAR_COLOR.z, CLEAR_COLOR.w}};

const VkClearDepthStencilValue DEFAULT_CLEAR_DEPTH_STENCIL_VALUE = {1.0f, 0};

enum class RENDER_PASS : uint32_t {
    DEFAULT = 0,           // Swapchain
    IMGUI,                 // Swapchain
    SAMPLER_DEFAULT,       // Sampler
    PROJECT,               // Sampler
    SCREEN_SPACE,          // Swapchain
    SCREEN_SPACE_SAMPLER,  // Sampler
    // Used to indicate bad data, and "all" in uniform offsets
    ALL_ENUM = UINT32_MAX,
};

namespace RenderPass {

using offset = uint32_t;
using descriptorPipelineOffsetsMap = std::map<Uniform::offsetsMapKey, Uniform::offsets>;

constexpr uint8_t RESOURCE_SIZE = 20;
constexpr std::string_view SWAPCHAIN_TARGET_ID = "Swapchain";
extern const std::vector<RENDER_PASS> ALL;

// clang-format off
using FLAG = enum : FlagBits {
    NONE =                  0x00000000,
    SWAPCHAIN =             0x00000001,
    DEPTH =                 0x00000002,
    MULTISAMPLE =           0x00000004,
    SECONDARY_COMMANDS =    0x00000008,
};
// clang-format off

struct PipelineData {
    constexpr bool operator==(const PipelineData &other) const {
        return usesDepth == other.usesDepth &&  //
               samples == other.samples;
    }
    constexpr bool operator!=(const PipelineData &other) { return !(*this == other); }
    VkBool32 usesDepth;
    VkSampleCountFlagBits samples;
};

struct SwapchainResources {
    std::vector<VkImage> images;
    std::vector<VkImageView> views;
};

struct SubmitResource {
    uint32_t waitSemaphoreCount = 0;
    std::array<VkSemaphore, RESOURCE_SIZE> waitSemaphores = {};
    std::array<VkPipelineStageFlags, RESOURCE_SIZE> waitDstStageMasks = {};
    uint32_t commandBufferCount = 0;
    std::array<VkCommandBuffer, RESOURCE_SIZE> commandBuffers = {};
    uint32_t signalSemaphoreCount = 0;
    std::array<VkSemaphore, RESOURCE_SIZE> signalSemaphores = {};
    // Set below for stages the signal semaphores should wait on.
    std::array<VkPipelineStageFlags, RESOURCE_SIZE> signalSrcStageMasks = {};
};

struct Data {
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> priCmds;
    std::vector<VkCommandBuffer> secCmds;
    std::vector<VkSemaphore> semaphores;
    VkPipelineStageFlags signalSrcStageMask = VK_PIPELINE_STAGE_FLAG_BITS_MAX_ENUM;
};

struct Resources {
    // storage
    std::vector<VkAttachmentReference> inputAttachments;
    std::vector<VkAttachmentReference> colorAttachments;
    std::vector<VkAttachmentReference> resolveAttachments;
    VkAttachmentReference depthStencilAttachment = {};
    std::vector<uint32_t> preserveAttachments;
    // render pass create info
    std::vector<VkSubpassDescription> subpasses;
    std::vector<VkAttachmentDescription> attachments;
    std::vector<VkSubpassDependency> dependencies;
};

struct CreateInfo {
    RENDER_PASS type;
    std::string name;
    std::unordered_set<PIPELINE> pipelineTypes;
    FlagBits flags = (FLAG::SWAPCHAIN | FLAG::DEPTH | FLAG::MULTISAMPLE);
    std::vector<std::string> textureIds;
    descriptorPipelineOffsetsMap descPipelineOffsets;
};

// TEXTURE
constexpr std::string_view DEFAULT_2D_TEXTURE_ID = "Render Pass Texture 2D Texture";
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

#endif  //! RENDER_PASS_CONSTANTS_H