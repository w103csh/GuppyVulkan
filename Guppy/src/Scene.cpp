
#include <type_traits>

#include "Scene.h"
#include "Vertex.h"

Scene::Scene(const MyShell::Context& ctx, const Game::Settings& settings, const CommandData& cmd_data,
             const VkDescriptorBufferInfo& ubo_info, const ShaderResources& vs, const ShaderResources& fs,
             const VkPipelineCache& cache)
    : desc_pool_(nullptr), pipeline_info_({}), layout_(nullptr), tex_count_(0) {
    create_descriptor_set_layout(ctx);
    create_descriptor_pool(ctx);
    create_render_pass(ctx, settings);
    create_base_pipeline(ctx, settings, vs, fs, cache);
    create_draw_cmds(ctx, cmd_data);
}

void Scene::addMesh(const MyShell::Context& ctx, const CommandData& cmd_data, const VkDescriptorBufferInfo& ubo_info,
                    std::unique_ptr<Mesh> pMesh) {
    if (!pMesh->isReady()) {
        loading_futures_.emplace_back(pMesh->load(ctx, cmd_data));
    } else {
        pMesh->prepare(ctx, desc_set_layout_, desc_pool_, ubo_info);
    }
    meshes_.push_back(std::move(pMesh));
}

void Scene::removeMesh(Mesh* mesh) {
    // TODO...
    return;
}

bool Scene::update(const MyShell::Context& ctx, const Game::Settings& settings, const VkDescriptorBufferInfo& ubo_info,
                   const ShaderResources& vs, const ShaderResources& fs, const VkPipelineCache& cache) {
    auto startCount = readyCount();

    // Check futures...
    auto it = loading_futures_.begin();
    while (it != loading_futures_.end()) {
        auto& fut = (*it);
        auto status = fut.wait_for(std::chrono::seconds(0));
        if (status == std::future_status::ready) {
            auto pMesh = fut.get();
            pMesh->prepare(ctx, desc_set_layout_, desc_pool_, ubo_info);
            it = loading_futures_.erase(it);
        } else
            ++it;
    }

    //// Check if texture count has changed... This results in having to recreate the descriptors/pipelines.
    // uint32_t tex_count = 0;
    // for (auto& pMesh : meshes_)
    //    if (pMesh->hasTexture()) tex_count++;

    //// Create or re-create descriptor layout/pool, and pipelines
    // if (tex_count != tex_count) {
    //    bool destroy = desc_set_layouts_.size() > 0;

    //    // Descriptors
    //    // Layouts
    //    if (destroy) destroy_descriptor_set_layouts(ctx.dev);
    //    create_descriptor_set_layouts(ctx);
    //    // Pool
    //    if (destroy) destroy_descriptor_pool(ctx.dev);
    //    create_descriptor_pool(ctx);

    //    // Pipelines (just recreate everything for now...)
    //    if (destroy) destroy_pipelines(ctx.dev);
    //    create_pipelines(ctx.dev, settings, vs, fs, cache);
    //}

    // TODO: This is bad. Should store descriptor sets on the scene and only record if a new
    // descriptor set is added... or something like that. Really this should be fast...
    return startCount != readyCount();
}

// TODO: make cmd amount dynamic
const VkCommandBuffer& Scene::getDrawCmds(const uint32_t& frame_data_index) {
    auto& res = draw_resources_[frame_data_index];
    if (res.thread.joinable()) res.thread.join();
    return res.cmd;
}

void Scene::create_draw_cmds(const MyShell::Context& ctx, const CommandData& cmd_data) {
    draw_resources_.resize(ctx.image_count);

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = cmd_data.cmd_pools[cmd_data.graphics_queue_family];
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;

    for (auto& res : draw_resources_) {
        vk::assert_success(vkAllocateCommandBuffers(ctx.dev, &alloc_info, &res.cmd));
    }
}

