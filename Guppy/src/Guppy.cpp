
#include <algorithm>

#include "CmdBufHandler.h"
#include "Constants.h"
#include "EventHandlers.h"
#include "Extensions.h"
#include "FileLoader.h"
#include "Guppy.h"
#include "Helpers.h"
#include "InputHandler.h"
#include "PipelineHandler.h"
#include "Plane.h"

namespace {

// TODO do not rely on compiler to use std140 layout
// TODO move lower frequency data to another descriptor set
struct ShaderParamBlock {
    float light_pos[4];
    float light_color[4];
    float model[4 * 4];
    float view_projection[4 * 4];
    float alpha;
};

struct DemoTag {
    const char name[17] = "debug marker tag";
} demoTag;

}  // namespace

Guppy::Guppy(const std::vector<std::string>& args)
    : Game("Guppy", args),
      multithread_(true),
      use_push_constants_(false),
      sim_paused_(false),
      sim_fade_(false),
      // sim_(5000),
      // camera_(2.5f),
      frame_data_(),
      render_pass_clear_value_({{0.0f, 0.1f, 0.2f, 1.0f}}),
      render_pass_begin_info_(),
      primary_cmd_begin_info_(),
      primary_cmd_submit_info_(),
      depth_resource_(),
      active_scene_index_(-1),
      camera_(glm::vec3(2.0f, -4.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f),
              static_cast<float>(settings_.initial_width) / static_cast<float>(settings_.initial_height)) {
    for (auto it = args.begin(); it != args.end(); ++it) {
        if (*it == "-s")
            multithread_ = false;
        else if (*it == "-p")
            use_push_constants_ = true;
    }

    // init_workers();
}

Guppy::~Guppy() {}

void Guppy::attach_shell(MyShell& sh) {
    Game::attach_shell(sh);

    const MyShell::Context& ctx = sh.context();
    physical_dev_ = ctx.physical_dev;
    dev_ = ctx.dev;
    // queue_ = ctx.game_queue;
    queue_family_ = ctx.game_queue_family;
    format_ = ctx.surface_format.format;
    // * Data from MyShell
    swapchain_image_count_ = ctx.image_count;
    depth_resource_.format = ctx.depth_format;
    physical_dev_props_ = ctx.physical_dev_props[ctx.physical_dev_index].properties;
    CmdBufHandler::init(ctx);

    if (use_push_constants_ && sizeof(ShaderParamBlock) > physical_dev_props_.limits.maxPushConstantsSize) {
        shell_->log(MyShell::LOG_WARN, "cannot enable push constants");
        use_push_constants_ = false;
    }

    mem_flags_.reserve(CmdBufHandler::mem_props().memoryTypeCount);
    for (uint32_t i = 0; i < CmdBufHandler::mem_props().memoryTypeCount; i++)
        mem_flags_.push_back(CmdBufHandler::mem_props().memoryTypes[i].propertyFlags);

    // meshes_ = new Meshes(dev_, mem_flags_);

    create_uniform_buffer();
    PipelineHandler::init(ctx, settings());

    create_scenes();

    // render_pass_begin_info_.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    // render_pass_begin_info_.renderPass = render_pass_;
    // render_pass_begin_info_.clearValueCount = 1;
    // render_pass_begin_info_.pClearValues = &render_pass_clear_value_;

    // primary_cmd_begin_info_.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // primary_cmd_begin_info_.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    //// we will render to the swapchain images
    // primary_cmd_submit_wait_stages_ = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    // primary_cmd_submit_info_.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // primary_cmd_submit_info_.waitSemaphoreCount = 1;
    // primary_cmd_submit_info_.pWaitDstStageMask = &primary_cmd_submit_wait_stages_;
    // primary_cmd_submit_info_.commandBufferCount = 1;
    // primary_cmd_submit_info_.signalSemaphoreCount = 1;

    if (multithread_) {
        // for (auto &worker : workers_) worker->start();
    }
}

void Guppy::detach_shell() {
    if (multithread_) {
        // for (auto &worker : workers_) worker->stop();
    }

    // vk::DestroyPipeline(dev_, pipeline_, nullptr);
    // vk::DestroyPipelineLayout(dev_, pipelineLayout_, nullptr);
    // if (!use_push_constants_) vk::DestroyDescriptorSetLayout(dev_, descSets_layout_, nullptr);
    // vk::DestroyShaderModule(dev_, fs_, nullptr);
    // vk::DestroyShaderModule(dev_, vs_, nullptr);
    // vk::DestroyRenderPass(dev_, renderPass_, nullptr);

    // delete meshes_;

    // *** NEW

    for (auto& scene : scenes_) scene->destroy(dev_);

    // destroy_pipelines();
    destroy_pipeline_cache();
    // destroy_descriptor_and_pipeline_layouts();
    // destroy_descriptor_sets();
    // destroy_descriptor_pool();
    destroy_shader_modules();
    // destroy_render_pass();

    destroy_uniform_buffer();  // TODO: ugh!
    // destroy_input_assembly_data();
    // destroy_textures();

    CmdBufHandler::destroy();
    PipelineHandler::destroy();

    Game::detach_shell();
}

