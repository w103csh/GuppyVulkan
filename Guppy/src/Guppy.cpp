
#include <algorithm>

#include "Axes.h"
#include "CmdBufHandler.h"
#include "Constants.h"
#include "Extensions.h"
#include "Game.h"
#include "Guppy.h"
#include "InputHandler.h"
#include "ModelHandler.h"
#include "LoadingResourceHandler.h"
#include "Model.h"
#include "ModelHandler.h"
#include "Shell.h"
#include "PipelineHandler.h"
#include "Plane.h"
#include "SceneHandler.h"
#include "TextureHandler.h"
#include "UIHandler.h"

#ifdef USE_DEBUG_GUI
#include "ImGuiUI.h"
#endif

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

struct UBOTag {
    const char name[17] = "ubo tag";
} uboTag;

}  // namespace

Guppy::Guppy(const std::vector<std::string>& args)
    : Game("Guppy", args),
      multithread_(true),
      use_push_constants_(false),
      sim_paused_(false),
      sim_fade_(false),
      // sim_(5000),
      physical_dev_(VK_NULL_HANDLE),
      dev_(VK_NULL_HANDLE),
      queue_(VK_NULL_HANDLE),
      queue_family_(),
      format_(),
      aligned_object_data_size(),
      physical_dev_props_(),
      primary_cmd_pool_(VK_NULL_HANDLE),
      desc_pool_(VK_NULL_HANDLE),
      frame_data_index_(),
      frame_data_mem_(VK_NULL_HANDLE),
      render_pass_clear_value_({0.0f, 0.1f, 0.2f, 1.0f}),
      render_pass_begin_info_(),
      primary_cmd_begin_info_(),
      primary_cmd_submit_wait_stages_(),
      primary_cmd_submit_info_(),
      extent_(),
      viewport_(),
      scissor_(),
      swapchain_image_count_(),
      color_resource_(),
      depth_resource_(),
      camera_(glm::vec3(2.0f, 2.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f),
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

void Guppy::attach_shell(Shell& sh) {
    Game::attach_shell(sh);

    const Shell::Context& ctx = sh.context();
    physical_dev_ = ctx.physical_dev;
    dev_ = ctx.dev;
    // queue_ = ctx.game_queue;
    queue_family_ = ctx.game_queue_family;
    format_ = ctx.surface_format.format;
    // * Data from Shell
    swapchain_image_count_ = ctx.image_count;
    depth_resource_.format = ctx.depth_format;
    physical_dev_props_ = ctx.physical_dev_props[ctx.physical_dev_index].properties;

    if (use_push_constants_ && sizeof(ShaderParamBlock) > physical_dev_props_.limits.maxPushConstantsSize) {
        shell_->log(Shell::LOG_WARN, "cannot enable push constants");
        use_push_constants_ = false;
    }

    mem_flags_.reserve(CmdBufHandler::mem_props().memoryTypeCount);
    for (uint32_t i = 0; i < CmdBufHandler::mem_props().memoryTypeCount; i++)
        mem_flags_.push_back(CmdBufHandler::mem_props().memoryTypes[i].propertyFlags);

    // meshes_ = new Meshes(dev_, mem_flags_);

    CmdBufHandler::init(ctx);
#ifdef USE_DEBUG_GUI
    UIHandler::init(&sh, settings_, std::make_unique<ImGuiUI>());
#else
    UIHandler::init(&sh, settings_);
#endif
    LoadingResourceHandler::init(ctx);

    createLights();
    createUniformBuffer();

    ShaderHandler::init(sh, settings_, defUBO_.positionalLightData.size(), defUBO_.spotLightData.size());
    PipelineHandler::init(&sh, settings_);
    TextureHandler::init(&sh);
    ModelHandler::init(&sh);

    SceneHandler::init(&sh, settings_);
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
    // TODO: kill futures !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    if (multithread_) {
        // for (auto &worker : workers_) worker->stop();
    }
    // delete meshes_;

    // *
    SceneHandler::destroy();

    CmdBufHandler::destroy();
    PipelineHandler::destroy();
    ShaderHandler::destroy();
    LoadingResourceHandler::cleanupResources();

    destroyUniformBuffer();
    TextureHandler::destroy();

    Game::detach_shell();
}