void Scene::recordDrawCmds(const MyShell::Context& ctx, const std::vector<FrameData>& frame_data,
                           const std::vector<VkFramebuffer>& framebuffers, const VkViewport& viewport, const VkRect2D& scissor) {
    for (size_t i = 0; i < draw_resources_.size(); i++) {
        if (vkGetFenceStatus(ctx.dev, frame_data[i].fence) == VK_SUCCESS) {
            // sync
            record(ctx, draw_resources_[i].cmd, framebuffers[i], frame_data[i].fence, viewport, scissor, i);
        } else {
            // asnyc TODO: Can't update current draw until its done.. This isn't great. It can be revisted later.
            draw_resources_[i].thread = std::thread(&Scene::record, this, ctx, draw_resources_[i].cmd, framebuffers[i],
                                                    frame_data[i].fence, viewport, scissor, i);
        }
    }
}

void Scene::record(const MyShell::Context& ctx, const VkCommandBuffer& cmd, const VkFramebuffer& framebuffer, const VkFence& fence,
                   const VkViewport& viewport, const VkRect2D& scissor, size_t index) {
    // Wait for fences
    vk::assert_success(vkWaitForFences(ctx.dev, 1, &fence, VK_TRUE, UINT64_MAX));

    // Reset buffer
    vkResetCommandBuffer(cmd, 0);

    // Record
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin_info.pInheritanceInfo = nullptr;  // Optional

    vk::assert_success(vkBeginCommandBuffer(cmd, &begin_info));

    for (auto& render_pass : render_passes_) {
        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = render_pass;
        render_pass_info.framebuffer = framebuffer;
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = ctx.extent;

        /*  Need to pad this here because:

            In vkCmdBeginRenderPass() the VkRenderPassBeginInfo struct has a clearValueCount
            of 2 but there must be at least 3 entries in pClearValues array to account for the
            highest index attachment in renderPass 0x2f that uses VK_ATTACHMENT_LOAD_OP_CLEAR
            is 3. Note that the pClearValues array is indexed by attachment number so even if
            some pClearValues entries between 0 and 2 correspond to attachments that aren't
            cleared they will be ignored. The spec valid usage text states 'clearValueCount
            must be greater than the largest attachment index in renderPass that specifies a
            loadOp (or stencilLoadOp, if the attachment has a depth/stencil format) of
            VK_ATTACHMENT_LOAD_OP_CLEAR'
            (https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#VUID-VkRenderPassBeginInfo-clearValueCount-00902)
        */
        std::array<VkClearValue, 3> clearValues = {};
        // clearValuues[0] = final layout is in this spot; // pad this spot
        clearValues[1].color = CLEAR_VALUE;
        clearValues[2].depthStencil = {1.0f, 0};

        render_pass_info.clearValueCount = static_cast<uint32_t>(clearValues.size());
        render_pass_info.pClearValues = clearValues.data();

        // Start a new debug marker region
        ext::DebugMarkerBegin(cmd, "Render x scene", glm::vec4(0.5f, 0.76f, 0.34f, 1.0f));

        vkCmdBeginRenderPass(cmd, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);

        for (auto& pMesh : meshes_) {
            auto& pipeline = pipelines_[static_cast<int>(PIPELINE_TYPE::BASE)];
            pMesh->draw(cmd, layout_, pipeline, index);
            // switch (pMesh->vertexType()) {
            //    case VERTEX_TYPE::COLOR: {
            //        // pipeline = &pipelines_[static_cast<int>(PIPELINE_TYPE::POLY)];
            //        pipeline = &pipelines_[static_cast<int>(PIPELINE_TYPE::BASE)];
            //        auto& pCMesh = reinterpret_cast<std::unique_ptr<Mesh<ColorVertex>>&>(pMesh);
            //        pCMesh->draw(cmd, layout_, *pipeline, i);
            //    } break;
            //    case VERTEX_TYPE::TEXTURE: {
            //        pipeline = &pipelines_[static_cast<int>(PIPELINE_TYPE::LINE)];
            //        auto& pTMesh = reinterpret_cast<std::unique_ptr<Mesh<TextureVertex>>&>(pMesh);
            //        pTMesh->draw(cmd, layout_, *pipeline, i);
            //    } break;
            //}
        }

        vkCmdEndRenderPass(cmd);

        // End current debug marker region
        ext::DebugMarkerEnd(cmd);

    }

    vk::assert_success(vkEndCommandBuffer(cmd));
}

