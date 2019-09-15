// Set this to the index in VkRenderPassCreateInfo::pAttachments where the depth image is described.
uint32_t depthAttachmentIndex = ...;

VkSubpassDescription subpasses[2];

VkAttachmentReference depthAttachment = {
    .attachment = depthAttachmentIndex,
    .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL};

// Subpass containing first draw
subpasses[0] = {
    ...
    .pDepthStencilAttachment = &depthAttachment,
    ...};
    
VkAttachmentReference depthAsInputAttachment = {
    .attachment = depthAttachmentIndex,
    .layout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

// Subpass containing second draw
subpasses[1] = {
    ...
    .inputAttachmentCount = 1,
    .pInputAttachments = &depthAsInputAttachment,
    ...};
    
VkSubpassDependency dependency = {
    .srcSubpass = 0,
    .dstSubpass = 1,
    .srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
    .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
    .dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
    .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT};

// If initialLayout does not match the layout of the attachment reference in the first subpass, there will be an implicit transition before starting the render pass.
// If finalLayout does not match the layout of the attachment reference in the last subpass, there will be an implicit transition at the end.
VkAttachmentDescription depthFramebufferAttachment = {
    ...
    .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
    .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL};

VkRenderPassCreateInfo renderPassCreateInfo = {
    ...
    .attachmentCount = 1,
    .pAttachments = &depthFramebufferAttachment,
    .subpassCount = 2,
    .pSubpasses = subpasses,
    .dependencyCount = 1,
    .pDependencies = &dependency};

vkCreateRenderPass(...);

...