void Guppy::attach_swapchain() {
    const Shell::Context& ctx = shell_->context();
    extent_ = ctx.extent;

    // Get the image data from the swapchain
    get_swapchain_image_data(ctx.swapchain);

    create_frame_data(swapchain_image_count_);

    prepare_viewport();
    prepare_framebuffers(ctx.swapchain);
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

    CmdBufHandler::resetCmdBuffers();
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
        case KEY::KEY_1: {
            if (positionalLights_.size() > 1) {
                // light
                auto& light = positionalLights_[1];
                light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_X * -2.0f)));
                //// mesh
                // auto& pMesh = SceneHandler::getActiveScene()->getTextureMesh(1);
                // pMesh->transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_X * -2.0f)));
                // pMesh->setStatusRedraw();
            }
            // FlagBits flags = positionalLights_[0].getFlags();
            // flags ^= Light::FLAGS::SHOW;
            // positionalLights_[0].setFlags(flags);
        } break;
        case KEY::KEY_2: {
            if (positionalLights_.size() > 1) {
                auto& light = positionalLights_[1];
                light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_X * 2.0f)));
            }
            // FlagBits flags = positionalLights_[1].getFlags();
            // flags ^= Light::FLAGS::SHOW;
            // positionalLights_[1].setFlags(flags);
        } break;
        case KEY::KEY_3: {
            if (positionalLights_.size() > 1) {
                auto& light = positionalLights_[1];
                light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_Z * 2.0f)));
            }
            // if (false) {
            //    auto tm1 =
            //        std::make_unique<TextureMesh>(std::make_unique<Material>(getTextureByPath(CHALET_TEX_PATH)),
            //        CHALET_MODEL_PATH);
            //    SceneHandler::getActiveScene()->addMesh(shell_->context(), std::move(tm1));
            //} else if (false) {
            //    auto model = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)) *
            //                 glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
            //    auto tm1 = std::make_unique<TextureMesh>(std::make_unique<Material>(getTextureByPath(MED_H_DIFF_TEX_PATH)),
            //                                             MED_H_MODEL_PATH, model);
            //    SceneHandler::getActiveScene()->addMesh(shell_->context(), std::move(tm1));
            //} else if (false) {
            //    auto sphereBot = std::make_unique<ColorMesh>(std::make_unique<Material>(), SPHERE_MODEL_PATH);
            //    SceneHandler::getActiveScene()->addMesh(shell_->context(), std::move(sphereBot));
            //    auto sphereTop =
            //        std::make_unique<ColorMesh>(std::make_unique<Material>(), SPHERE_MODEL_PATH,
            //                                    glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(1.0f, 0.0f,
            //                                    0.0f)));
            //    SceneHandler::getActiveScene()->addMesh(shell_->context(), std::move(sphereTop));
            //} else if (true) {
            //    auto p2 = std::make_unique<TexturePlane>(getTextureByPath(HARDWOOD_FLOOR_TEX_PATH), glm::mat4(1.0f), true);
            //    SceneHandler::getActiveScene()->addMesh(shell_->context(), std::move(p2));
            //}
        } break;
        case KEY::KEY_4: {
            if (positionalLights_.size() > 1) {
                auto& light = positionalLights_[1];
                light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_Z * -2.0f)));
            }
            // auto p1 = std::make_unique<ColorPlane>(std::make_unique<Material>(Material::PER_VERTEX_COLOR));
            // SceneHandler::getActiveScene()->addMesh(shell_->context(), std::move(p1));
        } break;
        case KEY::KEY_5: {
            if (positionalLights_.size() > 1) {
                auto& light = positionalLights_[1];
                light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_Y * 2.0f)));
            }
            // for (auto& light : positionalLights_) {
            //    FlagBits flags = light.getFlags();
            //    flags ^= Light::FLAGS::SHOW;
            //    light.setFlags(flags);
            //}
            // if (test < 3) {
            //    addTexture(dev_, STATUE_TEXTURE_PATH);
            //}
            // auto p1 = std::make_unique<TexturePlane>(
            //    getTextureByPath(STATUE_TEXTURE_PATH), 1.0f, 1.0f, true, glm::vec3(0.0f, -test, 0.0f),
            //    glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));

            // SceneHandler::getActiveScene()->addMesh(shell_->context(), std::move(p1));
            // auto p1 = std::make_unique<ColorPlane>(1.0f, 1.0f, true, glm::vec3(),
            //                                       glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f,
            //                                       0.0f)));
            // SceneHandler::getActiveScene()->addMesh(shell_->context(), std::move(p1), frame_data_index_);
        } break;
        case KEY::KEY_6: {
            if (positionalLights_.size() > 1) {
                auto& light = positionalLights_[1];
                light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_Y * -2.0f)));
            }
            // auto model =
            //    helpers::affine(glm::vec3(0.5f), glm::vec3(0.0f, ++test, 0.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f,
            //    0.0f));
            // auto p1 = std::make_unique<TexturePlane>(getTextureByPath(VULKAN_TEX_PATH), model, true);
            // SceneHandler::getActiveScene()->addMesh(shell_->context(), std::move(p1));
        } break;
        case KEY::KEY_7: {
            if (spotLights_.size() > 0) {
                auto& light = spotLights_[0];
                auto worldY = light.worldToLocal(CARDINAL_Y);
                light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_X * -2.0f), M_PI_FLT, worldY));
                light.setCutoff(glm::radians(25.0f));
                light.setExponent(25.0f);
            }
        } break;
        case KEY::KEY_8: {
            defUBO_.shaderData.flags ^= Shader::FLAGS::TOON_SHADE;
        } break;
        default:
            break;
    }
}