// Depends on:
//      ctx - dev
//      ctx - image_count
// Can change:
//      Possibly make the set layout creation optional based on state of scene
//      Shader bindings
//      Make binding count dynamic
void Scene::create_descriptor_set_layout(const MyShell::Context& ctx) {
    // if (use_push_constants_) return;

    // UNIFORM BUFFER
    VkDescriptorSetLayoutBinding ubo_binding = {};
    ubo_binding.binding = 0;
    ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_binding.descriptorCount = 1;
    ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_binding.pImmutableSamplers = nullptr;  // Optional

    // TEXTURE
    VkDescriptorSetLayoutBinding tex_binding = {};
    tex_binding.binding = 1;
    tex_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    tex_binding.descriptorCount = 1;
    tex_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    tex_binding.pImmutableSamplers = nullptr;  // Optional

    std::array<VkDescriptorSetLayoutBinding, 2> layout_bindings = {ubo_binding, tex_binding};

    // LAYOUT
    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    uint32_t bindingCount = static_cast<uint32_t>(layout_bindings.size());
    layout_info.bindingCount = bindingCount;
    layout_info.pBindings = bindingCount > 0 ? layout_bindings.data() : nullptr;

    //// TODO: are all these layouts necessary?
    //desc_set_layouts_.resize(ctx.image_count);
    //for (auto& layout : desc_set_layouts_) {
        vk::assert_success(vkCreateDescriptorSetLayout(ctx.dev, &layout_info, nullptr, &desc_set_layout_));
    //}

    // Name for debugging
    ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)desc_set_layout_, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, "base descriptor set layout");
}

// Depends on:
//      ctx - dev
//      ctx - image_count
// Can change:
//      Possibly make the pool creation optional based on state of scene
//      Make uniform buffer optional
void Scene::create_descriptor_pool(const MyShell::Context& ctx) {
    //// 1 for uniform buffer
    //uint32_t descCount = 1 + tex_count_;
    //std::vector<VkDescriptorPoolSize> pool_sizes(descCount);

    //// Uniform buffer
    //// TODO: look at dynamic ubo's (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
    //pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //pool_sizes[0].descriptorCount = 1;

    //// Texture samplers
    //for (uint32_t i = 1; i < tex_count_; i++) {
    //    pool_sizes[i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    //    pool_sizes[i].descriptorCount = 1;
    //}

    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = ctx.image_count;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = ctx.image_count;

    VkDescriptorPoolCreateInfo desc_pool_info = {};
    desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc_pool_info.maxSets = ctx.image_count;
    desc_pool_info.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    //desc_pool_info.poolSizeCount = descCount;
    desc_pool_info.pPoolSizes = poolSizes.data();
    //desc_pool_info.pPoolSizes = descCount > 0 ? pool_sizes.data() : nullptr;

    vk::assert_success(vkCreateDescriptorPool(ctx.dev, &desc_pool_info, nullptr, &desc_pool_));
}

