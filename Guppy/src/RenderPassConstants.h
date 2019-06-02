#ifndef RENDER_PASS_CONSTANTS_H
#define RENDER_PASS_CONSTANTS_H

#include <array>
#include <glm/glm.hpp>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <vulkan/vulkan.h>

// const glm::vec4 CLEAR_COLOR = {190.0f / 256.0f, 223.0f / 256.0f, 246.0f / 256.0f, 1.0f};
const glm::vec4 CLEAR_COLOR = {};
const VkClearColorValue DEFAULT_CLEAR_COLOR_VALUE = {{CLEAR_COLOR.x, CLEAR_COLOR.y, CLEAR_COLOR.z, CLEAR_COLOR.w}};

const VkClearDepthStencilValue DEFAULT_CLEAR_DEPTH_STENCIL_VALUE = {1.0f, 0};

enum class PIPELINE;
// clang-format off
namespace Sampler { struct CreateInfo; }
namespace Texture { struct CreateInfo; }
// clang-format on

enum class RENDER_PASS : uint64_t {
    DEFAULT = 0,
    IMGUI,
    SAMPLER,
};

extern const std::vector<RENDER_PASS> RENDER_PASS_ALL;

namespace RenderPass {

class Base;

using offset = uint32_t;

constexpr uint8_t RESOURCE_SIZE = 20;

struct SwapchainResources {
    /* The "cleared" value should be set by the pass that clears the
     *  swapchain attachment (probably the first "SWAPCHAIN_DEPENDENT"
     *  pass), so that that any subsequent passes that do work on it
     *  know not to clear it.
     */
    bool cleared = false;
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

void AddDefaultSubpasses(RenderPass::Resources& resources, uint64_t count);

struct CreateInfo {
    RENDER_PASS type;
    std::string name;
    std::unordered_set<PIPELINE> pipelineTypes;
    bool swapchainDependent = true;
    bool includeDepth = true;
};

extern const CreateInfo DEFAULT_CREATE_INFO;
extern const CreateInfo SAMPLER_CREATE_INFO;

// TEXTURE
extern const Texture::CreateInfo TEXTURE_2D_CREATE_INFO;
extern const Texture::CreateInfo TEXTURE_2D_ARRAY_CREATE_INFO;

}  // namespace RenderPass

#endif  //! RENDER_PASS_CONSTANTS_H