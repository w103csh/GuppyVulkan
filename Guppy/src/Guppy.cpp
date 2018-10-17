
#include <algorithm>

#include "CmdBufHandler.h"
#include "Constants.h"
#include "Extensions.h"
#include "Guppy.h"
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

    if (use_push_constants_ && sizeof(ShaderParamBlock) > physical_dev_props_.limits.maxPushConstantsSize) {
        shell_->log(MyShell::LOG_WARN, "cannot enable push constants");
        use_push_constants_ = false;
    }

    mem_flags_.reserve(CmdBufHandler::mem_props().memoryTypeCount);
    for (uint32_t i = 0; i < CmdBufHandler::mem_props().memoryTypeCount; i++)
        mem_flags_.push_back(CmdBufHandler::mem_props().memoryTypes[i].propertyFlags);

    // meshes_ = new Meshes(dev_, mem_flags_);

    CmdBufHandler::init(ctx);
    LoadingResourceHandler::init(ctx);

    create_uniform_buffer();

    PipelineHandler::init(ctx, settings());

    createTextures();
    createScenes();

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

    // delete meshes_;

    // *
    for (auto& scene : scenes_) scene->destroy(dev_);
    void destroy_ubo_resources();

    CmdBufHandler::destroy();
    PipelineHandler::destroy();
    LoadingResourceHandler::cleanupResources();

    destroyTextures();

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
            active_scene()->addMesh(shell_->context(), std::move(p1));
        } break;
        case KEY::KEY_F3: {
            addTexture(dev_, CHALET_TEXTURE_PATH);
            auto tm1 = std::make_unique<TextureMesh>(pTextures_.back(), CHALET_MODEL_PATH);
            active_scene()->addMesh(shell_->context(), std::move(tm1));
        } break;
        case KEY::KEY_F5: {
            if (test < 3) {
                addTexture(dev_, STATUE_TEXTURE_PATH);
            }
            auto p1 = std::make_unique<TexturePlane>(
                getTextureByPath(STATUE_TEXTURE_PATH), 1.0f, 1.0f, true, glm::vec3(0.0f, -test, 0.0f),
                glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));

            active_scene()->addMesh(shell_->context(), std::move(p1));
            // auto p1 = std::make_unique<ColorPlane>(1.0f, 1.0f, true, glm::vec3(),
            //                                       glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f,
            //                                       0.0f)));
            // active_scene()->addMesh(shell_->context(), std::move(p1), frame_data_index_);
        } break;
        case KEY::KEY_F6: {
            auto p1 = std::make_unique<TexturePlane>(
                getTextureByPath(VULKAN_TEXTURE_PATH), 1.0f, 1.0f, true, glm::vec3(0.0f, ++test, 0.0f),
                glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));

            active_scene()->addMesh(shell_->context(), std::move(p1));
            // auto p1 = std::make_unique<ColorPlane>(1.0f, 1.0f, true, glm::vec3(),
            //                                       glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f,
            //                                       0.0f)));
            // active_scene()->addMesh(shell_->context(), std::move(p1), frame_data_index_);
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

    // **********************
    //  Frame specfic tasks
    // **********************

    update_ubo();

    // **********************

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
    // loading resources
    LoadingResourceHandler::cleanupResources();
    // textures
    updateTextures(dev_);
    // scenes
    for (auto& scene : scenes_) {
        // TODO: im really not into this...
        if (scene->update(ctx, frame_data_index_)) {
            scene->recordDrawCmds(ctx, frameData(), framebuffers_, viewport_, scissor_);
        }
    }

    // for (auto &worker : workers_) worker->update_simulation();
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

void Guppy::createScenes() {
    auto ctx = shell_->context();

    auto scene1 = std::make_unique<Scene>(ctx, ubo_resource_, pTextures_.size());

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

void Guppy::createTextures() {
    // addTexture(dev_, STATUE_TEXTURE_PATH);
    addTexture(dev_, CHALET_TEXTURE_PATH);
    addTexture(dev_, VULKAN_TEXTURE_PATH);
}

void Guppy::addTexture(const VkDevice& dev, std::string texPath) {
    for (auto& tex : pTextures_) {
        if (tex->path == texPath) shell_->log(MyShell::LOG_WARN, "Texture with same path was already loaded.");
    }
    // make texture and a loading future
    auto pTex = std::make_shared<Texture::TextureData>(pTextures_.size(), texPath);
    auto fut = Texture::loadTexture(dev, true, pTex);
    // move texture and future to the vectors
    pTextures_.emplace_back(std::move(pTex));
    texFutures_.emplace_back(std::move(fut));
}

std::shared_ptr<Texture::TextureData> Guppy::getTextureByPath(std::string path) {
    auto it = std::find_if(pTextures_.begin(), pTextures_.end(), [&path](auto& pTex) { return pTex->path == path; });
    return it == pTextures_.end() ? nullptr : (*it);
}

void Guppy::updateTextures(const VkDevice& dev) {
    // Check texture futures
    if (!texFutures_.empty()) {
        auto itFut = texFutures_.begin();

        while (itFut != texFutures_.end()) {
            auto& fut = (*itFut);

            // Check the status but don't wait...
            auto status = fut.wait_for(std::chrono::seconds(0));

            if (status == std::future_status::ready) {
                // Finish creating the texture
                auto& tex = fut.get();

                Texture::createTexture(dev, true, tex);

                // Remove the future from the list if all goes well.
                itFut = texFutures_.erase(itFut);

            } else {
                ++itFut;
            }
        }
    }
}

void Guppy::destroyTextures() {
    for (auto& pTex : pTextures_) {
        vkDestroySampler(dev_, pTex->sampler, nullptr);
        vkDestroyImageView(dev_, pTex->view, nullptr);
        vkDestroyImage(dev_, pTex->image, nullptr);
        vkFreeMemory(dev_, pTex->memory, nullptr);
    }
    pTextures_.clear();
}