// Depends on:
//      ctx - surface_format.format
//      ctx - depth_format
//      settings - include_color
//      settings - include_depth
// Can change:
//      Make subpasses dynamic
//      Make dependencies dynamic
//      Make attachments more dynamic
void Scene::create_render_pass(const MyShell::Context& ctx, const Game::Settings& settings, bool clear, VkImageLayout finalLayout) {
    // ATTACHMENTS
    std::vector<VkAttachmentDescription> attachments;

    // COLOR ATTACHMENT (SWAP-CHAIN)
    VkAttachmentReference color_reference = {};
    VkAttachmentDescription attachemnt = {};
    attachemnt = {};
    attachemnt.format = ctx.surface_format.format;
    attachemnt.samples = VK_SAMPLE_COUNT_1_BIT;
    attachemnt.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachemnt.finalLayout = finalLayout;
    attachemnt.flags = 0;
    attachments.push_back(attachemnt);
    // REFERENCE
    color_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // COLOR ATTACHMENT RESOLVE (MULTI-SAMPLE)
    VkAttachmentReference color_resolve_reference = {};
    if (settings.include_color) {
        attachemnt = {};
        attachemnt.format = ctx.surface_format.format;
        attachemnt.samples = ctx.num_samples;
        attachemnt.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachemnt.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachemnt.flags = 0;
        // REFERENCE
        color_resolve_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);  // point to swapchain attachment
        color_resolve_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        attachments.push_back(attachemnt);
        color_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);  // point to multi-sample attachment
    }

    // DEPTH ATTACHMENT
    VkAttachmentReference depth_reference = {};
    if (settings.include_depth) {
        attachemnt = {};
        attachemnt.format = ctx.depth_format;
        attachemnt.samples = ctx.num_samples;
        attachemnt.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        // This was "don't care" in the sample and that makes more sense to
        // me. This obvious is for some kind of stencil operation.
        attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachemnt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        attachemnt.flags = 0;
        attachments.push_back(attachemnt);
        // REFERENCE
        depth_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);
        depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    // SUBPASSES
    std::vector<VkSubpassDescription> subpasses;
    VkSubpassDescription subpass = {};

    subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pResolveAttachments = settings.include_color ? &color_resolve_reference : nullptr;
    subpass.pDepthStencilAttachment = settings.include_depth ? &depth_reference : nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;
    subpasses.push_back(subpass);

    // Line drawing subpass (just need an identical subpass for now)
    // subpasses.push_back(subpass);

    // DEPENDENCIES
    std::vector<VkSubpassDependency> dependencies;
    VkSubpassDependency dependency = {};

    //// TODO: used for waiting in draw (figure this out... what is the VK_SUBPASS_EXTERNAL one?)
    //dependency = {};
    //dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    //dependency.dstSubpass = 0;
    //dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dependency.srcAccessMask = 0;
    //dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //dependencies.push_back(dependency);

    // TODO: fix this...
     //dependency = {};
     //dependency.srcSubpass = 0;
     //dependency.dstSubpass = 1;
     //dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
     //dependency.srcAccessMask = 0;
     //dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
     //dependency.dstAccessMask = 0;
     //dependencies.push_back(dependency);

    VkRenderPassCreateInfo rp_info = {};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.pNext = nullptr;
    // Attachmments
    uint32_t attachmentCount = static_cast<uint32_t>(attachments.size());
    rp_info.attachmentCount = attachmentCount;
    rp_info.pAttachments = attachmentCount > 0 ? attachments.data() : nullptr;
    // Subpasses
    uint32_t subpassCount = static_cast<uint32_t>(subpasses.size());
    rp_info.subpassCount = subpassCount;
    rp_info.pSubpasses = subpassCount > 0 ? subpasses.data() : nullptr;
    // Dependencies
    uint32_t dependencyCount = static_cast<uint32_t>(dependencies.size());
    rp_info.dependencyCount = dependencyCount;
    rp_info.pDependencies = dependencyCount > 0 ? dependencies.data() : nullptr;

    VkRenderPass render_pass;
    vk::assert_success(vkCreateRenderPass(ctx.dev, &rp_info, nullptr, &render_pass));
    render_passes_.emplace_back(render_pass);
}

// Depends on:
//      desc_set_layouts_
// Can change:
//      Hook up push constants
void Scene::create_pipeline_layout(const VkDevice& dev, VkPipelineLayout& layout) {
    VkPushConstantRange push_const_range = {};

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    // if (use_push_constants_) {
    //    throw std::runtime_error("not handled!");
    //    push_const_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    //    push_const_range.offset = 0;
    //    push_const_range.size = sizeof(ShaderParamBlock);
    //
    //    pipeline_layout_info.pushConstantRangeCount = 1;
    //    pipeline_layout_info.pPushConstantRanges = &push_const_range;
    //} else {
    //uint32_t setLayoutCount = static_cast<uint32_t>(desc_set_layout_.size());
    //pipeline_layout_info.setLayoutCount = setLayoutCount;
    //pipeline_layout_info.pSetLayouts = setLayoutCount > 0 ? desc_set_layout_.data() : nullptr;
    pipeline_layout_info.setLayoutCount = 1;
    pipeline_layout_info.pSetLayouts = &desc_set_layout_;
    //}

    vk::assert_success(vkCreatePipelineLayout(dev, &pipeline_layout_info, nullptr, &layout));

    // Name for debugging
    ext::DebugMarkerSetObjectName(dev, (uint64_t)layout, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, "Base pipeline layout");
}