/*  This function is for updating things regardless of framerate. It is based on settings.ticks_per_second,
    and should be called that many times per second. add_game_time is weird and appears to limit the amount
    of ticks per second arbitrarily, so this in reality could do anything.
    NOTE: Things like input should not used here since this is not guaranteed to be called each time input
    is collected. This function could be called as many as
*/
void Guppy::on_tick() {
    if (sim_paused_) return;

    auto& pScene = SceneHandler::getActiveScene();

    // TODO: Should this be "on_frame"? every other frame? async? ... I have no clue yet.
    LoadingResourceHandler::cleanupResources();
    TextureHandler::update();

    // TODO: move to SceneHandler::update or something!
    ModelHandler::update(pScene);

    // TODO: ifdef this stuff out
    if (settings_.enable_directory_listener) {
        ShaderHandler::update(pScene);
    }

    pScene->update(shell_->context());

    // for (auto &worker : workers_) worker->update_simulation();
}

void Guppy::on_frame(float framePred) {
    auto& data = frame_data_[frame_data_index_];
    const auto& ctx = shell_->context();
    auto& pScene = SceneHandler::getActiveScene();

    // wait for the last submission since we reuse frame data
    vk::assert_success(vkWaitForFences(dev_, 1, &data.fence, true, UINT64_MAX));
    vk::assert_success(vkResetFences(dev_, 1, &data.fence));

    const Shell::BackBuffer& back = ctx.acquired_back_buffer;

    // **********************
    // Pre-record tasks
    // **********************

    updateUniformBuffer();

    // **********************
    // Record
    // **********************

    pScene->record(ctx, framebuffers_[frame_data_index_], data.fence, viewport_, scissor_, frame_data_index_, false);

    // **********************
    // Post-record tasks
    // **********************

    ShaderHandler::cleanupOldResources();
    PipelineHandler::cleanupOldResources();
    SceneHandler::cleanupInvalidResources();

    // **********************

    uint32_t commandBufferCount;
    auto draw_cmds = pScene->getDrawCmds(frame_data_index_, commandBufferCount);

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

    vk::assert_success(vkQueueSubmit(CmdBufHandler::graphics_queue(), 1, &submit_info, data.fence));

    frame_data_index_ = (frame_data_index_ + 1) % static_cast<uint8_t>(frame_data_.size());
}

