#ifndef RENDER_PASS_CONSTANTS_H
#define RENDER_PASS_CONSTANTS_H

#include <unordered_set>
#include <vulkan/vulkan.h>

enum class PIPELINE;

namespace RenderPass {

struct CreateInfo {
    std::unordered_set<PIPELINE> types;
    bool clearColor = false;
    bool clearDepth = false;
};

struct Data {
    std::vector<VkFence> fences;
    std::vector<VkFramebuffer> framebuffers;
    std::vector<VkCommandBuffer> priCmds;
    std::vector<VkCommandBuffer> secCmds;
    std::vector<VkSemaphore> semaphores;
    std::vector<int> tests;
};

struct SubpassResources {
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

void AddDefaultSubpasses(RenderPass::SubpassResources& resources, uint64_t count);

extern const CreateInfo DEFAULT_CREATE_INFO;
extern const CreateInfo SAMPLER_CREATE_INFO;

}  // namespace RenderPass

#endif  //! RENDER_PASS_CONSTANTS_H