void Scene::destroy_pipeline_layouts(const VkDevice& dev) { vkDestroyPipelineLayout(dev, layout_, nullptr); }

// Depends on:
//      Vertex::getBindingDescription()
//      Vertex::getAttributeDescriptions()
//      settings - enable_sample_shading
//      MIN_SAMPLE_SHADING
//      NUM_VIEWPORTS
//      NUM_SCISSORS
//      settings - include_depth
//      vs - info
//      fs - info
//      layout_
//      render_pass_
// Can change:
//      Just need an array of shader info
//      Make layout_ dynamic
//      Make render_pass_ dynamic
void Scene::create_base_pipeline(const MyShell::Context& ctx, const Game::Settings& settings, const ShaderResources& vs,
                                 const ShaderResources& fs, const VkPipelineCache& cache) {
    create_pipeline_layout(ctx.dev, layout_);

    // DYNAMIC STATE

    // TODO: this is weird
    VkPipelineDynamicStateCreateInfo dynamic_info = {};
    VkDynamicState dynamic_states[VK_DYNAMIC_STATE_RANGE_SIZE];
    memset(dynamic_states, 0, sizeof(dynamic_states));
    dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_info.pNext = nullptr;
    dynamic_info.pDynamicStates = dynamic_states;
    dynamic_info.dynamicStateCount = 0;

    // INPUT ASSEMBLY

    VkPipelineVertexInputStateCreateInfo vertex_info = {};
    vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_info.pNext = nullptr;
    vertex_info.flags = 0;
    // bindings
    auto bindingDescription = Vertex::getTextureBindingDescription();
    vertex_info.vertexBindingDescriptionCount = 1;
    vertex_info.pVertexBindingDescriptions = &bindingDescription;
    // attributes
    auto attributeDescriptions = Vertex::getTextureAttributeDescriptions();
    vertex_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertex_info.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo input_info = {};
    input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_info.pNext = nullptr;
    input_info.flags = 0;
    input_info.primitiveRestartEnable = VK_FALSE;
    input_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // RASTERIZER

    VkPipelineRasterizationStateCreateInfo rast_info = {};
    rast_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rast_info.polygonMode = VK_POLYGON_MODE_FILL;
    rast_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rast_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    /*  If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far
        planes are clamped to them as opposed to discarding them. This is useful in some special
        cases like shadow maps. Using this requires enabling a GPU feature.
    */
    rast_info.depthClampEnable = VK_FALSE;
    rast_info.rasterizerDiscardEnable = VK_FALSE;
    rast_info.depthBiasEnable = VK_FALSE;
    rast_info.depthBiasConstantFactor = 0;
    rast_info.depthBiasClamp = 0;
    rast_info.depthBiasSlopeFactor = 0;
    /*  The lineWidth member is straightforward, it describes the thickness of lines in terms of
        number of fragments. The maximum line width that is supported depends on the hardware and
        any line thicker than 1.0f requires you to enable the wideLines GPU feature.
    */
    rast_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_info = {};
    multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_info.rasterizationSamples = ctx.num_samples;
    multisample_info.sampleShadingEnable =
        settings.enable_sample_shading;  // enable sample shading in the pipeline (sampling for fragment interiors)
    multisample_info.minSampleShading =
        settings.enable_sample_shading ? MIN_SAMPLE_SHADING : 0.0f;  // min fraction for sample shading; closer to one is smooth
    multisample_info.pSampleMask = nullptr;                          // Optional
    multisample_info.alphaToCoverageEnable = VK_FALSE;               // Optional
    multisample_info.alphaToOneEnable = VK_FALSE;                    // Optional

    // BLENDING

    VkPipelineColorBlendAttachmentState blend_attachment = {};
    blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // blend_attachment.blendEnable = VK_FALSE;
    // blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional
    // blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
    // blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    // blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    // blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    // blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    // common setup
    blend_attachment.blendEnable = VK_TRUE;
    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo blend_info = {};
    blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blend_info.attachmentCount = 1;
    blend_info.pAttachments = &blend_attachment;
    blend_info.logicOpEnable = VK_FALSE;
    blend_info.logicOp = VK_LOGIC_OP_COPY;  // What does this do?
    blend_info.blendConstants[0] = 0.0f;
    blend_info.blendConstants[1] = 0.0f;
    blend_info.blendConstants[2] = 0.0f;
    blend_info.blendConstants[3] = 0.0f;
    // blend_info.blendConstants[0] = 0.2f;
    // blend_info.blendConstants[1] = 0.2f;
    // blend_info.blendConstants[2] = 0.2f;
    // blend_info.blendConstants[3] = 0.2f;

    // VIEWPORT & SCISSOR

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
#ifndef __ANDROID__
    viewport_info.viewportCount = NUM_VIEWPORTS;
    dynamic_states[dynamic_info.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    viewport_info.scissorCount = NUM_SCISSORS;
    dynamic_states[dynamic_info.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
    viewport_info.pScissors = nullptr;
    viewport_info.pViewports = nullptr;
#else
    // Temporary disabling dynamic viewport on Android because some of drivers doesn't
    // support the feature.
    VkViewport viewports;
    viewports.minDepth = 0.0f;
    viewports.maxDepth = 1.0f;
    viewports.x = 0;
    viewports.y = 0;
    viewports.width = info.width;
    viewports.height = info.height;
    VkRect2D scissor;
    scissor.extent.width = info.width;
    scissor.extent.height = info.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vp.viewportCount = NUM_VIEWPORTS;
    vp.scissorCount = NUM_SCISSORS;
    vp.pScissors = &scissor;
    vp.pViewports = &viewports;
#endif

    // DEPTH

    VkPipelineDepthStencilStateCreateInfo depth_info;
    depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_info.pNext = nullptr;
    depth_info.flags = 0;
    depth_info.depthTestEnable = settings.include_depth;
    depth_info.depthWriteEnable = settings.include_depth;
    depth_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_info.depthBoundsTestEnable = VK_FALSE;
    depth_info.minDepthBounds = 0;
    depth_info.maxDepthBounds = 1.0f;
    depth_info.stencilTestEnable = VK_FALSE;
    depth_info.front = {};
    depth_info.back = {};
    // dss.back.failOp = VK_STENCIL_OP_KEEP; // ARE THESE IMPORTANT !!!
    // dss.back.passOp = VK_STENCIL_OP_KEEP;
    // dss.back.compareOp = VK_COMPARE_OP_ALWAYS;
    // dss.back.compareMask = 0;
    // dss.back.reference = 0;
    // dss.back.depthFailOp = VK_STENCIL_OP_KEEP;
    // dss.back.writeMask = 0;
    // dss.front = ds.back;

    // SHADER

    // TODO: this is not great
    std::vector<VkPipelineShaderStageCreateInfo> stage_infos;
    stage_infos.push_back(vs.info);
    stage_infos.push_back(fs.info);

    // PIPELINE

    VkGraphicsPipelineCreateInfo pipeline_info_ = {};
    pipeline_info_.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // shader stages
    uint32_t stageCount = static_cast<uint32_t>(stage_infos.size());
    pipeline_info_.stageCount = stageCount;
    pipeline_info_.pStages = stageCount > 0 ? stage_infos.data() : nullptr;
    // fixed function stages
    pipeline_info_.pColorBlendState = &blend_info;
    pipeline_info_.pDepthStencilState = &depth_info;
    pipeline_info_.pDynamicState = &dynamic_info;
    pipeline_info_.pInputAssemblyState = &input_info;
    pipeline_info_.pMultisampleState = &multisample_info;
    pipeline_info_.pRasterizationState = &rast_info;
    pipeline_info_.pTessellationState = nullptr;
    pipeline_info_.pVertexInputState = &vertex_info;
    pipeline_info_.pViewportState = &viewport_info;
    // layout
    pipeline_info_.layout = layout_;
    pipeline_info_.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info_.basePipelineIndex = 0;
    // render pass
    pipeline_info_.renderPass = render_passes_[0];
    pipeline_info_.subpass = 0;

    vk::assert_success(vkCreateGraphicsPipelines(ctx.dev, cache, 1, &pipeline_info_, nullptr, &base_pipeline()));

    // Name shader moduels for debugging
    // Shader module count starts at 2 when UI overlay in base class is enabled
    //uint32_t moduleIndex = settings.overlay ? 2 : 0;
    uint32_t moduleIndex = false ? 2 : 0;
    ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)vs.module, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "vs_");
    ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)fs.module, VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, "fs_");

    // Name pipelines for debugging
    ext::DebugMarkerSetObjectName(ctx.dev, (uint64_t)base_pipeline(), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, "Base pipeline");
}

