

//#define STB_IMAGE_IMPLEMENTATION
//#include <stb_image.h>
//#define GLFW_INCLUDE_VULKAN
//#include <GLFW/glfw3.h>
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
//#include <glm/gtc/matrix_transform.hpp>

//#include <algorithm>
//#include <chrono>
//#include <cstdlib>
//#include <functional>
//#include <fstream>
//#include <iostream>
//#include <map>
//#include <stdexcept>
//#include <thread>
//#include <utility>
//#include <vector>

#include "Constants.h"
#include "EventHandlers.h"
#include "Extensions.h"
#include "FileLoader.h"
#include "Guppy.h"
#include "Helpers.h"
#include "StagingBufferHandler.h"
#include "Vertex.h"

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

}  // namespace

Guppy::Guppy(const std::vector<std::string>& args)
    : Game("Guppy", args),
      // multithread_(true),
      // use_push_constants_(false),
      // sim_paused_(false),
      // sim_fade_(false),
      // sim_(5000),
      // camera_(2.5f),
      // frame_data_(),
      render_pass_clear_value_({{0.0f, 0.1f, 0.2f, 1.0f}}),
      render_pass_begin_info_(),
      primary_cmd_begin_info_(),
      primary_cmd_submit_info_(),
      sample_shading_supported_(false),
      depth_resource_(),
      info() {
    // for (auto it = args.begin(); it != args.end(); ++it) {
    //    if (*it == "-s")
    //        multithread_ = false;
    //    else if (*it == "-p")
    //        use_push_constants_ = true;
    //}

    // init_workers();
}

Guppy::~Guppy() {}

void Guppy::attach_shell(MyShell& sh) {
    Game::attach_shell(sh);

    const MyShell::Context& ctx = sh.context();
    physical_dev_ = ctx.physical_dev;
    dev_ = ctx.dev;
    queue_ = ctx.game_queue;
    queue_family_ = ctx.game_queue_family;
    format_ = ctx.format.format;
    // * Data from MyShell
    swapchain_image_count_ = ctx.image_count;
    depth_resource_.format = ctx.depth_format;
    cmd_data_.graphics_queue_family = ctx.graphics_index;
    cmd_data_.present_queue_family = ctx.present_index;
    cmd_data_.transfer_queue_family = ctx.transfer_index;
    cmd_data_.queues = ctx.queues;
    cmd_data_.mem_props = ctx.physical_dev_props[ctx.physical_dev_index].memory_properties;
    physical_dev_props_ = ctx.physical_dev_props[ctx.physical_dev_index].properties;

    determine_sample_count(ctx.physical_dev_props[ctx.physical_dev_index]);

    if (use_push_constants_ && sizeof(ShaderParamBlock) > physical_dev_props_.limits.maxPushConstantsSize) {
        shell_->log(MyShell::LOG_WARN, "cannot enable push constants");
        use_push_constants_ = false;
    }

    mem_flags_.reserve(cmd_data_.mem_props.memoryTypeCount);
    for (uint32_t i = 0; i < cmd_data_.mem_props.memoryTypeCount; i++)
        mem_flags_.push_back(cmd_data_.mem_props.memoryTypes[i].propertyFlags);

    // meshes_ = new Meshes(dev_, mem_flags_);

    // Not sure how wise something like this is.
    create_command_pools_and_buffers();
    // this was the only way I could think of for init.
    StagingBufferHandler::get(&sh, dev_, &cmd_data_);
    // begin recording commands?
    for (auto& cmd : cmd_data_.cmds) execute_begin_command_buffer(cmd);

    create_model();
    create_input_assembly_data();
    create_uniform_buffer();  // TODO: ugh!

    create_render_pass(settings_.include_color, settings_.include_depth);
    create_shader_modules();
    create_descriptor_set_layout();
    create_descriptor_pool();
    create_descriptor_sets();
    create_pipeline_layout();
    create_pipeline_cache();
    create_pipeline();

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
    // if (multithread_) {
    //    for (auto &worker : workers_) worker->stop();
    //}

    // destroy_frame_data();

    // vk::DestroyPipeline(dev_, pipeline_, nullptr);
    // vk::DestroyPipelineLayout(dev_, pipelineLayout_, nullptr);
    // if (!use_push_constants_) vk::DestroyDescriptorSetLayout(dev_, descSets_layout_, nullptr);
    // vk::DestroyShaderModule(dev_, fs_, nullptr);
    // vk::DestroyShaderModule(dev_, vs_, nullptr);
    // vk::DestroyRenderPass(dev_, renderPass_, nullptr);

    // delete meshes_;

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

    create_draw_cmds();
}

void Guppy::detach_swapchain() {
    for (auto fb : framebuffers_) vkDestroyFramebuffer(dev_, fb, nullptr);
    for (auto view : image_views_) vkDestroyImageView(dev_, view, nullptr);

    framebuffers_.clear();
    image_views_.clear();
    images_.clear();
}

void Guppy::on_key(Key key) {
    // switch (key) {
    // case KEY_SHUTDOWN:
    // case KEY_ESC:
    //    shell_->quit();
    //    break;
    // case KEY_UP:
    //    camera_.eye_pos -= glm::vec3(0.05f);
    //    update_camera();
    //    break;
    // case KEY_DOWN:
    //    camera_.eye_pos += glm::vec3(0.05f);
    //    update_camera();
    //    break;
    // case KEY_SPACE:
    //    sim_paused_ = !sim_paused_;
    //    break;
    // case KEY_F:
    //    sim_fade_ = !sim_fade_;
    //    break;
    // default:
    //    break;
    //}
}

void Guppy::on_frame(float frame_pred) {
    auto& data = frame_data_[frame_data_index_];

    // wait for the last submission since we reuse frame data
    vk::assert_success(vkWaitForFences(dev_, 1, &data.fence, true, UINT64_MAX));
    vk::assert_success(vkResetFences(dev_, 1, &data.fence));

    const MyShell::BackBuffer& back = shell_->context().acquired_back_buffer;

    update_camera();

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.pWaitSemaphores = &back.acquire_semaphore;
    submit_info.commandBufferCount = 1;  // * Just the draw command
    submit_info.pCommandBuffers = &draw_cmds_[frame_data_index_];
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &back.render_semaphore;

    VkResult res = vkQueueSubmit(graphics_queue(), 1, &submit_info, data.fence);

    frame_data_index_ = (frame_data_index_ + 1) % frame_data_.size();

    (void)res;
}

void Guppy::on_tick() {
    // if (sim_paused_) return;

    // for (auto &worker : workers_) worker->update_simulation();
}

// void Guppy::run() {
//    // initWindow();
//    // initVulkan();
//    // mainLoop();
//    // cleanup();
//}

// void Guppy::init_vulkan() {
//    // createInstance();X
//    // setupDebugCallback();X
//    // createSurface();X
//    // pickPhysicalDevice();X
//    // createLogicalDevice();X
//    // createSwapChain();X
//    // createSwapChainImageViews();X
//    // createRenderPass();X
//    // createDescriptorSetLayout();X
//    // createGraphicsPipeline();
//    // createCommandPools();X
//    // createColorResources();X
//    // createDepthResources();X
//    // createFramebuffers();X
//    // createModel();X
//    // createVertexBuffer();X
//    // createIndexBuffer();
//    // createUniformBuffers();X
//    // createDescriptorPool();X
//    // createDescriptorSets();X
//    // createGraphicsCommandBuffers();X
//    // createSyncObjects();X
//}

void Guppy::my_init_vk() {
    VkResult U_ASSERT_ONLY res = {};
    // const bool depthPresent = true;

    // info.enableSampleShading = ENABLE_SAMPLE_SHADING;
    // init_window_size(info, WIDTH, HEIGHT);

    // if (ENABLE_VALIDATION_LAYERS) {
    //    init_global_layer_properties(info, VALIDATION_LAYERS);
    //    init_instance_layer_names(info, VALIDATION_LAYERS);
    //    init_instance_extension_names(info, INSTANCE_EXTENSIONS);
    //} else {
    //    init_global_layer_properties(info);
    //    init_instance_layer_names(info);
    //    init_instance_extension_names(info);
    //}
    // init_device_extension_names(info);
    // init_instance(info, APP_SHORT_NAME);
    // init_validation_layers(info, debug_callback, CreateDebugUtilsMessengerEXT);
    // init_surface(info);
    // init_enumerate_devices(info);
    // pick_device(info);
    // init_device(info);
    // init_swapchain_extension(info);

    // init_command_pools(info);
    // init_command_buffers(info);
    for (auto& cmd : info.cmds) {
        execute_begin_command_buffer(cmd);
    }
    // init_device_queues(info);
    // init_swapchain(info);
    // init_depth_buffer(info);
    // init_color_buffer(info);
    // init_uniform_buffer(info);
    // init_descriptor_set_layout(info);
    // init_pipelineLayout(info);
    // init_renderpass(info, depthPresent);

    //// Relative to CMake being run in a "build" directory in the root of the repo like VulkanSamples
    // auto vertShaderText = FileLoader::read_file("..\\..\\..\\Guppy\\src\\shaders\\shader.vert");
    // auto fragShaderText = FileLoader::read_file("..\\..\\..\\Guppy\\src\\shaders\\shader.frag");
    // init_shaders(info, vertShaderText.data(), fragShaderText.data());

    // init_framebuffers(info, depthPresent);

    // createModel(info);
    // create_vertex_data(info);
    // create_ndex_data(info);

    init_descriptor_pool(info, true);
    init_descriptor_set(info, true);
    // init_pipeline_cache(info);
    // create_pipeline();

    // for (auto& cmd : info.cmds) {
    //    execute_end_command_buffer(cmd);
    //}
    // for (size_t i = 0; i < info.cmds.size(); i++) {
    //    execute_submit_queue_fenced(info, info.queues[i], info.cmds[i]);
    //}

    // submit the queues
    // Texture::submitQueues(shell_);

    // because above is fenced and waited on this should work for now
    destroy_staging_buffers(info);
}

void Guppy::my_cleanup_vk() {
    destroy_pipeline();
    destroy_pipeline_cache();
    destroy_descriptor_pool(info);
    destroy_vertexBuffer(info);
    destroy_textures();
    destroy_framebuffers(info);
    destroy_shaders(info);
    destroy_renderpass(info);
    destroy_descriptor_and_pipelineLayouts();
    destroy_uniform_buffer(info);
    destroy_depth_buffer(info);
    destroy_color_buffer(info);
    destroy_swapchain(info);
    destroy_command_buffers();
    destroy_command_pools();
    destroy_device(info);
    destroy_window(info);
    destroy_validation_layers(info);
    destroy_instance(info);

    // cleanupSwapChain();

    // vkDestroySampler(m_device, m_textureSampler, nullptr);
    // vkDestroyImageView(m_device, m_textureImageView, nullptr);

    // vkDestroyImage(m_device, m_textureImage, nullptr);
    // vkFreeMemory(m_device, m_textureImageMemory, nullptr);

    // vkDestroyDescriptorPool(m_device, m_descriptorPool, nullptr);

    // vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

    // for (size_t i = 0; i < m_swapChainImages.size(); i++)
    //{
    //    vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
    //    vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
    //}

    // vkDestroyBuffer(m_device, m_indexBuffer, nullptr);
    // vkFreeMemory(m_device, m_indexBufferMemory, nullptr);

    // vkDestroyBuffer(m_device, m_vertexBuffer, nullptr);
    // vkFreeMemory(m_device, m_vertexBufferMemory, nullptr);

    // for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
    //{
    //    vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
    //    vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
    //    vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
    //}

    // vkDestroyCommandPool(m_device, m_graphicsCommandPool, nullptr);
    // vkDestroyCommandPool(m_device, m_transferCommandPool, nullptr);

    // vkDestroyDevice(m_device, nullptr); // not sure of order here (callback is in the device)

    // if (ENABLE_VALIDATION_LAYERS)
    //{
    //    DestroyDebugUtilsMessengerEXT(m_instance, m_callback, nullptr);
    //}

    // vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    // vkDestroyInstance(m_instance, nullptr);

    // glfwDestroyWindow(m_window);

    // glfwTerminate();
}