void Guppy::attach_swapchain() {
    const MyShell::Context& ctx = shell_->context();
    extent_ = ctx.extent;

    // Get the image data from the swapchain
    get_swapchain_image_data(ctx.swapchain);

    create_frame_data(swapchain_image_count_);

    prepare_viewport();
    prepare_framebuffers(ctx.swapchain);

    active_scene()->recordDrawCmds(ctx, frameData(), framebuffers_, viewport_, scissor_);
}

void Guppy::detach_swapchain() {
    for (auto& fb : framebuffers_) vkDestroyFramebuffer(dev_, fb, nullptr);
    for (auto& res : swapchain_image_resources_) {
        vkDestroyImageView(dev_, res.view, nullptr);
        // res.image is destoryed by vkDestroySwapchainKHR
    }

    framebuffers_.clear();
    swapchain_image_resources_.clear();

    destroy_frame_data();
}

void Guppy::on_key(KEY key) {
    switch (key) {
        case KEY::KEY_SHUTDOWN:
        case KEY::KEY_ESC:
            shell_->quit();
            break;
        case KEY::KEY_SPACE:
            // sim_paused_ = !sim_paused_;
            break;
        case KEY::KEY_F:
            // sim_fade_ = !sim_fade_;
            break;
        case KEY::KEY_F1: {
            auto p1 = std::make_unique<ColorPlane>();
            active_scene()->addMesh(shell_->context(), ubo_resource_.info, std::move(p1));
        } break;
        case KEY::KEY_F3: {
            auto p1 = std::make_unique<TexturePlane>();
            active_scene()->addMesh(shell_->context(), ubo_resource_.info, std::move(p1));
        } break;
        case KEY::KEY_F5: {
            auto p1 = std::make_unique<ColorPlane>(1.0f, 1.0f, true, glm::vec3(),
                                                   glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
            active_scene()->addMesh(shell_->context(), ubo_resource_.info, std::move(p1));
        } break;
        default:
            break;
    }
}

void Guppy::on_frame(float frame_pred) {
    auto& data = frame_data_[frame_data_index_];

    // wait for the last submission since we reuse frame data
    vk::assert_success(vkWaitForFences(dev_, 1, &data.fence, true, UINT64_MAX));
    vk::assert_success(vkResetFences(dev_, 1, &data.fence));

    const MyShell::BackBuffer& back = shell_->context().acquired_back_buffer;

    update_ubo();

    uint32_t commandBufferCount = 1;
    auto draw_cmds = active_scene()->getDrawCmds(frame_data_index_);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.pWaitSemaphores = &back.acquire_semaphore;
    submit_info.commandBufferCount = commandBufferCount;
    submit_info.pCommandBuffers = &draw_cmds;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &back.render_semaphore;

    VkResult res = vkQueueSubmit(CmdBufHandler::graphics_queue(), 1, &submit_info, data.fence);

    frame_data_index_ = (frame_data_index_ + 1) % frame_data_.size();

    (void)res;
}

void Guppy::on_tick() {
    if (sim_paused_) return;

    auto ctx = shell_->context();

    // TODO: should this be "on_frame", here, every other frame, async ... I have no clue yet.
    for (auto& scene : scenes_) {
        // TODO: im really not into this...
        if (scene->update(ctx, ubo_resource_.info)) {
            scene->recordDrawCmds(ctx, frameData(), framebuffers_, viewport_, scissor_);
        }
    }

    // for (auto &worker : workers_) worker->update_simulation();
}

void Guppy::destroy_pipeline_cache() { vkDestroyPipelineCache(dev_, pipeline_cache_, nullptr); }

void Guppy::create_pipelines() {
    // DYNAMIC STATE
    //
    //    // TODO: this is weird
    //    VkPipelineDynamicStateCreateInfo dynamic_info = {};
    //    VkDynamicState dynamic_states[VK_DYNAMIC_STATE_RANGE_SIZE];
    //    memset(dynamic_states, 0, sizeof(dynamic_states));
    //    dynamic_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    //    dynamic_info.pNext = nullptr;
    //    dynamic_info.pDynamicStates = dynamic_states;
    //    dynamic_info.dynamicStateCount = 0;
    //
    //    // INPUT ASSEMBLY
    //
    //    VkPipelineVertexInputStateCreateInfo vertex_info = {};
    //    vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    //    vertex_info.pNext = nullptr;
    //    vertex_info.flags = 0;
    //    // bindings
    //    auto bindingDescription = Vertex::getBindingDescription();
    //    vertex_info.vertexBindingDescriptionCount = 1;
    //    vertex_info.pVertexBindingDescriptions = &bindingDescription;
    //    // attributes
    //    auto attributeDescriptions = Vertex::getAttributeDescriptions();
    //    vertex_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    //    vertex_info.pVertexAttributeDescriptions = attributeDescriptions.data();
    //
    //    VkPipelineInputAssemblyStateCreateInfo input_info = {};
    //    input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    //    input_info.pNext = nullptr;
    //    input_info.flags = 0;
    //    input_info.primitiveRestartEnable = VK_FALSE;
    //    input_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    //
    //    // RASTERIZER
    //
    //    VkPipelineRasterizationStateCreateInfo rast_info = {};
    //    rast_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    //    rast_info.polygonMode = VK_POLYGON_MODE_FILL;
    //    rast_info.cullMode = VK_CULL_MODE_BACK_BIT;
    //    rast_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    //    /*  If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far
    //        planes are clamped to them as opposed to discarding them. This is useful in some special
    //        cases like shadow maps. Using this requires enabling a GPU feature.
    //    */
    //    rast_info.depthClampEnable = VK_FALSE;
    //    rast_info.rasterizerDiscardEnable = VK_FALSE;
    //    rast_info.depthBiasEnable = VK_FALSE;
    //    rast_info.depthBiasConstantFactor = 0;
    //    rast_info.depthBiasClamp = 0;
    //    rast_info.depthBiasSlopeFactor = 0;
    //    /*  The lineWidth member is straightforward, it describes the thickness of lines in terms of
    //        number of fragments. The maximum line width that is supported depends on the hardware and
    //        any line thicker than 1.0f requires you to enable the wideLines GPU feature.
    //    */
    //    rast_info.lineWidth = 1.0f;
    //
    //    VkPipelineMultisampleStateCreateInfo multisample_info = {};
    //    multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    //    multisample_info.rasterizationSamples = num_samples_;
    //    multisample_info.sampleShadingEnable =
    //        settings_.enable_sample_shading;  // enable sample shading in the pipeline (sampling for fragment interiors)
    //    multisample_info.minSampleShading =
    //        settings_.enable_sample_shading ? MIN_SAMPLE_SHADING : 0.0f;  // min fraction for sample shading; closer to one is
    //        smooth
    //    multisample_info.pSampleMask = nullptr;                           // Optional
    //    multisample_info.alphaToCoverageEnable = VK_FALSE;                // Optional
    //    multisample_info.alphaToOneEnable = VK_FALSE;                     // Optional
    //
    //    // BLENDING
    //
    //    VkPipelineColorBlendAttachmentState blend_attachment = {};
    //    blend_attachment.colorWriteMask =
    //        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    //    // blend_attachment.blendEnable = VK_FALSE;
    //    // blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional
    //    // blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
    //    // blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    //    // blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    //    // blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    //    // blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    //    // common setup
    //    blend_attachment.blendEnable = VK_TRUE;
    //    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    //    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    //    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    //    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    //    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    //    blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
    //
    //    VkPipelineColorBlendStateCreateInfo blend_info = {};
    //    blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    //    blend_info.attachmentCount = 1;
    //    blend_info.pAttachments = &blend_attachment;
    //    blend_info.logicOpEnable = VK_FALSE;
    //    blend_info.logicOp = VK_LOGIC_OP_COPY;  // What does this do?
    //    blend_info.blendConstants[0] = 0.0f;
    //    blend_info.blendConstants[1] = 0.0f;
    //    blend_info.blendConstants[2] = 0.0f;
    //    blend_info.blendConstants[3] = 0.0f;
    //    // blend_info.blendConstants[0] = 0.2f;
    //    // blend_info.blendConstants[1] = 0.2f;
    //    // blend_info.blendConstants[2] = 0.2f;
    //    // blend_info.blendConstants[3] = 0.2f;
    //
    //    // VIEWPORT & SCISSOR
    //
    //    VkPipelineViewportStateCreateInfo viewport_info = {};
    //    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    //#ifndef __ANDROID__
    //    viewport_info.viewportCount = NUM_VIEWPORTS;
    //    dynamic_states[dynamic_info.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    //    viewport_info.scissorCount = NUM_SCISSORS;
    //    dynamic_states[dynamic_info.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
    //    viewport_info.pScissors = nullptr;
    //    viewport_info.pViewports = nullptr;
    //#else
    //    // Temporary disabling dynamic viewport on Android because some of drivers doesn't
    //    // support the feature.
    //    VkViewport viewports;
    //    viewports.minDepth = 0.0f;
    //    viewports.maxDepth = 1.0f;
    //    viewports.x = 0;
    //    viewports.y = 0;
    //    viewports.width = info.width;
    //    viewports.height = info.height;
    //    VkRect2D scissor;
    //    scissor.extent.width = info.width;
    //    scissor.extent.height = info.height;
    //    scissor.offset.x = 0;
    //    scissor.offset.y = 0;
    //    vp.viewportCount = NUM_VIEWPORTS;
    //    vp.scissorCount = NUM_SCISSORS;
    //    vp.pScissors = &scissor;
    //    vp.pViewports = &viewports;
    //#endif
    //
    //    // DEPTH
    //
    //    VkPipelineDepthStencilStateCreateInfo depth_info;
    //    depth_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    //    depth_info.pNext = nullptr;
    //    depth_info.flags = 0;
    //    depth_info.depthTestEnable = settings_.include_depth;
    //    depth_info.depthWriteEnable = settings_.include_depth;
    //    depth_info.depthCompareOp = VK_COMPARE_OP_LESS;
    //    depth_info.depthBoundsTestEnable = VK_FALSE;
    //    depth_info.minDepthBounds = 0;
    //    depth_info.maxDepthBounds = 1.0f;
    //    depth_info.stencilTestEnable = VK_FALSE;
    //    depth_info.front = {};
    //    depth_info.back = {};
    //    // dss.back.failOp = VK_STENCIL_OP_KEEP; // ARE THESE IMPORTANT !!!
    //    // dss.back.passOp = VK_STENCIL_OP_KEEP;
    //    // dss.back.compareOp = VK_COMPARE_OP_ALWAYS;
    //    // dss.back.compareMask = 0;
    //    // dss.back.reference = 0;
    //    // dss.back.depthFailOp = VK_STENCIL_OP_KEEP;
    //    // dss.back.writeMask = 0;
    //    // dss.front = ds.back;
    //
    //    // SHADER
    //
    //    // TODO: this is not great
    //    std::vector<VkPipelineShaderStageCreateInfo> stage_infos;
    //    stage_infos.push_back(vs_.info);
    //    stage_infos.push_back(fs_.info);
    //
    //    // PIPELINE
    //
    //    VkGraphicsPipelineCreateInfo pipeline_info_ = {};
    //    pipeline_info_.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    //    // shader stages
    //    pipeline_info_.stageCount = static_cast<uint32_t>(stage_infos.size());
    //    pipeline_info_.pStages = stage_infos.data();
    //    // fixed function stages
    //    pipeline_info_.pColorBlendState = &blend_info;
    //    pipeline_info_.pDepthStencilState = &depth_info;
    //    pipeline_info_.pDynamicState = &dynamic_info;
    //    pipeline_info_.pInputAssemblyState = &input_info;
    //    pipeline_info_.pMultisampleState = &multisample_info;
    //    pipeline_info_.pRasterizationState = &rast_info;
    //    pipeline_info_.pTessellationState = nullptr;
    //    pipeline_info_.pVertexInputState = &vertex_info;
    //    pipeline_info_.pViewportState = &viewport_info;
    //    // layout
    //    pipeline_info_.layout = pipeline_layout_;
    //    pipeline_info_.basePipelineHandle = VK_NULL_HANDLE;
    //    pipeline_info_.basePipelineIndex = 0;
    //    // render pass
    //    pipeline_info_.renderPass = render_pass_;
    //    pipeline_info_.subpass = 0;
    //
    //    vk::assert_success(vkCreateGraphicsPipelines(dev_, pipeline_cache_, 1, &pipeline_info_, nullptr, &pipeline_));
    //
    //    // *** LINE PIPELINE ***
    //
    //    // INPUT ASSEMBLY IS THE ONLY DIFFERENCE ????
    //
    //    pipeline_info_.subpass = 1;
    //    input_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
    //
    //    vk::assert_success(vkCreateGraphicsPipelines(dev_, pipeline_cache_, 1, &pipeline_info_, nullptr, &pipeline_line_));
}

void Guppy::destroy_pipelines() {
    vkDestroyPipeline(dev_, pipeline_, nullptr);
    vkDestroyPipeline(dev_, pipeline_line_, nullptr);
}

void Guppy::get_swapchain_image_data(const VkSwapchainKHR& swapchain) {
    uint32_t image_count;
    vk::assert_success(vkGetSwapchainImagesKHR(dev_, swapchain, &image_count, nullptr));
    // If this ever fails then you have to change the start up a ton!!!!
    assert(swapchain_image_count_ == image_count);

    VkImage* images = (VkImage*)malloc(image_count * sizeof(VkImage));
    vk::assert_success(vkGetSwapchainImagesKHR(dev_, swapchain, &image_count, images));

    for (uint32_t i = 0; i < image_count; i++) {
        SwapchainImageResource res;
        res.image = images[i];
        helpers::create_image_view(dev_, res.image, 1, format_, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, res.view);
        swapchain_image_resources_.push_back(res);
    }
    free(images);
}

void Guppy::prepare_viewport() {
    viewport_.x = 0.0f;
    viewport_.y = 0.0f;
    viewport_.width = static_cast<float>(extent_.width);
    viewport_.height = static_cast<float>(extent_.height);
    viewport_.minDepth = 0.0f;
    viewport_.maxDepth = 1.0f;

    scissor_.offset = {0, 0};
    scissor_.extent = extent_;
}

// Depends on:
//      swapchain_image_count_
//      The attachment descriptions created by create_render_pass in Scene...
//      active_scene() - render pass
//      extent_
//      swapchain_image_resources_ - view -> are an attachment
// Can change:
//      Tightly couple the attachment desriptions and the image views here...
//      Possibly move the framebuffers to the scene...
void Guppy::prepare_framebuffers(const VkSwapchainKHR& swapchain) {
    assert(framebuffers_.empty());
    framebuffers_.reserve(swapchain_image_count_);

    // There will always be a swapchain buffer
    std::vector<VkImageView> attachments;
    attachments.resize(1);

    // multi-sample (optional - needs to be same attachment index as in render pass)
    if (settings_.include_color) {
        attachments.resize(attachments.size() + 1);
        attachments[attachments.size() - 1] = color_resource_.view;
    }

    // depth
    if (settings_.include_depth) {
        attachments.resize(attachments.size() + 1);
        attachments[attachments.size() - 1] = depth_resource_.view;
    }

    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = active_scene()->activeRenderPass();
    fb_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    fb_info.pAttachments = attachments.data();
    fb_info.width = extent_.width;
    fb_info.height = extent_.height;
    fb_info.layers = 1;

    for (auto& swapchain_img_res : swapchain_image_resources_) {
        VkFramebuffer fb;
        attachments[0] = swapchain_img_res.view;
        vk::assert_success(vkCreateFramebuffer(dev_, &fb_info, NULL, &fb));
        framebuffers_.push_back(fb);
    }
}

void Guppy::create_uniform_buffer() {
    camera_.update(static_cast<float>(settings_.initial_width) / static_cast<float>(settings_.initial_height));
    auto mvp = camera_.getMVP();
    auto buffer_size = sizeof(mvp);

    ubo_resource_.count = 1;

    camera_.memory_requirements_size = helpers::create_buffer(
        dev_, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ubo_resource_.buffer, ubo_resource_.memory);

    ext::DebugMarkerSetObjectName(dev_, (uint64_t)ubo_resource_.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                  "Scene uniform buffer block");
    // Add some random tag
    ext::DebugMarkerSetObjectTag(dev_, (uint64_t)ubo_resource_.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0, sizeof(demoTag),
                                 &demoTag);

    copy_ubo_to_memory();
}

void Guppy::destroy_uniform_buffer() {
    vkDestroyBuffer(dev_, ubo_resource_.buffer, nullptr);
    vkFreeMemory(dev_, ubo_resource_.memory, nullptr);
}

void Guppy::update_ubo() {
    // Surface changed
    auto aspect = static_cast<float>(extent_.width) / static_cast<float>(extent_.height);

    // Update the camera
    auto pos_dir = InputHandler::get().getPosDir();
    auto look_dir = InputHandler::get().getLookDir();
    camera_.update(aspect, pos_dir, look_dir);

    copy_ubo_to_memory();
}

void Guppy::copy_ubo_to_memory() {
    auto mvp = camera_.getMVP();
    auto buffer_size = sizeof(mvp);

    uint8_t* pData;
    vk::assert_success(vkMapMemory(dev_, ubo_resource_.memory, 0, camera_.memory_requirements_size, 0, (void**)&pData));
    memcpy(pData, &mvp, buffer_size);
    vkUnmapMemory(dev_, ubo_resource_.memory);

    ubo_resource_.info.buffer = ubo_resource_.buffer;
    ubo_resource_.info.offset = 0;
    ubo_resource_.info.range = buffer_size;
}

void Guppy::create_render_pass(bool include_depth, bool include_color, bool clear, VkImageLayout finalLayout) {
    // std::vector<VkAttachmentDescription> attachments;

    //// COLOR ATTACHMENT (SWAP-CHAIN)
    // VkAttachmentReference color_reference = {};
    // VkAttachmentDescription attachemnt = {};
    // attachemnt = {};
    // attachemnt.format = format_;
    // attachemnt.samples = VK_SAMPLE_COUNT_1_BIT;
    // attachemnt.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;  // clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    // attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    // attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    // attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // attachemnt.finalLayout = finalLayout;
    // attachemnt.flags = 0;
    // attachments.push_back(attachemnt);
    //// REFERENCE
    // color_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);
    // color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //// COLOR ATTACHMENT RESOLVE (MULTI-SAMPLE)
    // VkAttachmentReference color_resolve_reference = {};
    // if (include_color) {
    //    attachemnt = {};
    //    attachemnt.format = format_;
    //    attachemnt.samples = num_samples_;
    //    attachemnt.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    //    attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    //    attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    //    attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    //    attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //    attachemnt.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //    attachemnt.flags = 0;
    //    // REFERENCE
    //    color_resolve_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);  // point to swapchain attachment
    //    color_resolve_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    //    attachments.push_back(attachemnt);
    //    color_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);  // point to multi-sample attachment
    //}

    //// DEPTH ATTACHMENT
    // VkAttachmentReference depth_reference = {};
    // if (include_depth) {
    //    attachemnt = {};
    //    attachemnt.format = depth_resource_.format;
    //    attachemnt.samples = num_samples_;
    //    attachemnt.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    //    // This was "don't care" in the sample and that makes more sense to
    //    // me. This obvious is for some kind of stencil operation.
    //    attachemnt.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    //    attachemnt.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    //    attachemnt.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    //    attachemnt.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    //    attachemnt.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    //    attachemnt.flags = 0;
    //    attachments.push_back(attachemnt);
    //    // REFERENCE
    //    depth_reference.attachment = static_cast<uint32_t>(attachments.size() - 1);
    //    depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    //}

    // VkSubpassDescription subpass = {};
    // subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // subpass.inputAttachmentCount = 0;
    // subpass.pInputAttachments = nullptr;
    // subpass.colorAttachmentCount = 1;
    // subpass.pColorAttachments = &color_reference;
    // subpass.pResolveAttachments = include_color ? &color_resolve_reference : nullptr;
    // subpass.pDepthStencilAttachment = include_depth ? &depth_reference : nullptr;
    // subpass.preserveAttachmentCount = 0;
    // subpass.pPreserveAttachments = nullptr;

    // std::vector<VkSubpassDescription> subpasses;
    // subpasses.push_back(subpass);

    //// Line drawing subpass (just need an identical subpass for now)
    // subpasses.push_back(subpass);

    ////// TODO: used for waiting in draw (figure this out... what is the VK_SUBPASS_EXTERNAL one?)
    //// VkSubpassDependency dependency = {};
    //// dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    //// dependency.dstSubpass = 0;
    //// dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //// dependency.srcAccessMask = 0;
    //// dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //// dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    //// std::vector<VkSubpassDependency> dependencies;
    //// dependencies.push_back(dependency);

    ////// From poly to line
    // VkSubpassDependency dependency = {};
    // dependency.srcSubpass = 0;
    // dependency.dstSubpass = 1;
    // dependency.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    // dependency.srcAccessMask = 0;
    // dependency.dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    // dependency.dstAccessMask = 0;

    // VkRenderPassCreateInfo rp_info = {};
    // rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    // rp_info.pNext = nullptr;
    // rp_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    // rp_info.pAttachments = attachments.data();
    // rp_info.subpassCount = static_cast<uint32_t>(subpasses.size());
    // rp_info.pSubpasses = subpasses.data();
    // rp_info.dependencyCount = 1;
    // rp_info.pDependencies = &dependency;
    //// rp_info.dependencyCount = 0;
    //// rp_info.pDependencies = nullptr;

    // vk::assert_success(vkCreateRenderPass(dev_, &rp_info, nullptr, &render_pass_));
}

void Guppy::destroy_render_pass() { vkDestroyRenderPass(dev_, render_pass_, nullptr); }

void Guppy::destroy_shader_modules() {
    vkDestroyShaderModule(dev_, fs_.module, nullptr);
    vkDestroyShaderModule(dev_, vs_.module, nullptr);
}

void Guppy::create_descriptor_set_layout(const VkDescriptorSetLayoutCreateFlags descSetLayoutCreateFlags) {
    if (use_push_constants_) return;

    // UNIFORM BUFFER
    VkDescriptorSetLayoutBinding ubo_binding = {};
    ubo_binding.binding = 0;
    ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_binding.descriptorCount = 1;
    ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_binding.pImmutableSamplers = nullptr;  // Optional

    // TODO: move texture things into a texture place

    // TEXTURE
    VkDescriptorSetLayoutBinding tex_binding = {};
    tex_binding.binding = 1;
    tex_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    tex_binding.descriptorCount = 1;
    tex_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    tex_binding.pImmutableSamplers = nullptr;  // Optional

    // LAYOUT
    std::array<VkDescriptorSetLayoutBinding, 2> layout_bindings = {ubo_binding, tex_binding};
    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.pNext = nullptr;
    layout_info.flags = descSetLayoutCreateFlags;
    layout_info.bindingCount = static_cast<uint32_t>(layout_bindings.size());
    layout_info.pBindings = layout_bindings.data();

    // TODO: are all these layouts necessary?
    desc_set_layouts_.resize(swapchain_image_count_);
    for (auto& layout : desc_set_layouts_) {
        vk::assert_success(vkCreateDescriptorSetLayout(dev_, &layout_info, nullptr, &layout));
    }
}

void Guppy::create_pipeline_layout() {
    VkPushConstantRange push_const_range = {};

    VkPipelineLayoutCreateInfo pipeline_layout_info = {};
    pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    if (use_push_constants_) {
        throw std::runtime_error("not handled!");
        push_const_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        push_const_range.offset = 0;
        push_const_range.size = sizeof(ShaderParamBlock);

        pipeline_layout_info.pushConstantRangeCount = 1;
        pipeline_layout_info.pPushConstantRanges = &push_const_range;
    } else {
        pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(desc_set_layouts_.size());
        pipeline_layout_info.pSetLayouts = desc_set_layouts_.data();
    }

    vk::assert_success(vkCreatePipelineLayout(dev_, &pipeline_layout_info, nullptr, &pipeline_layout_));
}

void Guppy::destroy_descriptor_and_pipeline_layouts() {
    for (auto& layout : desc_set_layouts_) vkDestroyDescriptorSetLayout(dev_, layout, nullptr);
    vkDestroyPipelineLayout(dev_, pipeline_layout_, nullptr);
}

void Guppy::create_frame_data(int count) {
    const MyShell::Context& ctx = shell_->context();

    frame_data_.resize(count);

    create_fences();

    if (!use_push_constants_) {
        // create_buffers(); // TODO: this is their ubo implementation. It is worth figuring out !!!!!
        // create_buffer_memory();
        create_color_resources();
        create_depth_resources();
        update_ubo();
        // create_descriptor_pool();
        // create_descriptor_sets(); // Not sure how to make this work here... pipeline relies on this info
    }

    frame_data_index_ = 0;
}

void Guppy::create_fences() {
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& data : frame_data_) vk::assert_success(vkCreateFence(dev_, &fence_info, nullptr, &data.fence));
}