void Scene::create_pipelines(const VkDevice& dev, const Game::Settings& settings, const ShaderResources& vs,
                             const ShaderResources& fs, const VkPipelineCache& cache) {
    create_pipeline_layout(dev, layout_);

    pipeline_info_.flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
    pipeline_info_.layout = layout_;
    pipeline_info_.basePipelineHandle = base_pipeline();
    pipeline_info_.basePipelineIndex = -1;

    VkPipelineInputAssemblyStateCreateInfo input_info = {};
    input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_info.primitiveRestartEnable = VK_FALSE;

    // Start from BASE + 1
    for (size_t i = 1; i < pipelines_.size(); i++) {
        // Do whatever is different...

        switch (i) {
            case static_cast<size_t>(PIPELINE_TYPE::POLY): {
                input_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
                pipeline_info_.pInputAssemblyState = &input_info;
                pipeline_info_.layout = layout_;

                pipeline_info_.subpass = 0;
            } break;
            case static_cast<size_t>(PIPELINE_TYPE::LINE): {
                input_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
                pipeline_info_.pInputAssemblyState = &input_info;
                pipeline_info_.layout = layout_;

                pipeline_info_.subpass = 1;
            } break;
        }

        vk::assert_success(vkCreateGraphicsPipelines(dev, cache, 1, &pipeline_info_, nullptr, &pipelines_[i]));
    }
}