void Guppy::create_pipeline_cache() {
    VkPipelineCacheCreateInfo pipeline_cache_info = {};
    pipeline_cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipeline_cache_info.initialDataSize = 0;
    pipeline_cache_info.pInitialData = nullptr;
    vk::assert_success(vkCreatePipelineCache(dev_, &pipeline_cache_info, nullptr, &pipeline_cache_));
}

void Guppy::destroy_pipeline_cache() { vkDestroyPipelineCache(dev_, pipeline_cache_, nullptr); }

void Guppy::create_pipeline() {
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
    auto bindingDescription = Vertex::getBindingDescription();
    vertex_info.vertexBindingDescriptionCount = 1;
    vertex_info.pVertexBindingDescriptions = &bindingDescription;
    // attributes
    auto attributeDescriptions = Vertex::getAttributeDescriptions();
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
    multisample_info.rasterizationSamples = num_samples_;
    multisample_info.sampleShadingEnable =
        settings_.enable_sample_shading_;  // enable sample shading in the pipeline (sampling for fragment interiors)
    multisample_info.minSampleShading =
        settings_.enable_sample_shading_ ? MIN_SAMPLE_SHADING : 0.0f;  // min fraction for sample shading; closer to one is smooth
    multisample_info.pSampleMask = nullptr;                            // Optional
    multisample_info.alphaToCoverageEnable = VK_FALSE;                 // Optional
    multisample_info.alphaToOneEnable = VK_FALSE;                      // Optional

    // BLENDING

    VkPipelineColorBlendAttachmentState blend_attachment = {};
    blend_attachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blend_attachment.blendEnable = VK_FALSE;
    blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;              // Optional
    blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;              // Optional
    blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional
    blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // Optional
    blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // Optional

    // common setup
    // colorBlendAttachment.blendEnable = VK_TRUE;
    // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

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
    depth_info.depthTestEnable = settings_.include_depth;
    depth_info.depthWriteEnable = settings_.include_depth;
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
    stage_infos.push_back(vs_.info);
    stage_infos.push_back(fs_.info);

    // PIPELINE

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // shader stages
    pipeline_info.stageCount = static_cast<uint32_t>(stage_infos.size());
    pipeline_info.pStages = stage_infos.data();
    // fixed function stages
    pipeline_info.pColorBlendState = &blend_info;
    pipeline_info.pDepthStencilState = &depth_info;
    pipeline_info.pDynamicState = &dynamic_info;
    pipeline_info.pInputAssemblyState = &input_info;
    pipeline_info.pMultisampleState = &multisample_info;
    pipeline_info.pRasterizationState = &rast_info;
    pipeline_info.pTessellationState = nullptr;
    pipeline_info.pVertexInputState = &vertex_info;
    pipeline_info.pViewportState = &viewport_info;
    // layout
    pipeline_info.layout = pipeline_layout_;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = 0;
    // render pass
    pipeline_info.renderPass = render_pass_;
    pipeline_info.subpass = 0;

    vk::assert_success(vkCreateGraphicsPipelines(dev_, pipeline_cache_, 1, &pipeline_info, nullptr, &pipeline_));
}

void Guppy::destroy_pipeline() { vkDestroyPipeline(dev_, pipeline_, nullptr); }

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
        create_image_view(dev_, res.image, 1, format_, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, res.view);
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
    fb_info.renderPass = render_pass_;
    fb_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    fb_info.pAttachments = attachments.data();
    fb_info.width = extent_.width;
    fb_info.height = extent_.height;
    fb_info.layers = 1;

    for (auto& res : swapchain_image_resources_) {
        VkFramebuffer fb;
        attachments[0] = res.view;
        vk::assert_success(vkCreateFramebuffer(dev_, &fb_info, NULL, &fb));
        framebuffers_.push_back(fb);
    }
}

void Guppy::determine_sample_count(const MyShell::PhysicalDeviceProperties& props) {
    //// TODO: OPTION (FEATURE BASED)
    VkSampleCountFlags counts =
        min(props.properties.limits.framebufferColorSampleCounts, props.properties.limits.framebufferDepthSampleCounts);
    num_samples_ = VK_SAMPLE_COUNT_1_BIT;
    // return the highest possible one for now
    if (counts & VK_SAMPLE_COUNT_64_BIT)
        num_samples_ = VK_SAMPLE_COUNT_64_BIT;
    else if (counts & VK_SAMPLE_COUNT_32_BIT)
        num_samples_ = VK_SAMPLE_COUNT_32_BIT;
    else if (counts & VK_SAMPLE_COUNT_16_BIT)
        num_samples_ = VK_SAMPLE_COUNT_16_BIT;
    else if (counts & VK_SAMPLE_COUNT_8_BIT)
        num_samples_ = VK_SAMPLE_COUNT_8_BIT;
    else if (counts & VK_SAMPLE_COUNT_4_BIT)
        num_samples_ = VK_SAMPLE_COUNT_4_BIT;
    else if (counts & VK_SAMPLE_COUNT_2_BIT)
        num_samples_ = VK_SAMPLE_COUNT_2_BIT;
}

void Guppy::create_uniform_buffer() {
    // TODO: this is wrong also
    Camera::default_perspective(camera_, static_cast<float>(settings_.initial_width), static_cast<float>(settings_.initial_height));

    auto buffer_size = sizeof(camera_.mvp);
    camera_.memory_requirements_size = create_buffer(dev_, cmd_data_.mem_props, buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                                     ubo_resource_.buffer, ubo_resource_.memory);

    update_camera();  // TODO: yikes!
}

void Guppy::update_camera() {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::ratio<3, 1>>(currentTime - startTime).count();

    camera_.model = glm::rotate(glm::mat4(1.0f),             // matrix
                                time * glm::radians(90.0f),  // angle
                                glm::vec3(0.0f, 0.0f, 1.0f)  // vector
    );
    camera_.mvp = camera_.clip * camera_.proj * camera_.view * camera_.model;

    uint8_t* pData;
    vk::assert_success(vkMapMemory(dev_, ubo_resource_.memory, 0, camera_.memory_requirements_size, 0, (void**)&pData));
    memcpy(pData, &camera_.mvp, sizeof(camera_.mvp));
    vkUnmapMemory(dev_, ubo_resource_.memory);

    ubo_resource_.info.buffer = ubo_resource_.buffer;
    ubo_resource_.info.offset = 0;
    ubo_resource_.info.range = sizeof(camera_.mvp);
}