void Guppy::create_descriptor_sets(bool use_texture) {
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = desc_pool_;
    alloc_info.descriptorSetCount = static_cast<uint32_t>(swapchain_image_count_);
    alloc_info.pSetLayouts = desc_set_layouts_.data();

    desc_sets_.resize(swapchain_image_count_);
    vk::assert_success(vkAllocateDescriptorSets(dev_, &alloc_info, desc_sets_.data()));

    for (size_t i = 0; i < swapchain_image_count_; i++) {
        std::array<VkWriteDescriptorSet, 2> writes = {};

        writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[0].dstSet = desc_sets_[i];
        writes[0].dstBinding = 0;
        writes[0].dstArrayElement = 0;
        writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writes[0].descriptorCount = 1;
        writes[0].pBufferInfo = &ubo_resource_.info;

        writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[1].dstSet = desc_sets_[i];
        writes[1].dstBinding = 1;
        writes[1].dstArrayElement = 0;
        writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[1].descriptorCount = static_cast<uint32_t>(textures_.size());  // 1;

        // TODO: fix this
        std::vector<VkDescriptorImageInfo> imgDescInfos;
        for (auto& texture : textures_) imgDescInfos.push_back(texture.imgDescInfo);

        writes[1].pImageInfo = imgDescInfos.data();

        vkUpdateDescriptorSets(dev_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void Guppy::destroy_frame_data() {
    if (!use_push_constants_) {
        // vk::DestroyDescriptorPool(dev_, desc_pool_, nullptr);

        // vkUnmapMemory(dev_, frame_data_mem_);
        // vkFreeMemory(dev_, frame_data_mem_, nullptr);

        for (auto& data : frame_data_) vkDestroyBuffer(dev_, data.buf, nullptr);
    }

    // for (auto cmd_pool : worker_cmd_pools_) vk::DestroyCommandPool(dev_, cmd_pool, nullptr);
    // worker_cmd_pools_.clear();
    // vk::DestroyCommandPool(dev_, primary_cmd_pool_, nullptr);

    for (auto& data : frame_data_) vkDestroyFence(dev_, data.fence, nullptr);

    frame_data_.clear();

    // *** NEW ***

    destroy_color_resources();
    destroy_depth_resources();
}

// TODO: See if using context directly simplifies the signatures here...
void Guppy::create_color_resources() {
    helpers::create_image(dev_, CmdBufHandler::getUniqueQueueFamilies(true, false, true), extent_.width, extent_.height, 1,
                          shell_->context().num_samples, format_, VK_IMAGE_TILING_OPTIMAL,
                          VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, color_resource_.image, color_resource_.memory);

    helpers::create_image_view(dev_, color_resource_.image, 1, format_, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D,
                               color_resource_.view);

    helpers::transition_image_layout(CmdBufHandler::graphics_cmd(), color_resource_.image, 1, format_, VK_IMAGE_LAYOUT_UNDEFINED,
                                     VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                     VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);

    // Name some objects for debugging
    ext::DebugMarkerSetObjectName(dev_, (uint64_t)color_resource_.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                  "Guppy color framebuffer");
}

void Guppy::destroy_color_resources() {
    // TODO: what about the format member?
    vkDestroyImageView(dev_, color_resource_.view, nullptr);
    vkDestroyImage(dev_, color_resource_.image, nullptr);
    vkFreeMemory(dev_, color_resource_.memory, nullptr);
}

// TODO: See if using context directly simplifies the signatures here...
void Guppy::create_depth_resources() {
    VkFormatProperties props;
    VkImageTiling tiling;
    vkGetPhysicalDeviceFormatProperties(physical_dev_, depth_resource_.format, &props);
    if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        tiling = VK_IMAGE_TILING_LINEAR;
    } else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        tiling = VK_IMAGE_TILING_OPTIMAL;
    } else {
        /* Try other depth formats? */
        throw std::runtime_error(("depth_format Unsupported.\n"));
    }

    helpers::create_image(dev_, CmdBufHandler::getUniqueQueueFamilies(true, false, true), extent_.width, extent_.height, 1,
                          shell_->context().num_samples, depth_resource_.format, tiling,
                          VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_resource_.image,
                          depth_resource_.memory);

    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (helpers::has_stencil_component(depth_resource_.format)) {
        aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    helpers::create_image_view(dev_, depth_resource_.image, 1, depth_resource_.format, aspectFlags, VK_IMAGE_VIEW_TYPE_2D,
                               depth_resource_.view);

    helpers::transition_image_layout(CmdBufHandler::graphics_cmd(), depth_resource_.image, 1, depth_resource_.format,
                                     VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                     VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);

    // Name some objects for debugging
    ext::DebugMarkerSetObjectName(dev_, (uint64_t)depth_resource_.image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                  "Guppy depth framebuffer");
}

void Guppy::destroy_depth_resources() {
    // TODO: what about the format member?
    vkDestroyImageView(dev_, depth_resource_.view, nullptr);
    vkDestroyImage(dev_, depth_resource_.image, nullptr);
    vkFreeMemory(dev_, depth_resource_.memory, nullptr);
}

void Guppy::destroy_ubo_resources() {
    vkDestroyBuffer(dev_, ubo_resource_.buffer, nullptr);
    vkFreeMemory(dev_, ubo_resource_.memory, nullptr);
}

void Guppy::create_draw_cmds() {
    draw_cmds_.resize(swapchain_image_count_);

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = CmdBufHandler::graphics_cmd_pool();
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = static_cast<uint32_t>(draw_cmds_.size());

    vk::assert_success(vkAllocateCommandBuffers(dev_, &alloc_info, draw_cmds_.data()));

    for (size_t i = 0; i < draw_cmds_.size(); i++) {
        // RECORD BUFFER

        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        begin_info.pInheritanceInfo = nullptr;  // Optional

        vk::assert_success(vkBeginCommandBuffer(draw_cmds_[i], &begin_info));

        VkRenderPassBeginInfo render_pass_info = {};
        render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        render_pass_info.renderPass = render_pass_;
        render_pass_info.framebuffer = framebuffers_[i];
        render_pass_info.renderArea.offset = {0, 0};
        render_pass_info.renderArea.extent = extent_;

        /*
            Need to pad this here because:

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

        // 1 RENDER PASS

        vkCmdBeginRenderPass(draw_cmds_[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        // COMMON

        VkBuffer vertex_buffers[] = {vertex_res_.buffer};
        VkDeviceSize offsets[] = {0};

        vkCmdSetViewport(draw_cmds_[i], 0, 1, &viewport_);
        vkCmdSetScissor(draw_cmds_[i], 0, 1, &scissor_);
        vkCmdBindDescriptorSets(draw_cmds_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1, &desc_sets_[i], 0, nullptr);

        // SUBPASS 0
        // ** POLYS **

        // vkCmdBindPipeline(draw_cmds_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);
        // vkCmdBindVertexBuffers(draw_cmds_[i], 0, 1, vertex_buffers, offsets);
        // vkCmdBindIndexBuffer(draw_cmds_[i], index_res_.buffer, 0, VK_INDEX_TYPE_UINT32);
        // vkCmdDrawIndexed(draw_cmds_[i], model_.getIndexSize(), 1, 0, model_.m_linesCount, 0);

        // vkCmdNextSubpass(draw_cmds_[i], VK_SUBPASS_CONTENTS_INLINE);
        //// SUBPASS 1
        //// ** LINES **

        // vkCmdBindPipeline(draw_cmds_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_line_);
        // vkCmdBindVertexBuffers(draw_cmds_[i], 0, 1, vertex_buffers, offsets);
        // vkCmdDraw(draw_cmds_[i],
        //          model_.m_linesCount,  // vertexCount
        //          1,                    // instanceCount - Used for instanced rendering, use 1 if you're not doing that.
        //          0,  // firstVertex - Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
        //          0   // firstInstance - Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
        //);

        vkCmdEndRenderPass(draw_cmds_[i]);

        vk::assert_success(vkEndCommandBuffer(draw_cmds_[i]));
    }
}

void Guppy::create_scenes() {
    auto ctx = shell_->context();

    auto scene1 = std::make_unique<Scene>(ctx, settings(), ubo_resource_);

    // meshes
    // Plane p("..\\..\\..\\images\\texture.jpg");
    // std::unique_ptr<Mesh> p1 = std::make_unique<Plane>();
    // scene1->addMesh(ctx, cmd_data_, ubo_resource_.info, std::move(p1)); // add mesh tries to load it...

    scenes_.push_back(std::move(scene1));
    active_scene_index_ = 0;

    // if (loadDefault) {
    // model_.loadAxes();
    // model_.loadDefault();
    //} else {
    //    // UP_VECTOR = glm::vec3(0.0f, 0.0f, 1.0f);
    //    tex_path = CHALET_TEXTURE_PATH;
    //    model_.loadChalet();
    //}

    // VkCommandBuffer cmd1, cmd2;
    // StagingBufferHandler::get().begin_command_recording(cmd);

    // StagingBufferResource stg_res;
    // std::vector<StagingBufferResource> staging_resources;
    // std::vector<uint32_t> queueFamilyIndices = {cmd_data_.graphics_queue_family};
    // if (cmd_data_.graphics_queue_family != cmd_data_.transfer_queue_family)
    //    queueFamilyIndices.push_back(cmd_data_.transfer_queue_family);

    // stg_res = {};
    // tex_path = "..\\..\\..\\images\\chalet.jpg";
    // textures_.emplace_back(Texture::createTexture(ctx, stg_res, transfer_cmd(), graphics_cmd(), queueFamilyIndices, tex_path));
    // staging_resources.emplace_back(stg_res);

    // stg_res = {};
    // tex_path = "..\\..\\..\\images\\texture.jpg";
    // textures_.emplace_back(Texture::createTexture(ctx, stg_res, transfer_cmd(), graphics_cmd(), queueFamilyIndices, tex_path));
    // staging_resources.emplace_back(stg_res);

    // VkPipelineStageFlags wait_stages[] = {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};
    // StagingBufferHandler::get().end_recording_and_submit(staging_resources.data(), staging_resources.size(), transfer_cmd(),
    //                                                     cmd_data_.graphics_queue_family, &graphics_cmd(), wait_stages,
    //                                                     StagingBufferHandler::END_TYPE::RESET);

    assert(active_scene_index_ >= 0);
}

void Guppy::destroy_textures() {
    for (auto& tex : textures_) {
        vkDestroySampler(dev_, tex.sampler, nullptr);
        vkDestroyImageView(dev_, tex.view, nullptr);
        vkDestroyImage(dev_, tex.image, nullptr);
        vkFreeMemory(dev_, tex.memory, nullptr);
    }
}