void Scene::destroy_descriptor_set_layouts(const VkDevice& dev) {
    //for (auto& layout : desc_set_layout_) vkDestroyDescriptorSetLayout(dev, layout, nullptr);
    //desc_set_layout_.clear();
    vkDestroyDescriptorSetLayout(dev, desc_set_layout_, nullptr);
}

void Scene::destroy_descriptor_pool(const VkDevice& dev) { vkDestroyDescriptorPool(dev, desc_pool_, nullptr); }

// TODO: fix this
void Scene::destroy_pipelines(const VkDevice& dev, bool destroy_base) {
    // for (size_t i = 0; i < pipelines_.size(); i++) {
    for (size_t i = 0; i < 1; i++) {
        if (i == static_cast<size_t>(PIPELINE_TYPE::BASE) && !destroy_base) {
            continue;  // really...
        } else {
            vkDestroyPipeline(dev, pipelines_[i], nullptr);
        }
    }
}

void Scene::destroy_render_passes(const VkDevice& dev) {
    for (auto& pass : render_passes_) vkDestroyRenderPass(dev, pass, nullptr);
}

void Scene::destroy_cmds(const VkDevice& dev, const CommandData& cmd_data) {
    for (auto& res : draw_resources_) {
        if (res.thread.joinable()) res.thread.join();
        vkFreeCommandBuffers(dev, cmd_data.cmd_pools[cmd_data.graphics_queue_family], 1, &res.cmd);
    }
    draw_resources_.clear();
}

void Scene::destroy(const VkDevice& dev, const CommandData& cmd_data) {
    // mesh
    for (auto& pMesh : meshes_) pMesh->destroy(dev);
    // descriptor
    destroy_descriptor_pool(dev);
    destroy_descriptor_set_layouts(dev);
    // pipeline
    destroy_pipeline_layouts(dev);
    destroy_pipelines(dev, true);
    // render pass
    destroy_render_passes(dev);
    // commands
    destroy_cmds(dev, cmd_data);
}