void Guppy::get_swapchain_image_data(const VkSwapchainKHR& swapchain) {
    uint32_t image_count;
    vk::assert_success(vkGetSwapchainImagesKHR(dev_, swapchain, &image_count, nullptr));
    // If this ever fails then you have to change the start up a ton!!!!
    assert(swapchain_image_count_ == image_count);

    std::vector<VkImage> images(image_count);
    vk::assert_success(vkGetSwapchainImagesKHR(dev_, swapchain, &image_count, images.data()));

    for (auto& image : images) {
        SwapchainImageResource res;
        res.image = std::move(image);
        helpers::createImageView(dev_, res.image, 1, format_, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1, res.view);
        swapchain_image_resources_.push_back(res);
    }
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
//      SceneHandler::getActiveScene() - render pass
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
    fb_info.renderPass = SceneHandler::getActiveScene()->activeRenderPass();
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

void Guppy::createUniformBuffer(std::string markerName) {
    const Shell::Context& ctx = shell_->context();

    // camera
    camera_.update(static_cast<float>(settings_.initial_width) / static_cast<float>(settings_.initial_height));
    // lights
    defUBO_.positionalLightData.resize(positionalLights_.size());
    defUBO_.spotLightData.resize(spotLights_.size());

    VkDeviceSize size = 0;
    // camera
    size += sizeof(Camera::Data);
    // shader
    size += sizeof(DefaultUniformBuffer::ShaderData);
    // lights
    size += sizeof(Light::PositionalData) * defUBO_.positionalLightData.size();
    size += sizeof(Light::SpotData) * defUBO_.spotLightData.size();

    UBOResource_.size = helpers::createBuffer(dev_, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                              UBOResource_.buffer, UBOResource_.memory);

    UBOResource_.count = 1;
    UBOResource_.info.offset = 0;
    UBOResource_.info.range = size;
    UBOResource_.info.buffer = UBOResource_.buffer;

    if (settings_.enable_debug_markers) {
        if (markerName.empty()) markerName += "Default";
        markerName += " uniform buffer block";
        ext::DebugMarkerSetObjectName(dev_, (uint64_t)UBOResource_.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                      markerName.c_str());
        ext::DebugMarkerSetObjectTag(dev_, (uint64_t)UBOResource_.buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, 0,
                                     sizeof(uboTag), &uboTag);
    }
}

void Guppy::copyUniformBufferMemory() {
    uint8_t* pData;
    vk::assert_success(vkMapMemory(dev_, UBOResource_.memory, 0, UBOResource_.size, 0, (void**)&pData));

    VkDeviceSize size = 0, offset = 0;

    // CAMERA
    size = sizeof(Camera::Data);
    memcpy(pData, defUBO_.pCamera, size);
    offset += size;

    // SHADER
    size = sizeof(DefaultUniformBuffer::ShaderData);
    memcpy(pData + offset, &defUBO_.shaderData.flags, size);
    offset += size;

    // LIGHTS
    // positional
    size = sizeof(Light::PositionalData) * defUBO_.positionalLightData.size();
    if (size) memcpy(pData + offset, defUBO_.positionalLightData.data(), size);
    offset += size;
    // spot
    size = sizeof(Light::SpotData) * defUBO_.spotLightData.size();
    if (size) memcpy(pData + offset, defUBO_.spotLightData.data(), size);
    offset += size;

    vkUnmapMemory(dev_, UBOResource_.memory);
}

void Guppy::updateUniformBuffer() {
    // Surface changed
    auto aspect = static_cast<float>(extent_.width) / static_cast<float>(extent_.height);

    // auto x1 = sizeof(DefaultUBO);
    // auto x2 = sizeof(DefaultUBO::camera);
    // auto x3 = sizeof(DefaultUBO::light);
    // auto x4 = offsetof(DefaultUBO, camera);
    // auto x5 = offsetof(DefaultUBO, light);

    // auto y1 = sizeof(Light::Ambient::Data);
    // auto y2 = offsetof(Light::Ambient::Data, model);
    // auto y3 = offsetof(Light::Ambient::Data, color);
    // auto y4 = offsetof(Light::Ambient::Data, flags);
    // auto y5 = offsetof(Light::Ambient::Data, intensity);
    // auto y6 = offsetof(Light::Ambient::Data, phongExp);

    // Update the camera...
    auto pos_dir = InputHandler::getPosDir();
    auto look_dir = InputHandler::getLookDir();
    camera_.update(aspect, pos_dir, look_dir);
    defUBO_.pCamera = camera_.getData();

    // Update the lights... (!!!!!! THE NUMBER OF LIGHTS HERE CANNOT CHANGE AT RUNTIME YET !!!!!
    // Need to recreate the uniform descriptors!)
    for (size_t i = 0; i < positionalLights_.size(); i++) {
        auto& uboLight = defUBO_.positionalLightData[i];
        // set data (TODO: this is terrible)
        positionalLights_[i].getData(uboLight);
        uboLight.position = camera_.getCameraSpacePosition(uboLight.position);
    }
    for (size_t i = 0; i < spotLights_.size(); i++) {
        auto& uboLight = defUBO_.spotLightData[i];
        // set data (TODO: this is terrible)
        spotLights_[i].getData(defUBO_.spotLightData[i]);
        uboLight.position = camera_.getCameraSpacePosition(uboLight.position);
        uboLight.direction = camera_.getCameraSpaceDirection(uboLight.direction);
    }

    // If these change update them here...
    // uboResource_.info.offset = 0;
    // uboResource_.info.range = sizeof(DefaultUBO);

    copyUniformBufferMemory();
}

void Guppy::destroyUniformBuffer() {
    vkDestroyBuffer(dev_, UBOResource_.buffer, nullptr);
    vkFreeMemory(dev_, UBOResource_.memory, nullptr);
}

void Guppy::create_frame_data(int count) {
    const Shell::Context& ctx = shell_->context();

    frame_data_.resize(count);

    create_fences();

    if (!use_push_constants_) {
        create_color_resources();
        create_depth_resources();
        updateUniformBuffer();
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
    helpers::createImage(dev_, CmdBufHandler::getUniqueQueueFamilies(true, false, true), shell_->context().num_samples,
                         format_, VK_IMAGE_TILING_OPTIMAL,
                         VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, extent_.width, extent_.height, 1, 1, color_resource_.image,
                         color_resource_.memory);

    helpers::createImageView(dev_, color_resource_.image, 1, format_, VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1,
                             color_resource_.view);

    helpers::transitionImageLayout(CmdBufHandler::graphics_cmd(), color_resource_.image, format_, VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 1, 1);

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

    helpers::createImage(dev_, CmdBufHandler::getUniqueQueueFamilies(true, false, true), shell_->context().num_samples,
                         depth_resource_.format, tiling, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, extent_.width, extent_.height, 1, 1, depth_resource_.image,
                         depth_resource_.memory);

    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (helpers::hasStencilComponent(depth_resource_.format)) {
        aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    helpers::createImageView(dev_, depth_resource_.image, 1, depth_resource_.format, aspectFlags, VK_IMAGE_VIEW_TYPE_2D, 1,
                             depth_resource_.view);

    helpers::transitionImageLayout(CmdBufHandler::graphics_cmd(), depth_resource_.image, depth_resource_.format,
                                   VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, 1, 1);

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

void Guppy::createLights() {
    // TODO: move the light code into the scene, including ubo data
    positionalLights_.push_back(Light::Positional());
    positionalLights_.back().transform(helpers::affine(glm::vec3(1.0f), glm::vec3(20.5f, 10.5f, -23.5f)));

    positionalLights_.push_back(Light::Positional());
    positionalLights_.back().transform(helpers::affine(glm::vec3(1.0f), {-2.5f, 4.5f, -1.5f}));

    // positionalLights_.push_back(Light::Positional());
    // positionalLights_.back().transform(helpers::affine(glm::vec3(1.0f), glm::vec3(-10.0f, 30.0f, 6.0f)));

    spotLights_.push_back({});
    spotLights_.back().transform(helpers::viewToWorld({0.0f, 4.5f, 1.0f}, {0.0f, 0.0f, -1.5f}, UP_VECTOR));
    spotLights_.back().setCutoff(glm::radians(25.0f));
    spotLights_.back().setExponent(25.0f);
}

void Guppy::createScenes() {
    SceneHandler::makeScene(UBOResource_, true);  // default active scene

    // Add defaults
    if (true) {
        // GROUND PLANE
        auto pMaterial = std::make_unique<Material>(TextureHandler::getTextureByPath(HARDWOOD_FLOOR_TEX_PATH));
        pMaterial->setRepeat(800.0f);
        glm::mat4 model = helpers::affine(glm::vec3(2000.0f), {}, -M_PI_2_FLT, CARDINAL_X);
        auto pGroundPlane = std::make_unique<TexturePlane>(std::move(pMaterial), model);
        auto groundPlane_bbmm = pGroundPlane->getBoundingBoxMinMax();
        SceneHandler::getActiveScene()->addMesh(shell_->context(), std::move(pGroundPlane));

        // ORIGIN AXES
        std::unique_ptr<LineMesh> pDefaultAxes = std::make_unique<Axes>(glm::mat4(1.0f), AXES_MAX_SIZE, true);
        SceneHandler::getActiveScene()->moveMesh(shell_->context(), std::move(pDefaultAxes));

        // model = helpers::affine(glm::vec3(2.0f), (CARDINAL_Y * 0.1f), -M_PI_2_FLT, CARDINAL_X);
        // auto p1 = std::make_unique<TexturePlane>(getTextureByPath(WOOD_007_DIFF_TEX_PATH), model, true);
        // SceneHandler::getActiveScene()->addMesh(shell_->context(), std::move(p1));

        // BURNT ORANGE TORUS
        Material material;
        material.setFlags(Material::FLAGS::PER_MATERIAL_COLOR | Material::FLAGS::MODE_BLINN_PHONG);
        material.setColor({0.8f, 0.3f, 0.0f});
        ModelHandler::makeModel(SceneHandler::getActiveScene(), TORUS_MODEL_PATH, material,
                                helpers::affine(glm::vec3(0.07f)), false,
                                [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); });

        // auto pTorus = std::make_unique<ColorMesh>(std::move(pMaterial), TORUS_MODEL_PATH, model);
        // SceneHandler::getActiveScene()->addMesh(shell_->context(), std::move(pTorus), true,
        //                        [groundPlane_bbmm](auto pMesh) { pMesh->putOnTop(groundPlane_bbmm); });

        // ORANGE
        model = helpers::affine(glm::vec3(1.0f), {6.0f, 0.0f, 0.0f});
        ModelHandler::makeModel(SceneHandler::getActiveScene(), ORANGE_MODEL_PATH, material, model, nullptr, true,
                                [groundPlane_bbmm](auto pmodel) { pmodel->putOnTop(groundPlane_bbmm); });

        // MEDIEVAL HOUSE
        model = helpers::affine(glm::vec3(0.0175f), {-6.5f, 0.0f, -3.5f}, M_PI_4_FLT, CARDINAL_Y);
        ModelHandler::makeModel(SceneHandler::getActiveScene(), MED_H_MODEL_PATH, {}, model,
                                TextureHandler::getTextureByPath(MED_H_DIFF_TEX_PATH), true,
                                [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); });
    }

    // Lights
    if (showLightHelpers_) {
        for (auto& light : positionalLights_) {
            std::unique_ptr<LineMesh> pHelper = std::make_unique<VisualHelper>(light, 0.5f);
            SceneHandler::getActiveScene()->moveMesh(shell_->context(), std::move(pHelper));
        }
        for (auto& light : spotLights_) {
            std::unique_ptr<LineMesh> pHelper = std::make_unique<VisualHelper>(light, 0.5f);
            SceneHandler::getActiveScene()->moveMesh(shell_->context(), std::move(pHelper));
        }
    }
}