void Guppy::create_render_pass(bool include_depth, bool include_color, bool clear, VkImageLayout finalLayout) {
    std::vector<VkAttachmentDescription> attachments;

    // COLOR ATTACHMENT (SWAP-CHAIN)
    VkAttachmentReference color_reference = {};
    VkAttachmentDescription attachemnt = {};
    attachemnt = {};
    attachemnt.format = format_;
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
    if (include_color) {
        attachemnt = {};
        attachemnt.format = format_;
        attachemnt.samples = num_samples_;
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
    if (include_depth) {
        attachemnt = {};
        attachemnt.format = depth_resource_.format;
        attachemnt.samples = num_samples_;
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

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = nullptr;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_reference;
    subpass.pResolveAttachments = include_color ? &color_resolve_reference : nullptr;
    subpass.pDepthStencilAttachment = include_depth ? &depth_reference : nullptr;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = nullptr;

    // TODO: used for waiting in draw (figure this out)
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo rp_info = {};
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.pNext = nullptr;
    rp_info.attachmentCount = static_cast<uint32_t>(attachments.size());
    rp_info.pAttachments = attachments.data();
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    rp_info.dependencyCount = 1;
    rp_info.pDependencies = &dependency;

    vk::assert_success(vkCreateRenderPass(dev_, &rp_info, nullptr, &render_pass_));
}

void Guppy::create_shader_modules() {
    // Relative to CMake being run in a "build" directory in the root of the repo like VulkanSamples

    auto vertShaderText = FileLoader::read_file("..\\..\\..\\Guppy\\src\\shaders\\shader.vert");
    auto fragShaderText = FileLoader::read_file("..\\..\\..\\Guppy\\src\\shaders\\shader.frag");
    bool retVal;

    // If no shaders were submitted, just return
    if (!(vertShaderText.data() || fragShaderText.data())) return;

    init_glslang();
    VkShaderModuleCreateInfo module_info;
    VkPipelineShaderStageCreateInfo stage_info;

    if (vertShaderText.data()) {
        std::vector<unsigned int> vtx_spv;
        retVal = GLSLtoSPV(VK_SHADER_STAGE_VERTEX_BIT, vertShaderText.data(), vtx_spv);
        assert(retVal);

        module_info = {};
        module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_info.codeSize = vtx_spv.size() * sizeof(unsigned int);
        module_info.pCode = vtx_spv.data();
        vk::assert_success(vkCreateShaderModule(dev_, &module_info, nullptr, &vs_.module));

        stage_info = {};
        stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
        stage_info.pName = "main";
        stage_info.module = vs_.module;
        vs_.info = std::move(stage_info);
    }

    if (fragShaderText.data()) {
        std::vector<unsigned int> frag_spv;
        retVal = GLSLtoSPV(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderText.data(), frag_spv);
        assert(retVal);

        module_info = {};
        module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        module_info.codeSize = frag_spv.size() * sizeof(unsigned int);
        module_info.pCode = frag_spv.data();
        vk::assert_success(vkCreateShaderModule(dev_, &module_info, nullptr, &fs_.module));

        stage_info = {};
        stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stage_info.pName = "main";
        stage_info.module = fs_.module;
        fs_.info = std::move(stage_info);
    }

    finalize_glslang();
}

void Guppy::create_descriptor_set_layout(const VkDescriptorSetLayoutCreateFlags descSetLayoutCreateFlags) {
    // if (use_push_constants_) return;

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

    // if (use_push_constants_) {
    //    push_const_range.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    //    push_const_range.offset = 0;
    //    push_const_range.size = sizeof(ShaderParamBlock);

    //    pipeline_layout_info.pushConstantRangeCount = 1;
    //    pipeline_layout_info.pPushConstantRanges = &push_const_range;
    //} else {
    pipeline_layout_info.setLayoutCount = static_cast<uint32_t>(desc_set_layouts_.size());
    pipeline_layout_info.pSetLayouts = desc_set_layouts_.data();
    //}

    vk::assert_success(vkCreatePipelineLayout(dev_, &pipeline_layout_info, nullptr, &pipeline_layout_));
}

void Guppy::destroy_descriptor_and_pipelineLayouts() {
    for (auto& layout : desc_set_layouts_) vkDestroyDescriptorSetLayout(dev_, layout, nullptr);
    vkDestroyPipelineLayout(dev_, pipeline_layout_, nullptr);
}

void Guppy::create_frame_data(int count) {
    const MyShell::Context& ctx = shell_->context();

    frame_data_.resize(count);

    create_fences();

    // if (!use_push_constants_) {
    // create_buffers(); // TODO: this is their ubo implementation. It is worth figuring out !!!!!
    // create_buffer_memory();
    create_color_resources();
    create_depth_resources();
    update_camera();  // TODO: this should be an update ubo!!!
    // create_descriptor_pool();
    // create_descriptor_sets(); // Not sure how to make this work here... pipeline relies on this info
    //}

    frame_data_index_ = 0;
}

void Guppy::create_fences() {
    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (auto& data : frame_data_) vk::assert_success(vkCreateFence(dev_, &fence_info, nullptr, &data.fence));
}

void Guppy::create_descriptor_pool(bool use_texture) {
    VkDescriptorPoolSize type_count[2];
    type_count[0].type =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;  // TODO: look at dynamic ubo's (VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC)
    type_count[0].descriptorCount = 1;
    if (use_texture) {
        type_count[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        type_count[1].descriptorCount = 1;
    }

    VkDescriptorPoolCreateInfo desc_pool_info = {};
    desc_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    desc_pool_info.maxSets = swapchain_image_count_;
    desc_pool_info.poolSizeCount = use_texture ? 2 : 1;
    desc_pool_info.pPoolSizes = type_count;

    vk::assert_success(vkCreateDescriptorPool(dev_, &desc_pool_info, nullptr, &desc_pool_));
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
        writes[1].descriptorCount = 1;
        writes[1].pImageInfo = &textures_[0].imgDescInfo;

        vkUpdateDescriptorSets(dev_, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
    }
}

void Guppy::destroy_frame_data() {
    destroy_color_resources();
    destroy_depth_resources();

    destroy_command_buffers();
    destroy_command_pools();

    // if (!use_push_constants_) {
    //    vk::DestroyDescriptorPool(dev_, desc_pool_, nullptr);

    //    vk::UnmapMemory(dev_, frame_data_mem_);
    //    vk::FreeMemory(dev_, frame_data_mem_, nullptr);

    //    for (auto &data : frame_data_) vk::DestroyBuffer(dev_, data.buf, nullptr);
    //}

    // for (auto cmd_pool : worker_cmd_pools_) vk::DestroyCommandPool(dev_, cmd_pool, nullptr);
    // worker_cmd_pools_.clear();
    // vk::DestroyCommandPool(dev_, primary_cmd_pool_, nullptr);

    // for (auto &data : frame_data_) vk::DestroyFence(dev_, data.fence, nullptr);

    // frame_data_.clear();
}

void Guppy::create_command_pools_and_buffers() {
    auto uniq = get_unique_queue_families();

    cmd_data_.cmd_pools.resize(uniq.size());
    cmd_data_.cmds.resize(uniq.size());

    for (const auto& index : uniq) {
        // POOLS
        VkCommandPoolCreateInfo cmd_pool_info = {};
        cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmd_pool_info.queueFamilyIndex = index;
        cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        vk::assert_success(vkCreateCommandPool(dev_, &cmd_pool_info, nullptr, &cmd_data_.cmd_pools[index]));

        // BUFFERS
        VkCommandBufferAllocateInfo cmd = {};
        cmd.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd.pNext = nullptr;
        cmd.commandPool = cmd_data_.cmd_pools[index];
        cmd.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd.commandBufferCount = 1;

        vk::assert_success(vkAllocateCommandBuffers(dev_, &cmd, &cmd_data_.cmds[index]));
    };
}

void Guppy::destroy_command_pools() {
    for (auto& cmd_pool : cmd_data_.cmd_pools) vkDestroyCommandPool(dev_, cmd_pool, nullptr);
    cmd_data_.cmd_pools.clear();
}

void Guppy::destroy_command_buffers() {
    for (size_t i = 0; i < info.cmd_pools.size(); i++) vkFreeCommandBuffers(dev_, cmd_data_.cmd_pools[i], 1, &cmd_data_.cmds[i]);
    cmd_data_.cmd_pools.clear();
}

void Guppy::create_color_resources() {
    std::vector<uint32_t> queueFamilyIndices = {cmd_data_.graphics_queue_family};
    if (cmd_data_.graphics_queue_family != cmd_data_.transfer_queue_family)
        queueFamilyIndices.push_back(cmd_data_.transfer_queue_family);

    create_image(dev_, cmd_data_.mem_props, queueFamilyIndices, extent_.width, extent_.height, 1, num_samples_, format_,
                 VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, color_resource_.image, color_resource_.memory);

    create_image_view(dev_, color_resource_.image, 1, format_, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D,
                      color_resource_.view);

    transition_image_layout(graphics_cmd(), color_resource_.image, 1, format_, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT);
}

void Guppy::destroy_color_resources() {
    // TODO: what about the format member?
    vkDestroyImageView(dev_, color_resource_.view, nullptr);
    vkDestroyImage(dev_, color_resource_.image, nullptr);
    vkFreeMemory(dev_, color_resource_.memory, nullptr);
}

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

    std::vector<uint32_t> queueFamilyIndices = {cmd_data_.graphics_queue_family};
    if (cmd_data_.graphics_queue_family != cmd_data_.transfer_queue_family)
        queueFamilyIndices.push_back(cmd_data_.transfer_queue_family);

    create_image(dev_, cmd_data_.mem_props, queueFamilyIndices, extent_.width, extent_.height, 1, num_samples_,
                 depth_resource_.format, tiling, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 depth_resource_.image, depth_resource_.memory);

    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (has_stencil_component(depth_resource_.format)) {
        aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    create_image_view(dev_, depth_resource_.image, 1, depth_resource_.format, aspectFlags, VK_IMAGE_VIEW_TYPE_2D,
                      depth_resource_.view);

    transition_image_layout(graphics_cmd(), depth_resource_.image, 1, depth_resource_.format, VK_IMAGE_LAYOUT_UNDEFINED,
                            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
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

// void Guppy::initWindow()
//{
//    glfwInit();
//
//    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
//
//    m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
//
//    glfwSetWindowUserPointer(m_window, this);
//
//    // callbacks
//    glfwSetKeyCallback(m_window, Key_callback);
//    glfwSetFramebufferSizeCallback(m_window, FramebufferSize_callback);
//}

// void Guppy::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer,
// VkDeviceMemory& bufferMemory)
//{
//    // CREATE BUFFER
//
//    VkBufferCreateInfo bufferInfo = {};
//    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//    bufferInfo.size = size;
//    bufferInfo.usage = usage;
//    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//
//    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create buffer!");
//    }
//
//    // ALLOCATE DEVICE MEMORY
//
//    VkMemoryRequirements memRequirements;
//    vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);
//
//    VkMemoryAllocateInfo allocInfo = {};
//    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//    allocInfo.allocationSize = memRequirements.size;
//    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
//
//    /*
//        It should be noted that in a real world application, you're not supposed to actually
//        call vkAllocateMemory for every individual buffer. The maximum number of simultaneous
//        memory allocations is limited by the maxMemoryAllocationCount physical device limit,
//        which may be as low as 4096 even on high end hardware like an NVIDIA GTX 1080. The
//        right way to allocate memory for a large number of objects at the same time is to create
//        a custom allocator that splits up a single allocation among many different objects by
//        using the offset parameters that we've seen in many functions.
//
//        You can either implement such an allocator yourself, or use the VulkanMemoryAllocator
//        library provided by the GPUOpen initiative. However, for this tutorial it's okay to
//        use a separate allocation for every resource, because we won't come close to hitting
//        any of these limits for now.
//
//        The previous chapter already mentioned that you should allocate multiple resources like
//        buffers from a single memory allocation, but in fact you should go a step further.
//        Driver developers recommend that you also store multiple buffers, like the vertex and
//        index buffer, into a single VkBuffer and use offsets in commands like
//        vkCmdBindVertexBuffers. The advantage is that your data is more cache friendly in that
//        case, because it's closer together. It is even possible to reuse the same chunk of
//        memory for multiple resources if they are not used during the same render operations,
//        provided that their data is refreshed, of course. This is known as aliasing and some
//        Vulkan functions have explicit flags to specify that you want to do this.
//    */
//
//    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to allocate buffer memory!");
//    }
//
//    // BIND MEMORY
//
//    vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
//}

// void Guppy::createImage(const uint32_t width,
//                          const uint32_t height,
//                          const uint32_t mipLevels,
//                          const VkSampleCountFlagBits numSamples,
//                          const VkFormat format,
//                          const VkImageTiling tiling,
//                          const VkImageUsageFlags usage,
//                          const VkMemoryPropertyFlags properties,
//                          VkImage& image,
//                          VkDeviceMemory& imageMemory)
//{
//    VkImageCreateInfo imageInfo = {};
//    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
//    imageInfo.imageType = VK_IMAGE_TYPE_2D;
//    imageInfo.extent.width = width;
//    imageInfo.extent.height = height;
//    imageInfo.extent.depth = 1;
//    imageInfo.mipLevels = mipLevels;
//    imageInfo.arrayLayers = 1;
//    imageInfo.format = format;
//    imageInfo.tiling = tiling;
//    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//    imageInfo.usage = usage;
//    imageInfo.samples = numSamples;
//    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//
//    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
//    uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.transferFamily };
//
//    // If the graphics queue, and transfer queue are on different devices
//    if (indices.graphicsFamily != indices.transferFamily)
//    {
//        imageInfo.queueFamilyIndexCount = 2;
//        imageInfo.pQueueFamilyIndices = queueFamilyIndices;
//    }
//    else
//    {
//        imageInfo.queueFamilyIndexCount = 0; // Optional
//        imageInfo.pQueueFamilyIndices = nullptr; // Optional
//    }
//
//    if (vkCreateImage(m_device, &imageInfo, nullptr, &image) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create image!");
//    }
//
//    VkMemoryRequirements memRequirements;
//    vkGetImageMemoryRequirements(m_device, image, &memRequirements);
//
//    VkMemoryAllocateInfo allocInfo = {};
//    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//    allocInfo.allocationSize = memRequirements.size;
//    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);
//
//    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to allocate image memory!");
//    }
//
//    vkBindImageMemory(m_device, image, imageMemory, 0);
//}

// uint32_t Guppy::findMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties)
//{
//    VkPhysicalDeviceMemoryProperties memProperties;
//    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
//
//    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
//    {
//        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
//        {
//            return i;
//        }
//    }
//
//    throw std::runtime_error("failed to find suitable memory type!");
//}

// void Guppy::mainLoop()
//{
//    while (!glfwWindowShouldClose(m_window))
//    {
//        InputHandler::get().reset();
//        glfwPollEvents();
//        drawFrame();
//    }
//
//    // Allow asynchronous calls in drawFrame to finish before cleanup.
//    vkDeviceWaitIdle(m_device);
//}

// void Guppy::createInstance()
//{
//    // check validation layers
//    if (ENABLE_VALIDATION_LAYERS && !checkValidationLayerSupport())
//    {
//        throw std::runtime_error("validation layers requested, but not available!");
//    }
//
//    // application information
//    VkApplicationInfo appInfo = {};
//    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
//    appInfo.pApplicationName = "Hello Triangle";
//    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
//    appInfo.pEngineName = "No Engine";
//    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
//    appInfo.apiVersion = VK_API_VERSION_1_0;
//
//    // create information
//    VkInstanceCreateInfo createInfo = {};
//    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
//    createInfo.pApplicationInfo = &appInfo;
//
//    // layers
//    if (ENABLE_VALIDATION_LAYERS)
//    {
//        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
//        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
//    }
//    else
//    {
//        createInfo.enabledLayerCount = 0;
//    }
//
//    // extensions
//    auto extensions = getRequiredExtensions();
//    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
//    createInfo.ppEnabledExtensionNames = extensions.data();
//
//    // create instance
//    if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create instance!");
//    }
//}

// void Guppy::setupDebugCallback()
//{
//    if (!ENABLE_VALIDATION_LAYERS) return;
//
//    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
//    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
//    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
//    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT; // | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT; createInfo.messageType =
//    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
//    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT; createInfo.pfnUserCallback = Debug_callback; createInfo.pUserData = nullptr;
//    // Optional
//
//    if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_callback) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to setup debug callback!");
//    }
//}

// void Guppy::createSurface()
//{
//    // This function abstracts away platform specific code.
//    // More here (https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Window_surface)
//    if (glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create window surface!");
//    }
//}

// std::vector<const char*> Guppy::getRequiredExtensions()
//{
//    uint32_t glfwExtensionCount = 0;
//    const char** glfwExtensions;
//    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
//
//    checkExtensions();
//
//    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
//
//    if (ENABLE_VALIDATION_LAYERS)
//    {
//        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
//    }
//
//    return extensions;
//}

// void Guppy::checkExtensions()
//{
//    uint32_t extensionCount = 0;
//    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
//    std::vector<VkExtensionProperties> extensions(extensionCount);
//    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());
//
//    std::cout << "available extensions:" << std::endl;
//
//    for (const auto& extension : extensions)
//    {
//        std::cout << "\t" << extension.extensionName << " " << extension.specVersion << std::endl;
//    }
//
//    // TODO: add validation
//}
//

// bool checkValidationLayerSupport() {
//    // uint32_t layerCount;
//    // vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
//
//    // std::vector<VkLayerProperties> availableLayers(layerCount);
//    // vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
//
//    // for (const char* layerName : VALIDATION_LAYERS)
//    //{
//    //    bool found = std::any_of(availableLayers.begin(), availableLayers.end(), [&layerName](VkLayerProperties layerProperties)
//    //    {
//    //        return strcmp(layerName, layerProperties.layerName) == 0;
//    //    });
//    //    if (!found)
//    //    {
//    //        return false;
//    //    }
//    //}
//
//    return true;
//}

// void Guppy::pickPhysicalDevice()
//{
//    uint32_t deviceCount = 0;
//    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
//    if (deviceCount == 0)
//    {
//        throw std::runtime_error("failed to find GPUs with Vulkan support!");
//    }
//
//    std::vector<VkPhysicalDevice> devices(deviceCount);
//    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
//
//    //// Use an ordered map to automatically sort candidates by increasing score
//    //std::map<int, VkPhysicalDevice> candidates;
//
//    for (const auto& device : devices)
//    {
//        //int score = rateDeviceSuitability(device);
//        //candidates[score] = device;
//        if (isDeviceSuitable(device))
//        {
//            m_physicalDevice = device;
//            m_msaaSamples = getMaxUsableSampleCount();
//            return;
//        }
//    }
//
//    //// Check if the best candidate is suitable at all
//    //if (candidates.rbegin()->first > 0) {
//    //	m_physicalDevice = candidates.rbegin()->second;
//    //}
//    //else {
//    throw std::runtime_error("failed to find a suitable GPU!");
//    //}
//}

// bool Guppy::isDeviceSuitable(VkPhysicalDevice device)
//{
//    QueueFamilyIndices indices = findQueueFamilies(device);
//
//    // queues
//    if (!indices.isComplete())
//        return false;
//
//    // extensions
//    if (!checkDeviceExtensionSupport(device))
//        return false;
//
//    // swap chain
//    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
//    if (swapChainSupport.formats.empty() || swapChainSupport.presentModes.empty())
//        return false;
//
//    // features
//    // TODO: OPTION (FEATURE BASED)
//    VkPhysicalDeviceFeatures supportedFeatures;
//    vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
//    if (!supportedFeatures.samplerAnisotropy)
//        return false;
//
//    return true;
//}

// bool Guppy::checkDeviceExtensionSupport(VkPhysicalDevice device)
//{
//    uint32_t extensionCount;
//    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
//
//    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
//    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());
//
//    std::set<std::string> requiredExtensions(DEVICE_EXTENSIONS.begin(), DEVICE_EXTENSIONS.end());
//
//    for (const auto& extension : availableExtensions)
//    {
//        requiredExtensions.erase(extension.extensionName);
//    }
//
//    return requiredExtensions.empty();
//}

// int Guppy::rateDeviceSuitability(VkPhysicalDevice device)
//{
//    // properties
//    VkPhysicalDeviceProperties deviceProperties;
//    vkGetPhysicalDeviceProperties(device, &deviceProperties);
//    // features
//    VkPhysicalDeviceFeatures deviceFeatures;
//    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
//
//    int score = 0;
//
//    // Application can't function without geometry shaders
//    if (!deviceFeatures.geometryShader)
//    {
//        return score;
//    }
//
//    // Need at least one that supports VK_QUEUE_GRAPHICS_BIT
//    auto indices = findQueueFamilies(device);
//    if (!indices.isComplete())
//    {
//        return score;
//    }
//    score += indices.graphicsFamily;
//
//    // Discrete GPUs have a significant performance advantage
//    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
//    {
//        score += 1000;
//    }
//
//    // Maximum possible size of textures affects graphics quality
//    score += deviceProperties.limits.maxImageDimension2D;
//
//    return score;
//}
//
// void Guppy::createLogicalDevice()
//{
//    auto indices = findQueueFamilies(m_physicalDevice);
//
//    // queues
//    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
//    std::set<int> uniqueQueueFamilies = {
//        indices.graphicsFamily,
//        indices.presentFamily,
//        indices.transferFamily
//    };
//
//    float queuePriority = 1.0f;
//    for (int queueFamily : uniqueQueueFamilies)
//    {
//        VkDeviceQueueCreateInfo queueCreateInfo = {};
//        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
//        queueCreateInfo.queueFamilyIndex = queueFamily;
//        queueCreateInfo.queueCount = 1;
//        queueCreateInfo.pQueuePriorities = &queuePriority;
//        queueCreateInfos.push_back(queueCreateInfo);
//    }
//
//    // features
//    VkPhysicalDeviceFeatures deviceFeatures = {};
//    deviceFeatures.samplerAnisotropy = VK_TRUE; // TODO: OPTION (FEATURE BASED (like below))
//    deviceFeatures.sampleRateShading = ENABLE_SAMPLE_SHADING;
//
//    // logical device
//    VkDeviceCreateInfo createInfo = {};
//    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
//    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
//    createInfo.pQueueCreateInfos = queueCreateInfos.data();
//    createInfo.pEnabledFeatures = &deviceFeatures;
//    createInfo.enabledExtensionCount = static_cast<uint32_t>(DEVICE_EXTENSIONS.size());
//    createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS.data();
//
//    if (ENABLE_VALIDATION_LAYERS)
//    {
//        createInfo.enabledLayerCount = static_cast<uint32_t>(VALIDATION_LAYERS.size());
//        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS.data();
//    }
//    else
//    {
//        createInfo.enabledLayerCount = 0;
//    }
//
//    if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create logical device!");
//    }
//
//    vkGetDeviceQueue(m_device, indices.graphicsFamily, 0, &m_graphicsQueue);
//    vkGetDeviceQueue(m_device, indices.presentFamily, 0, &m_presentQueue);
//    vkGetDeviceQueue(m_device, indices.transferFamily, 0, &m_transferQueue);
//}

// QueueFamilyIndices Guppy::findQueueFamilies(VkPhysicalDevice device)
//{
//    QueueFamilyIndices indices;
//
//    uint32_t queueFamilyCount = 0;
//    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
//
//    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
//    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
//
//
//    int i = 0;
//    for (const auto& queueFamily : queueFamilies)
//    {
//        // graphics
//        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
//        {
//            indices.graphicsFamily = i;
//        }
//
//        // present
//        VkBool32 presentSupport = false;
//        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
//        if (queueFamily.queueCount > 0 && presentSupport)
//        {
//            indices.presentFamily = i;
//        }
//
//        // transfer
//        if (queueFamily.queueCount > 0 && (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 && queueFamily.queueFlags &
//        VK_QUEUE_TRANSFER_BIT)
//        {
//            indices.transferFamily = i;
//        }
//
//        if (indices.isComplete())
//        {
//            break;
//        }
//
//        i++;
//    }
//
//    return indices;
//}

// SwapChainSupportDetails Guppy::querySwapChainSupport(VkPhysicalDevice device)
//{
//    SwapChainSupportDetails details;
//
//    // capabilities
//    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);
//
//    // formats
//    uint32_t formatCount;
//    vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
//    if (formatCount != 0)
//    {
//        details.formats.resize(formatCount);
//        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
//    }
//
//    // present modes
//    uint32_t presentModeCount;
//    vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);
//    if (presentModeCount != 0)
//    {
//        details.presentModes.resize(presentModeCount);
//        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
//    }
//
//    return details;
//}

// VkSurfaceFormatKHR Guppy::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
//{
//    // no preferred format
//    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED)
//    {
//        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
//    }
//
//    for (const auto& availableFormat : availableFormats)
//    {
//        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
//        {
//            return availableFormat;
//        }
//    }
//
//    // Could give a rank to the formats, but we are just testing so return the first available
//    std::cout << "choosing first available swap surface format!";
//    return availableFormats[0];
//}

// VkPresentModeKHR Guppy::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
//{
//    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR; // guaranteed to be present (vert sync)
//
//    for (const auto& availablePresentMode : availablePresentModes)
//    {
//        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
//        { // triple buffer
//            return availablePresentMode;
//        }
//        else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
//        { // no sync
//            bestMode = availablePresentMode;
//        }
//    }
//
//    return bestMode;
//}

// VkExtent2D Guppy::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
//{
//    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
//    {
//        return capabilities.currentExtent;
//    }
//    else
//    {
//        int width, height;
//        glfwGetFramebufferSize(m_window, &width, &height);
//
//        VkExtent2D actualExtent = {
//            static_cast<uint32_t>(width),
//            static_cast<uint32_t>(height)
//        };
//
//        actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width,
//        actualExtent.width)); actualExtent.height = std::max(capabilities.minImageExtent.height,
//        std::min(capabilities.maxImageExtent.height, actualExtent.height));
//
//        return actualExtent;
//    }
//}

// void Guppy::createSwapChain()
//{
//    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(m_physicalDevice);
//
//    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
//    m_swapChainImageFormat = surfaceFormat.format;
//    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
//    m_swapChainExtent = chooseSwapExtent(swapChainSupport.capabilities);
//
//    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
//    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
//    {
//        imageCount = swapChainSupport.capabilities.maxImageCount;
//    }
//
//    VkSwapchainCreateInfoKHR createInfo = {};
//    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
//    createInfo.surface = m_surface;
//
//    createInfo.minImageCount = imageCount;
//    createInfo.imageFormat = m_swapChainImageFormat;
//    createInfo.imageColorSpace = surfaceFormat.colorSpace;
//    createInfo.imageExtent = m_swapChainExtent;
//    createInfo.imageArrayLayers = 1; // 1 unless stereoscopic 3D application
//    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // render directly
//
//    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
//    uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };
//
//    // If the graphics queue, and present queue are on different devices
//    if (indices.graphicsFamily != indices.presentFamily)
//    {
//        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
//        createInfo.queueFamilyIndexCount = 2;
//        createInfo.pQueueFamilyIndices = queueFamilyIndices;
//    }
//    else
//    {
//        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
//        createInfo.queueFamilyIndexCount = 0; // Optional
//        createInfo.pQueueFamilyIndices = nullptr; // Optional
//    }
//
//    createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // ex: rotate 90
//
//    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // alpha blend with other windows
//
//    createInfo.presentMode = presentMode;
//    // Clip obscured pixels (by other windows for example). Only needed if the pixel info is reused.
//    createInfo.clipped = VK_TRUE;
//
//    // If window resized new swap chain is created. This points to the old one.
//    createInfo.oldSwapchain = VK_NULL_HANDLE;
//
//    if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create swap chain!");
//    }
//
//    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
//    m_swapChainImages.resize(imageCount);
//    vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());
//}

// void Guppy::createSwapChainImageViews()
//{
//    m_swapChainImageViews.resize(m_swapChainImages.size());
//
//    for (uint32_t i = 0; i < m_swapChainImages.size(); i++)
//    {
//        m_swapChainImageViews[i] = createImageView(1, m_swapChainImages[i], m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
//    }
//}

// VkImageView Guppy::createImageView(const uint32_t mipLevels,
//                                     const VkImage image,
//                                     const VkFormat format,
//                                     const VkImageAspectFlags aspectFlags,
//                                     const VkImageViewType viewType)
//{
//    VkImageViewCreateInfo viewInfo = {};
//    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
//    viewInfo.image = image;
//
//    // swap chain is a 2D depth texture
//    viewInfo.viewType = viewType;
//    viewInfo.format = format;
//
//    // defaults
//    viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
//    viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
//    viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
//    viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
//
//    viewInfo.subresourceRange.aspectMask = aspectFlags;
//    viewInfo.subresourceRange.baseMipLevel = 0;
//    viewInfo.subresourceRange.levelCount = mipLevels;
//    /*
//        If you were working on a stereographic 3D application, then you would create a swap
//        chain with multiple layers. You could then create multiple image views for each image
//        representing the views for the left and right eyes by accessing different layers.
//    */
//    viewInfo.subresourceRange.baseArrayLayer = 0;
//    viewInfo.subresourceRange.layerCount = 1;
//
//    VkImageView imageView;
//    if (vkCreateImageView(m_device, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create texture image view!");
//    }
//
//    return imageView;
//}

// void Guppy::createGraphicsPipeline()
//{
//    // MODULES
//    // vertex
//    auto vertShaderCode = FileLoader::readFile("shaders/vert.spv");
//    //std::cout << "vert.spv size: " << vertShaderCode.size() << std::endl;
//    auto vertShaderModule = createShaderModule(vertShaderCode);
//
//    // fragment
//    auto fragShaderCode = FileLoader::readFile("shaders/frag.spv");
//    //std::cout << "frag.spv size: " << fragShaderCode.size() << std::endl;
//    auto fragShaderModule = createShaderModule(fragShaderCode);
//
//
//    // STAGES
//    // vertex
//    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
//    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
//    vertShaderStageInfo.module = vertShaderModule;
//    vertShaderStageInfo.pName = "main";
//    /*  There is one more (optional) member, pSpecializationInfo, which we won't be using here,
//        but is worth discussing. It allows you to specify values for shader constants. You can
//        use a single shader module where its behavior can be configured at pipeline creation by
//        specifying different values for the constants used in it. This is more efficient than
//        configuring the shader using variables at render time, because the compiler can do
//        optimizations like eliminating if statements that depend on these values. If you don't
//        have any constants like that, then you can set the member to nullptr, which our struct
//        initialization does automatically.
//    */
//
//    // fragment
//    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
//    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//    fragShaderStageInfo.module = fragShaderModule;
//    fragShaderStageInfo.pName = "main";
//
//    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
//
//    // INPUT ASSEMBLY
//
//    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
//    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//    // bindings
//    auto bindingDescription = Vertex::getBindingDescription();
//    vertexInputInfo.vertexBindingDescriptionCount = 1;
//    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
//    // attributes
//    auto attributeDescriptions = Vertex::getAttributeDescriptions();
//    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
//    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
//
//    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
//    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//    inputAssembly.primitiveRestartEnable = VK_FALSE;
//
//    // VIEWPORT & SCISSOR
//
//    VkViewport viewport = {};
//    viewport.x = 0.0f;
//    viewport.y = 0.0f;
//    viewport.width = (float)m_swapChainExtent.width;
//    viewport.height = (float)m_swapChainExtent.height;
//    viewport.minDepth = 0.0f;
//    viewport.maxDepth = 1.0f;
//
//    VkRect2D scissor = {};
//    scissor.offset = { 0, 0 };
//    scissor.extent = m_swapChainExtent;
//
//    VkPipelineViewportStateCreateInfo viewportState = {};
//    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//    /*  It is possible to use multiple viewports and scissor rectangles on some graphics cards,
//        so its members reference an array of them. Using multiple requires enabling a GPU
//        feature (see logical device creation).
//    */
//    viewportState.viewportCount = 1;
//    viewportState.pViewports = &viewport;
//    viewportState.scissorCount = 1;
//    viewportState.pScissors = &scissor;
//
//    // DEPTH
//
//    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
//    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
//    depthStencil.depthTestEnable = VK_TRUE;
//    depthStencil.depthWriteEnable = VK_TRUE;
//    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
//    depthStencil.depthBoundsTestEnable = VK_FALSE;
//    depthStencil.minDepthBounds = 0.0f; // Optional
//    depthStencil.maxDepthBounds = 1.0f; // Optional
//    depthStencil.stencilTestEnable = VK_FALSE;
//    depthStencil.front = {}; // Optional
//    depthStencil.back = {}; // Optional
//
//    // RASTERIZER
//
//    VkPipelineRasterizationStateCreateInfo rasterizer = {};
//    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//    /*  If depthClampEnable is set to VK_TRUE, then fragments that are beyond the near and far
//        planes are clamped to them as opposed to discarding them. This is useful in some special
//        cases like shadow maps. Using this requires enabling a GPU feature.
//    */
//    rasterizer.depthClampEnable = VK_FALSE;
//    rasterizer.rasterizerDiscardEnable = VK_FALSE; // discards geometry
//    // Line & point are available here too. Others require feature enabling
//    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//    /*  The lineWidth member is straightforward, it describes the thickness of lines in terms of
//        number of fragments. The maximum line width that is supported depends on the hardware and
//        any line thicker than 1.0f requires you to enable the wideLines GPU feature.
//    */
//    rasterizer.lineWidth = 1.0f;
//    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
//    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
//    rasterizer.depthBiasEnable = VK_FALSE;
//    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
//    rasterizer.depthBiasClamp = 0.0f; // Optional
//    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional
//
//    VkPipelineMultisampleStateCreateInfo multisampling = {};
//    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//    multisampling.rasterizationSamples = m_msaaSamples;
//    multisampling.sampleShadingEnable = ENABLE_SAMPLE_SHADING; // enable sample shading in the pipeline (sampling for fragment
//    interiors) multisampling.minSampleShading = MIN_SAMPLE_SHADING; // min fraction for sample shading; closer to one is smooth
//    multisampling.pSampleMask = nullptr; // Optional
//    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
//    multisampling.alphaToOneEnable = VK_FALSE; // Optional
//
//    // BLENDING
//
//    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
//    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
//    VK_COLOR_COMPONENT_A_BIT; colorBlendAttachment.blendEnable = VK_FALSE; colorBlendAttachment.srcColorBlendFactor =
//    VK_BLEND_FACTOR_ONE; // Optional colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
//    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
//    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
//    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
//    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
//    // common setup
//    //colorBlendAttachment.blendEnable = VK_TRUE;
//    //colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
//    //colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
//    //colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
//    //colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
//    //colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
//    //colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
//
//    VkPipelineColorBlendStateCreateInfo colorBlending = {};
//    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//    colorBlending.logicOpEnable = VK_FALSE;
//    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
//    colorBlending.attachmentCount = 1;
//    colorBlending.pAttachments = &colorBlendAttachment;
//    colorBlending.blendConstants[0] = 0.0f; // Optional
//    colorBlending.blendConstants[1] = 0.0f; // Optional
//    colorBlending.blendConstants[2] = 0.0f; // Optional
//    colorBlending.blendConstants[3] = 0.0f; // Optional
//
//    //VkDynamicState dynamicStates[] = {
//    //	VK_DYNAMIC_STATE_VIEWPORT,
//    //	VK_DYNAMIC_STATE_LINE_WIDTH
//    //};
//
//    //VkPipelineDynamicStateCreateInfo dynamicState = {};
//    //dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
//    //dynamicState.dynamicStateCount = 2;
//    //dynamicState.pDynamicStates = dynamicStates;
//
//    // LAYOUT
//
//    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
//    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//    pipelineLayoutInfo.setLayoutCount = 1;
//    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
//    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
//    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional
//
//    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create pipeline layout!");
//    }
//
//    // PIPELINE
//
//    VkGraphicsPipelineCreateInfo pipelineInfo = {};
//    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//    // shader stages
//    pipelineInfo.stageCount = 2;
//    pipelineInfo.pStages = shaderStages;
//    // fixed function stage
//    pipelineInfo.pVertexInputState = &vertexInputInfo;
//    pipelineInfo.pInputAssemblyState = &inputAssembly;
//    pipelineInfo.pViewportState = &viewportState;
//    pipelineInfo.pRasterizationState = &rasterizer;
//    pipelineInfo.pMultisampleState = &multisampling;
//    pipelineInfo.pDepthStencilState = &depthStencil;
//    pipelineInfo.pColorBlendState = &colorBlending;
//    pipelineInfo.pDynamicState = nullptr; // Optional
//    // layout
//    pipelineInfo.layout = m_pipelineLayout;
//    // render pass
//    pipelineInfo.renderPass = m_renderPass;
//    pipelineInfo.subpass = 0;
//    // inheritance
//    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
//    pipelineInfo.basePipelineIndex = -1; // Optional
//
//    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create graphics pipeline!");
//    }
//
//    // CLEAN UP
//
//    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
//    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
//}
//
// VkShaderModule Guppy::createShaderModule(const std::vector<char>& code)
//{
//    VkShaderModuleCreateInfo createInfo = {};
//    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//    createInfo.codeSize = code.size(); // alignment works because it is a vector?
//    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
//
//    VkShaderModule shaderModule;
//    if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create shader module!");
//    }
//
//    return shaderModule;
//}
//
// void Guppy::createRenderPass()
//{
//    // DEPTH ATTACHMENT
//
//    VkAttachmentDescription depthAttachment = {};
//    depthAttachment.format = findDepthFormat();
//    depthAttachment.samples = m_msaaSamples;
//    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//    VkAttachmentReference depthAttachmentRef = {};
//    depthAttachmentRef.attachment = 1;
//    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//
//    // COLOR ATTACHMENT (MULIT-SAMPLED)
//
//    VkAttachmentDescription colorAttachment = {};
//    colorAttachment.format = m_swapChainImageFormat;
//    colorAttachment.samples = m_msaaSamples;
//    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
//    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//    VkAttachmentReference colorAttachmentRef = {};
//    colorAttachmentRef.attachment = 0;
//    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//    // COLOR ATTACHMENT RESOLVE (FINAL 1 SAMPLE)
//
//    VkAttachmentDescription colorAttachmentResolve = {};
//    colorAttachmentResolve.format = m_swapChainImageFormat;
//    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
//    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
//    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
//    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
//    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
//    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
//
//    VkAttachmentReference colorAttachmentResolveRef = {};
//    colorAttachmentResolveRef.attachment = 2;
//    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//
//    /*  The index of the attachment in this array is directly referenced from the fragment
//        shader with the layout(location = 0) out vec4 outColor directive!
//    */
//    VkSubpassDescription subpass = {};
//    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
//    subpass.colorAttachmentCount = 1;
//    subpass.pColorAttachments = &colorAttachmentRef;
//    subpass.pDepthStencilAttachment = &depthAttachmentRef;
//    subpass.pResolveAttachments = &colorAttachmentResolveRef;
//
//    // used for waiting in draw (figure this out)
//    VkSubpassDependency dependency = {};
//    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
//    dependency.dstSubpass = 0;
//    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//    dependency.srcAccessMask = 0;
//    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

//    std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };
//    VkRenderPassCreateInfo renderPassInfo = {};
//    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
//    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
//    renderPassInfo.pAttachments = attachments.data();
//    renderPassInfo.subpassCount = 1;
//    renderPassInfo.pSubpasses = &subpass;
//    renderPassInfo.dependencyCount = 1;
//    renderPassInfo.pDependencies = &dependency;
//
//    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create render pass!");
//    }
//}

// void Guppy::createFramebuffers()
//{
//    m_swapChainFramebuffers.resize(m_swapChainImageViews.size());
//
//    for (size_t i = 0; i < m_swapChainImageViews.size(); i++)
//    {
//        std::array<VkImageView, 3> attachments = {
//            m_colorImageView,
//            m_depthImageView,
//            m_swapChainImageViews[i]
//        };
//
//        VkFramebufferCreateInfo framebufferInfo = {};
//        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
//        framebufferInfo.renderPass = m_renderPass;
//        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
//        framebufferInfo.pAttachments = attachments.data();
//        framebufferInfo.width = m_swapChainExtent.width;
//        framebufferInfo.height = m_swapChainExtent.height;
//        framebufferInfo.layers = 1;
//
//        if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS)
//        {
//            throw std::runtime_error("failed to create framebuffer!");
//        }
//    }
//}

// void Guppy::createCommandPools()
//{
//    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(m_physicalDevice);
//
//    // GRAPHICS
//
//    VkCommandPoolCreateInfo poolInfo = {};
//    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
//    poolInfo.flags = 0; // Optional
//
//    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_graphicsCommandPool) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create graphics command pool!");
//    }
//
//    // TRANSFER
//
//    poolInfo = {};
//    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
//    poolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily;
//    poolInfo.flags = 0; // Optional
//
//    if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_transferCommandPool) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create transfer command pool!");
//    }
//}

void Guppy::create_draw_cmds() {
    draw_cmds_.resize(swapchain_image_count_);

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = graphics_cmd_pool();
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

        // DRAW

        vkCmdBeginRenderPass(draw_cmds_[i], &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(draw_cmds_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_);

        vkCmdSetViewport(draw_cmds_[i], 0, 1, &viewport_);
        vkCmdSetScissor(draw_cmds_[i], 0, 1, &scissor_);

        VkBuffer vertex_buffers[] = {vertex_res_.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(draw_cmds_[i], 0, 1, vertex_buffers, offsets);

        vkCmdBindIndexBuffer(draw_cmds_[i], index_res_.buffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(draw_cmds_[i], VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout_, 0, 1, &desc_sets_[i], 0, nullptr);

        // vkCmdDraw(
        //    m_graphicsCommandBuffers[i],
        //    static_cast<uint32_t>(VERTICES.size()), // vertexCount
        //    1, // instanceCount - Used for instanced rendering, use 1 if you're not doing that.
        //    0, // firstVertex - Used as an offset into the vertex buffer, defines the lowest value of gl_VertexIndex.
        //    0  // firstInstance - Used as an offset for instanced rendering, defines the lowest value of gl_InstanceIndex.
        //);

        vkCmdDrawIndexed(draw_cmds_[i], model_.getIndexSize(), 1, 0, 0, 0);

        vkCmdEndRenderPass(draw_cmds_[i]);

        vk::assert_success(vkEndCommandBuffer(draw_cmds_[i]));
    }
}

// void Guppy::drawFrame()
//{
//    // WAIT FOR FENCES
//
//    vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max()/*disables
//    timeout*/);
//
//    // ACQUIRE NEXT SWAP CHAIN IMAGE
//
//    uint32_t imageIndex;
//    VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, std::numeric_limits<uint64_t>::max()/*disables timeout*/,
//    m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
//
//    if (result == VK_ERROR_OUT_OF_DATE_KHR)
//    {
//        recreateSwapChain();
//        return;
//    }
//    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
//    {
//        throw std::runtime_error("failed to acquire swap chain image!");
//    }
//
//    updateUniformBuffer(imageIndex);
//
//    VkSubmitInfo submitInfo = {};
//    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//
//    VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[m_currentFrame] };
//    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
//    submitInfo.waitSemaphoreCount = 1;
//    submitInfo.pWaitSemaphores = waitSemaphores;
//    submitInfo.pWaitDstStageMask = waitStages;
//
//    submitInfo.commandBufferCount = 1;
//    submitInfo.pCommandBuffers = &m_graphicsCommandBuffers[imageIndex];
//
//    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
//    submitInfo.signalSemaphoreCount = 1;
//    submitInfo.pSignalSemaphores = signalSemaphores;
//
//    // RESET FENCES (AFTER POTENTIAL RECREATION OF SWAP CHAIN)
//
//    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
//
//    if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to submit draw command buffer!");
//    }
//
//    VkPresentInfoKHR presentInfo = {};
//    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
//
//    presentInfo.waitSemaphoreCount = 1;
//    presentInfo.pWaitSemaphores = signalSemaphores;
//
//    VkSwapchainKHR swapChains[] = { m_swapChain };
//    presentInfo.swapchainCount = 1;
//    presentInfo.pSwapchains = swapChains;
//    presentInfo.pImageIndices = &imageIndex;
//
//    presentInfo.pResults = nullptr; // Optional
//
//    result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
//
//    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized)
//    {
//        m_framebufferResized = false;
//        recreateSwapChain();
//    }
//    else if (result != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to present swap chain image!");
//    }
//
//    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
//}

// void Guppy::createSyncObjects()
//{
//    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
//    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
//    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
//
//    VkSemaphoreCreateInfo semaphoreInfo = {};
//    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
//
//    VkFenceCreateInfo fenceInfo = {};
//    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
//    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // start fence in signaled state
//
//    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
//    {
//        if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
//            vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
//            vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS)
//        {
//
//            throw std::runtime_error("failed to create synchronization objects for a frame!");
//        }
//    }
//}
//
// void Guppy::cleanupSwapChain()
//{
//    vkDestroyImageView(m_device, m_colorImageView, nullptr);
//    vkDestroyImage(m_device, m_colorImage, nullptr);
//    vkFreeMemory(m_device, m_colorImageMemory, nullptr);
//
//    vkDestroyImageView(m_device, m_depthImageView, nullptr);
//    vkDestroyImage(m_device, m_depthImage, nullptr);
//    vkFreeMemory(m_device, m_depthImageMemory, nullptr);
//
//    for (auto framebuffer : m_swapChainFramebuffers)
//    {
//        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
//    }
//
//    vkFreeCommandBuffers(m_device, m_graphicsCommandPool, static_cast<uint32_t>(m_graphicsCommandBuffers.size()),
//    m_graphicsCommandBuffers.data());
//
//    vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
//    vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
//    vkDestroyRenderPass(m_device, m_renderPass, nullptr);
//
//    for (auto imageView : m_swapChainImageViews)
//    {
//        vkDestroyImageView(m_device, imageView, nullptr);
//    }
//
//    vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
//}
//
// void Guppy::recreateSwapChain()
//{
//    int width = 0, height = 0;
//    while (width == 0 || height == 0)
//    {
//        glfwGetFramebufferSize(m_window, &width, &height);
//        glfwWaitEvents();
//    }
//
//    vkDeviceWaitIdle(m_device);
//
//    cleanupSwapChain();
//
//    createSwapChain();
//    createSwapChainImageViews();
//    createRenderPass();
//    /*  Viewport and scissor rectangle size is specified during graphics pipeline creation, so the pipeline
//        also needs to be rebuilt. It is possible to avoid Guppy::this by using dynamic state for the viewports and
//        scissor rectangles.
//    */
//    createGraphicsPipeline();
//    createColorResources();
//    createDepthResources();
//    createFramebuffers();
//    createGraphicsCommandBuffers();
//}

void Guppy::create_input_assembly_data() {
    // VkCommandBuffer cmd1, cmd2;
    // StagingBufferHandler::get().begin_command_recording(cmd);

    std::vector<StagingBufferResource> staging_resources;
    std::vector<uint32_t> queueFamilyIndices = {cmd_data_.graphics_queue_family};
    if (cmd_data_.graphics_queue_family != cmd_data_.transfer_queue_family)
        queueFamilyIndices.push_back(cmd_data_.transfer_queue_family);

    // Vertex buffer
    StagingBufferResource stg_res;
    create_vertex_data(stg_res);
    staging_resources.emplace_back(stg_res);

    // Index buffer
    stg_res = {};
    create_index_data(stg_res);
    staging_resources.emplace_back(stg_res);

    // TODO: this options below are wrong!!!

    VkPipelineStageFlags wait_stages[] = {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};
    StagingBufferHandler::get().end_recording_and_submit(staging_resources.data(), staging_resources.size(), transfer_cmd(),
                                                         cmd_data_.graphics_queue_family, &graphics_cmd(), wait_stages,
                                                         StagingBufferHandler::END_TYPE::RESET);
}

void Guppy::create_vertex_data(StagingBufferResource& stg_res) {
    // STAGING BUFFER
    VkDeviceSize bufferSize = model_.getVertexBufferSize();

    auto memReqsSize =
        create_buffer(dev_, cmd_data_.mem_props, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stg_res.buffer, stg_res.memory);

    // FILL STAGING BUFFER ON DEVICE
    void* pData;
    vkMapMemory(dev_, stg_res.memory, 0, memReqsSize, 0, &pData);
    /*
        You can now simply memcpy the vertex data to the mapped memory and unmap it again using vkUnmapMemory.
        Unfortunately the driver may not immediately copy the data into the buffer memory, for example because
        of caching. It is also possible that writes to the buffer are not visible in the mapped memory yet. There
        are two ways to deal with that problem:
            - Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
            - Call vkFlushMappedMemoryRanges to after writing to the mapped memory, and call
              vkInvalidateMappedMemoryRanges before reading from the mapped memory
        We went for the first approach, which ensures that the mapped memory always matches the contents of the
        allocated memory. Do keep in mind that this may lead to slightly worse performance than explicit flushing,
        but we'll see why that doesn't matter in the next chapter.
    */
    memcpy(pData, model_.getVertexData(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(dev_, stg_res.memory);

    // FAST VERTEX BUFFER
    create_buffer(dev_, cmd_data_.mem_props,
                  bufferSize,  // TODO: probably don't need to check memory requirements again
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                  vertex_res_.buffer, vertex_res_.memory);

    // COPY FROM STAGING TO FAST
    copyBuffer(transfer_cmd(), stg_res.buffer, vertex_res_.buffer, memReqsSize);
}

// void Guppy::createVertexBuffer()
//{
//    VkDeviceSize bufferSize = m_model.getVertexBufferSize();
//
//    // STAGING BUFFER
//
//    VkBuffer stagingBuffer;
//    VkDeviceMemory stagingBufferMemory;
//    createBuffer(
//        bufferSize,
//        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//        stagingBuffer,
//        stagingBufferMemory
//    );
//
//    // FILL STAGING BUFFER ON DEVICE
//
//    void* data;
//    vkMapMemory(m_device, stagingBufferMemory, 0, bufferSize, 0, &data);
//    /*
//        You can now simply memcpy the vertex data to the mapped memory and unmap it again using vkUnmapMemory.
//        Unfortunately the driver may not immediately copy the data into the buffer memory, for example because
//        of caching. It is also possible that writes to the buffer are not visible in the mapped memory yet. There
//        are two ways to deal with that problem:
//            - Use a memory heap that is host coherent, indicated with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
//            - Call vkFlushMappedMemoryRanges to after writing to the mapped memory, and call
//              vkInvalidateMappedMemoryRanges before reading from the mapped memory
//        We went for the first approach, which ensures that the mapped memory always matches the contents of the
//        allocated memory. Do keep in mind that this may lead to slightly worse performance than explicit flushing,
//        but we'll see why that doesn't matter in the next chapter.
//    */
//    memcpy(data, m_model.getVetexData(), (size_t)bufferSize);
//    vkUnmapMemory(m_device, stagingBufferMemory);
//
//    // FAST VERTEX BUFFER
//
//    createBuffer(
//        bufferSize,
//        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
//        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//        m_vertexBuffer,
//        m_vertexBufferMemory
//    );
//
//    // COPY FROM STAGING TO FAST
//
//    copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);
//
//    flushSetupCommandBuffers(m_transferCommandPool, m_transferQueue, [this, stagingBuffer, stagingBufferMemory]()
//    {
//        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
//        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
//    });
//}

void Guppy::copyBuffer(VkCommandBuffer& cmd, const VkBuffer& src_buf, const VkBuffer& dst_buf, const VkDeviceSize& size) {
    VkBufferCopy copy_region = {};
    copy_region.srcOffset = 0;  // Optional
    copy_region.dstOffset = 0;  // Optional
    copy_region.size = size;
    vkCmdCopyBuffer(cmd, src_buf, dst_buf, 1, &copy_region);
}

// void Guppy::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
//{
//    addSetupCommandBuffer(m_transferCommandPool, [&](VkCommandBuffer& commandBuffer)
//    {
//        VkBufferCopy copyRegion = {};
//        copyRegion.srcOffset = 0; // Optional
//        copyRegion.dstOffset = 0; // Optional
//        copyRegion.size = size;
//        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
//    });
//}

void Guppy::create_index_data(StagingBufferResource& stg_res) {
    VkDeviceSize bufferSize = model_.getIndexBufferSize();

    auto memReqsSize =
        create_buffer(dev_, cmd_data_.mem_props, bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stg_res.buffer, stg_res.memory);

    void* pData;
    vkMapMemory(dev_, stg_res.memory, 0, memReqsSize, 0, &pData);
    memcpy(pData, model_.getIndexData(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(dev_, stg_res.memory);

    create_buffer(dev_, cmd_data_.mem_props, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_res_.buffer, index_res_.memory);

    copyBuffer(transfer_cmd(), stg_res.buffer, index_res_.buffer, memReqsSize);
}

// void Guppy::createDescriptorSetLayout()
//{
//    // UNIFORM BUFFER
//
//    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
//    uboLayoutBinding.binding = 0;
//    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    uboLayoutBinding.descriptorCount = 1;
//    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
//    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional
//
//    // COMBINED IMAGE SAMPLER
//
//    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
//    samplerLayoutBinding.binding = 1;
//    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    samplerLayoutBinding.descriptorCount = 1;
//    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
//    samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional
//
//    // LAYOUT
//
//    std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
//    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
//    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
//    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
//    layoutInfo.pBindings = bindings.data();
//
//    if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create descriptor set layout!");
//    }
//}
//
// void Guppy::createUniformBuffers()
//{
//    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
//
//    m_uniformBuffers.resize(m_swapChainImages.size());
//    m_uniformBuffersMemory.resize(m_swapChainImages.size());
//
//    for (size_t i = 0; i < m_swapChainImages.size(); i++)
//    {
//        createBuffer(
//            bufferSize,
//            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
//            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//            m_uniformBuffers[i],
//            m_uniformBuffersMemory[i]
//        );
//    }
//
//    m_ubo.perspective.defaultPerspective((float)m_swapChainExtent.width, (float)m_swapChainExtent.height);
//}
//
// void Guppy::updateUniformBuffer(uint32_t currentImage)
//{
//    m_ubo.perspective.updatePosistion(InputHandler::get().getPosDir());
//
//    /*
//        Using a UBO this way is not the most efficient way to pass frequently changing values to the
//        shader. A more efficient way to pass a small buffer of data to shaders are push constants.
//        We may look at these in a future chapter.
//
//        TODO: !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//    */
//
//    void* data;
//    vkMapMemory(m_device, m_uniformBuffersMemory[currentImage], 0, sizeof(m_ubo), 0, &data);
//    memcpy(data, &m_ubo, sizeof(m_ubo));
//    vkUnmapMemory(m_device, m_uniformBuffersMemory[currentImage]);
//}
//
// void Guppy::createDescriptorPool()
//{
//    std::array<VkDescriptorPoolSize, 2> poolSizes = {};
//    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//    poolSizes[0].descriptorCount = static_cast<uint32_t>(m_swapChainImages.size());
//    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//    poolSizes[1].descriptorCount = static_cast<uint32_t>(m_swapChainImages.size());
//
//    VkDescriptorPoolCreateInfo poolInfo = {};
//    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
//    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
//    poolInfo.pPoolSizes = poolSizes.data();
//    poolInfo.maxSets = static_cast<uint32_t>(m_swapChainImages.size());
//
//    if (vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create descriptor pool!");
//    }
//}
//
// void Guppy::createDescriptorSets()
//{
//    std::vector<VkDescriptorSetLayout> layouts(m_swapChainImages.size(), m_descriptorSetLayout);
//    VkDescriptorSetAllocateInfo allocInfo = {};
//    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//    allocInfo.descriptorPool = m_descriptorPool;
//    allocInfo.descriptorSetCount = static_cast<uint32_t>(m_swapChainImages.size());
//    allocInfo.pSetLayouts = layouts.data();
//
//    m_descriptorSets.resize(m_swapChainImages.size());
//    if (vkAllocateDescriptorSets(m_device, &allocInfo, m_descriptorSets.data()) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to allocate descriptor sets!");
//    }
//
//    for (size_t i = 0; i < m_swapChainImages.size(); i++)
//    {
//        VkDescriptorBufferInfo bufferInfo = {};
//        bufferInfo.buffer = m_uniformBuffers[i];
//        bufferInfo.offset = 0;
//        bufferInfo.range = sizeof(UniformBufferObject);
//
//        VkDescriptorImageInfo imageInfo = {};
//        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//        imageInfo.imageView = m_textureImageView;
//        imageInfo.sampler = m_textureSampler;
//
//        std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};
//
//        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        descriptorWrites[0].dstSet = m_descriptorSets[i];
//        descriptorWrites[0].dstBinding = 0;
//        descriptorWrites[0].dstArrayElement = 0;
//        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//        descriptorWrites[0].descriptorCount = 1;
//        descriptorWrites[0].pBufferInfo = &bufferInfo;
//
//        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//        descriptorWrites[1].dstSet = m_descriptorSets[i];
//        descriptorWrites[1].dstBinding = 1;
//        descriptorWrites[1].dstArrayElement = 0;
//        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//        descriptorWrites[1].descriptorCount = 1;
//        descriptorWrites[1].pImageInfo = &imageInfo;
//
//        vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
//    }
//}

// void Guppy::createTextureImage(std::string texturePath)
//{
//    if (texturePath.empty())
//        texturePath = "textures/texture.jpg";
//
//    int texWidth, texHeight, texChannels;
//    stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
//
//    if (!pixels)
//    {
//        throw std::runtime_error("failed to load texture image!");
//    }
//
//    VkDeviceSize imageSize = texWidth * texHeight * 4;
//    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
//
//    VkBuffer stagingBuffer;
//    VkDeviceMemory stagingBufferMemory;
//
//    createBuffer(
//        imageSize,
//        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
//        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
//        stagingBuffer,
//        stagingBufferMemory
//    );
//
//    void* data;
//    vkMapMemory(m_device, stagingBufferMemory, 0, imageSize, 0, &data);
//    memcpy(data, pixels, static_cast<size_t>(imageSize));
//    vkUnmapMemory(m_device, stagingBufferMemory);
//
//    stbi_image_free(pixels);
//
//    createImage(
//        texWidth,
//        texHeight,
//        m_mipLevels,
//        VK_SAMPLE_COUNT_1_BIT,
//        VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_TILING_OPTIMAL,
//        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
//        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//        m_textureImage,
//        m_textureImageMemory
//    );
//
//    //QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
//    //uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//
//    //// If the graphics queue, and transfer queue are on different devices
//    //if (indices.graphicsFamily != indices.transferFamily)
//    //{
//    //    srcQueueFamilyIndex = (uint32_t)indices.transferFamily;
//    //    dstQueueFamilyIndex = (uint32_t)indices.graphicsFamily;
//    //}
//
//    transitionImageLayout(
//        VK_QUEUE_FAMILY_IGNORED,
//        VK_QUEUE_FAMILY_IGNORED,
//        m_mipLevels,
//        m_textureImage,
//        VK_FORMAT_R8G8B8A8_UNORM,
//        VK_IMAGE_LAYOUT_UNDEFINED,
//        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//        m_transferCommandPool
//    );
//
//    copyBufferToImage(stagingBuffer, m_textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
//
//    // Mipmaps have to be created in a graphics command pool queue so,
//    // FLUSH TRANSFER POOL COMMAND QUEUE
//
//    flushSetupCommandBuffers(m_transferCommandPool, m_transferQueue, [this, stagingBuffer, stagingBufferMemory]()
//    {
//        vkDestroyBuffer(m_device, stagingBuffer, nullptr);
//        vkFreeMemory(m_device, stagingBufferMemory, nullptr);
//    });
//
//    // transition to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps
//    generateMipmaps(m_textureImage, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, m_mipLevels);
//
//    flushSetupCommandBuffers(m_graphicsCommandPool, m_graphicsQueue, []() {});
//}

// void Guppy::transitionImageLayout(const uint32_t srcQueueFamilyIndex,
//                                     const uint32_t dstQueueFamilyIndex,
//                                     const uint32_t mipLevels,
//                                     const VkImage image,
//                                     const VkFormat format,
//                                     const VkImageLayout oldLayout,
//                                     const VkImageLayout newLayout,
//                                     const VkCommandPool commandPool)
//{
//    addSetupCommandBuffer(commandPool, [&](VkCommandBuffer& commandBuffer)
//    {
//        VkImageMemoryBarrier barrier = {};
//        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//        barrier.oldLayout = oldLayout;
//        barrier.newLayout = newLayout;
//        barrier.srcQueueFamilyIndex = srcQueueFamilyIndex;
//        barrier.dstQueueFamilyIndex = dstQueueFamilyIndex;
//        barrier.image = image;
//        barrier.subresourceRange.baseMipLevel = 0;
//        barrier.subresourceRange.levelCount = mipLevels;
//        barrier.subresourceRange.baseArrayLayer = 0;
//        barrier.subresourceRange.layerCount = 1;
//
//        VkPipelineStageFlags sourceStage;
//        VkPipelineStageFlags destinationStage;
//
//        // OLD LAYOUT
//
//        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
//        {
//            barrier.srcAccessMask = 0;
//            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//
//            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
//        }
//        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
//        {
//            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//
//            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
//            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
//        }
//        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
//        {
//            barrier.srcAccessMask = 0;
//            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
//
//            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
//        }
//        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
//        {
//            barrier.srcAccessMask = 0;
//            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
//
//            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
//            destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
//        }
//        else
//        {
//            throw std::invalid_argument("unsupported image layout transition!");
//        }
//
//        // NEW LAYOUT
//
//        if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
//        {
//            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
//
//            if (hasStencilComponent(format))
//            {
//                barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
//            }
//        }
//        else
//        {
//            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//        }
//
//        vkCmdPipelineBarrier(
//            commandBuffer,
//            sourceStage, destinationStage,
//            0,
//            0, nullptr,
//            0, nullptr,
//            1, &barrier
//        );
//    });
//}

// void Guppy::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
//{
//    addSetupCommandBuffer(m_transferCommandPool, [&](VkCommandBuffer& commandBuffer)
//    {
//        VkBufferImageCopy region = {};
//        region.bufferOffset = 0;
//        region.bufferRowLength = 0;
//        region.bufferImageHeight = 0;
//
//        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//        region.imageSubresource.mipLevel = 0;
//        region.imageSubresource.baseArrayLayer = 0;
//        region.imageSubresource.layerCount = 1;
//
//        region.imageOffset = { 0, 0, 0 };
//        region.imageExtent = {
//            width,
//            height,
//            1
//        };
//
//        vkCmdCopyBufferToImage(
//            commandBuffer,
//            buffer,
//            image,
//            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // VK_IMAGE_USAGE_TRANSFER_DST_BIT
//            1,
//            &region
//        );
//    });
//}

// void Guppy::createTextureImageView()
//{
//    m_textureImageView = createImageView(m_mipLevels, m_textureImage, VK_FORMAT_R8G8B8A8_UNORM /* format aligns with stb */,
//    VK_IMAGE_ASPECT_COLOR_BIT);
//}

// void Guppy::createTextureSampler()
//{
//    VkSamplerCreateInfo samplerInfo = {};
//    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
//    samplerInfo.magFilter = VK_FILTER_LINEAR;
//    samplerInfo.minFilter = VK_FILTER_LINEAR;
//    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
//    samplerInfo.anisotropyEnable = VK_TRUE; // TODO: OPTION (FEATURE BASED)
//    samplerInfo.maxAnisotropy = 16;
//    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
//    samplerInfo.unnormalizedCoordinates = VK_FALSE; // test this out for fun
//    samplerInfo.compareEnable = VK_FALSE;
//    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
//    samplerInfo.compareEnable = VK_FALSE;
//    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
//    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
//    samplerInfo.minLod =  0; // static_cast<float>(m_mipLevels / 2); // Optional
//    samplerInfo.maxLod = static_cast<float>(m_mipLevels);
//    samplerInfo.mipLodBias = 0; // Optional
//
//    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler) != VK_SUCCESS)
//    {
//        throw std::runtime_error("failed to create texture sampler!");
//    }
//}

//// this is all bad (you really need to think about these barriers and how to submit them, and transfering ownership)
// void Guppy::addSetupCommandBuffer(const VkCommandPool commandPool, const std::function<void(VkCommandBuffer&)>& lambda)
//{
//    VkCommandBufferAllocateInfo allocInfo = {};
//    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
//    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//    allocInfo.commandPool = commandPool;
//    allocInfo.commandBufferCount = 1;
//
//    VkCommandBuffer commandBuffer;
//    vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);
//
//    VkCommandBufferBeginInfo beginInfo = {};
//    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
//    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
//
//    vkBeginCommandBuffer(commandBuffer, &beginInfo);
//
//    lambda(commandBuffer);
//
//    vkEndCommandBuffer(commandBuffer);
//
//    m_setupCommandBuffers.push_back(std::move(commandBuffer));
//}

//// this all all bad
// void Guppy::flushSetupCommandBuffers(const VkCommandPool commandPool, const VkQueue queue, const std::function<void()>&
// lambda)
//{
//    std::vector<VkCommandBuffer> commandBuffers(m_setupCommandBuffers);
//    m_setupCommandBuffers.clear();
//
//    //std::thread([this, &commandBuffers, &lambda]()
//    //{
//
//    VkSubmitInfo submitInfo = {};
//    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
//    submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
//    submitInfo.pCommandBuffers = commandBuffers.data();
//
//    std::cout << "setup command buffer count " << submitInfo.commandBufferCount << std::endl;
//
//    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
//    vkQueueWaitIdle(queue); // FENCE, BARRIER, SEMAPHORE ?????????????????? TODO: FIGURE OUT SOMETHING BETTER THAN A BLANKET WAIT
//
//    vkFreeCommandBuffers(m_device, commandPool, 1, commandBuffers.data());
//
//    lambda();
//
//    //}).detach();
//
//}

void Guppy::create_model() {
    const MyShell::Context& ctx = shell_->context();

    bool loadDefault = true;
    std::string tex_path = "..\\..\\..\\images\\texture.jpg";

    if (loadDefault) {
        // m_model.loadAxes();
        model_.loadDefault();
    } else {
        // UP_VECTOR = glm::vec3(0.0f, 0.0f, 1.0f);
        tex_path = CHALET_TEXTURE_PATH;
        model_.loadChalet();
    }

    // VkCommandBuffer cmd1, cmd2;
    // StagingBufferHandler::get().begin_command_recording(cmd);

    StagingBufferResource stg_res;
    std::vector<StagingBufferResource> staging_resources;
    std::vector<uint32_t> queueFamilyIndices = {cmd_data_.graphics_queue_family};
    if (cmd_data_.graphics_queue_family != cmd_data_.transfer_queue_family)
        queueFamilyIndices.push_back(cmd_data_.transfer_queue_family);

    // stg_res = {};
    // tex_path = "..\\..\\..\\images\\texture.jpg";
    // textures_.emplace_back(Texture::createTexture(ctx, stg_res, transfer_cmd(), graphics_cmd(), queueFamilyIndices, tex_path));
    // staging_resources.emplace_back(stg_res);

    stg_res = {};
    tex_path = "..\\..\\..\\images\\chalet.jpg";
    textures_.emplace_back(Texture::createTexture(ctx, stg_res, transfer_cmd(), graphics_cmd(), queueFamilyIndices, tex_path));
    staging_resources.emplace_back(stg_res);

    VkPipelineStageFlags wait_stages[] = {VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL};
    StagingBufferHandler::get().end_recording_and_submit(staging_resources.data(), staging_resources.size(), transfer_cmd(),
                                                         cmd_data_.graphics_queue_family, &graphics_cmd(), wait_stages,
                                                         StagingBufferHandler::END_TYPE::RESET);
}

void Guppy::destroy_textures() {
    for (auto& tex : textures_) {
        vkDestroySampler(dev_, tex.sampler, nullptr);
        vkDestroyImageView(dev_, tex.view, nullptr);
        vkDestroyImage(dev_, tex.image, nullptr);
        vkFreeMemory(dev_, tex.memory, nullptr);
    }
}

// void Guppy::createModel()
//{
//    bool loadDefault = true;
//    std::string texturePath;
//
//    if (loadDefault)
//    {
//        m_model.loadAxes();
//         //m_model.loadDefault();
//    }
//    else
//    {
//        UP_VECTOR = glm::vec3(0.0f, 0.0f, 1.0f);
//        texturePath = CHALET_TEXTURE_PATH;
//        m_model.loadChalet();
//    }
//
//    createTextureImage(texturePath);
//    createTextureImageView();
//    createTextureSampler();
//}

// void Guppy::createDepthResources()
//{
//    VkFormat depthFormat = findDepthFormat();
//
//    createImage(
//        m_swapChainExtent.width,
//        m_swapChainExtent.height,
//        1,
//        m_msaaSamples,
//        depthFormat,
//        VK_IMAGE_TILING_OPTIMAL,
//        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
//        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//        m_depthImage,
//        m_depthImageMemory
//    );
//
//    m_depthImageView = createImageView(1, m_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
//
//    QueueFamilyIndices indices = findQueueFamilies(m_physicalDevice);
//    uint32_t srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//
//    // If the graphics queue, and transfer queue are on different devices
//    if (indices.graphicsFamily != indices.transferFamily)
//    {
//        srcQueueFamilyIndex = (uint32_t)indices.transferFamily;
//        dstQueueFamilyIndex = (uint32_t)indices.graphicsFamily;
//    }
//
//    transitionImageLayout(
//        srcQueueFamilyIndex,
//        dstQueueFamilyIndex,
//        1,
//        m_depthImage,
//        depthFormat,
//        VK_IMAGE_LAYOUT_UNDEFINED,
//        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
//        m_transferCommandPool
//    );
//
//    flushSetupCommandBuffers(m_transferCommandPool, m_transferQueue, []() {});
//}

// VkFormat Guppy::findSupportedFormat(const std::vector<VkFormat>& candidates, const VkImageTiling tiling, const
// VkFormatFeatureFlags features)
//{
//    VkFormat format;
//
//    for (VkFormat f : candidates)
//    {
//        VkFormatProperties props;
//        vkGetPhysicalDeviceFormatProperties(m_physicalDevice, f, &props);
//
//        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
//        {
//            format = f;
//            break;
//        }
//        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
//        {
//            format = f;
//            break;
//        }
//        else
//        {
//            throw std::runtime_error("failed to find supported format!");
//        }
//    }
//
//    return format;
//}

// VkFormat Guppy::findDepthFormat()
//{
//    return findSupportedFormat(
//        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
//        VK_IMAGE_TILING_OPTIMAL,
//        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
//    );
//}
//
// bool Guppy::hasStencilComponent(VkFormat format)
//{
//    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
//}

// void Guppy::generateMipmaps(const VkImage image,
//                              const VkFormat imageFormat,
//                              const int32_t texWidth,
//                              const int32_t texHeight,
//                              const uint32_t mipLevels)
//{
//    /*  Throw if the format we want doesn't support linear blitting!
//
//        TODO: Clean up what happens if the GPU doesn't handle linear blitting
//
//        There are two alternatives in this case. You could implement a function that searches common texture image
//        formats for one that does support linear blitting, or you could implement the mipmap generation in software
//        with a library like stb_image_resize. Each mip level can then be loaded into the image in the same way that
//        you loaded the original image.
//    */
//    findSupportedFormat(
//        { imageFormat },
//        VK_IMAGE_TILING_OPTIMAL,
//        VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT
//    );
//
//    // This was the way before mip maps
//    //transitionImageLayout(
//    //    srcQueueFamilyIndexFinal,
//    //    dstQueueFamilyIndexFinal,
//    //    m_mipLevels,
//    //    image,
//    //    VK_FORMAT_R8G8B8A8_UNORM,
//    //    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//    //    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
//    //);
//
//    addSetupCommandBuffer(m_graphicsCommandPool, [&](VkCommandBuffer& commandBuffer)
//    {
//        VkImageMemoryBarrier barrier = {};
//        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
//        barrier.image = image;
//        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
//        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//        barrier.subresourceRange.baseArrayLayer = 0;
//        barrier.subresourceRange.layerCount = 1;
//        barrier.subresourceRange.levelCount = 1;
//
//        int32_t mipWidth = texWidth;
//        int32_t mipHeight = texHeight;
//
//        for (uint32_t i = 1; i < mipLevels; i++)
//        {
//            // CREATE MIP LEVEL
//
//            barrier.subresourceRange.baseMipLevel = i - 1;
//            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
//            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//
//            vkCmdPipelineBarrier(commandBuffer,
//                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
//                                 0, nullptr,
//                                 0, nullptr,
//                                 1, &barrier);
//
//            VkImageBlit blit = {};
//            blit.srcOffsets[0] = { 0, 0, 0 };
//            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
//            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//            blit.srcSubresource.mipLevel = i - 1;
//            blit.srcSubresource.baseArrayLayer = 0;
//            blit.srcSubresource.layerCount = 1;
//            blit.dstOffsets[0] = { 0, 0, 0 };
//            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
//            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//            blit.dstSubresource.mipLevel = i;
//            blit.dstSubresource.baseArrayLayer = 0;
//            blit.dstSubresource.layerCount = 1;
//
//            vkCmdBlitImage(commandBuffer,
//                           image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
//                           image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
//                           1, &blit,
//                           VK_FILTER_LINEAR);
//
//            // TRANSITION TO SHADER READY
//
//            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
//            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
//            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//
//            vkCmdPipelineBarrier(commandBuffer,
//                                 VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
//                                 0, nullptr,
//                                 0, nullptr,
//                                 1, &barrier);
//
//            // This is a bit wonky methinks (non-sqaure is the case for this)
//            if (mipWidth > 1) mipWidth /= 2;
//            if (mipHeight > 1) mipHeight /= 2;
//        }
//
//        // This is not handled in the loop so one more!!!! The last level is never never
//        // blitted from.
//
//        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
//        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
//        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
//        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
//        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
//
//        vkCmdPipelineBarrier(commandBuffer,
//                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
//                             0, nullptr,
//                             0, nullptr,
//                             1, &barrier);
//
//    });
//}

// VkSampleCountFlagBits Guppy::getMaxUsableSampleCount()
//{
//    VkPhysicalDeviceProperties physicalDeviceProperties;
//    vkGetPhysicalDeviceProperties(m_physicalDevice, &physicalDeviceProperties);
//
//    VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts,
//    physicalDeviceProperties.limits.framebufferDepthSampleCounts);
//    //return the highest possible one for now
//    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
//    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
//    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
//    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
//    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
//    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
//
//    return VK_SAMPLE_COUNT_1_BIT;
//}
//
// void Guppy::createColorResources()
//{
//    createImage(
//        m_swapChainExtent.width,
//        m_swapChainExtent.height,
//        1,
//        m_msaaSamples,
//        m_swapChainImageFormat,
//        VK_IMAGE_TILING_OPTIMAL,
//        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
//        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
//        m_colorImage,
//        m_colorImageMemory
//    );
//
//    m_colorImageView = createImageView(1, m_colorImage, m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
//
//    transitionImageLayout(
//        VK_QUEUE_FAMILY_IGNORED,
//        VK_QUEUE_FAMILY_IGNORED,
//        1,
//        m_colorImage,
//        m_swapChainImageFormat,
//        VK_IMAGE_LAYOUT_UNDEFINED,
//        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
//        m_graphicsCommandPool
//    );
//
//    flushSetupCommandBuffers(m_graphicsCommandPool, m_graphicsQueue, []() {});
//}
