
#include <algorithm>

#include "Axes.h"
#include "CmdBufHandler.h"
#include "Constants.h"
#include "Game.h"
#include "Guppy.h"
#include "InputHandler.h"
#include "ModelHandler.h"
#include "LoadingResourceHandler.h"
#include "Model.h"
#include "ModelHandler.h"
#include "PipelineHandler.h"
#include "Plane.h"
#include "SceneHandler.h"
#include "ShaderHandler.h"
#include "Shell.h"
#include "TextureHandler.h"
#include "UIHandler.h"

#ifdef USE_DEBUG_GUI
#include "ImGuiUI.h"
#include "ImGuiRenderPass.h"
#endif

Guppy::Guppy(const std::vector<std::string>& args)
    : Game("Guppy", args),
      // TODO: use these or get rid of them...
      multithread_(true),
      use_push_constants_(false),
      sim_paused_(false),
      sim_fade_(false),
      // sim_(5000),
      frameIndex_(0),
      pDefaultRenderPass_(std::make_unique<RenderPass::Default>()),
#ifdef USE_DEBUG_GUI
      pUIRenderPass_(std::make_unique<ImGuiRenderPass>()),
#else
      pUIRenderPass_(std::make_unique<RenderPass::Default>()),
#endif
      camera_(glm::vec3(2.0f, 2.0f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f),
              static_cast<float>(settings_.initial_width) / static_cast<float>(settings_.initial_height)) {
    // ARGS
    for (auto it = args.begin(); it != args.end(); ++it) {
        if (*it == "-s")
            multithread_ = false;
        else if (*it == "-p")
            use_push_constants_ = true;
    }

    // init_workers();
}

Guppy::~Guppy() {}

void Guppy::attachShell(Shell& sh) {
    Game::attachShell(sh);
    const auto& ctx = shell_->context();

    // if (use_push_constants_ && sizeof(ShaderParamBlock) > physical_dev_props_.limits.maxPushConstantsSize) {
    //    shell_->log(Shell::LOG_WARN, "cannot enable push constants");
    //    use_push_constants_ = false;
    //}

    CmdBufHandler::init(ctx);

#ifdef USE_DEBUG_GUI
    UIHandler::init(&sh, settings_, sh.getUI());
#else
    UIHandler::init(&sh, settings_);
#endif

    LoadingResourceHandler::init(ctx);
    TextureHandler::init(&sh, settings_);

    Shader::Handler::init(sh, settings_, camera_);
    Pipeline::Handler::init(&sh, settings_);

    initRenderPasses();

    ModelHandler::init(&sh, settings_);
    SceneHandler::init(&sh, settings_);

    createScenes();

    if (multithread_) {
        // for (auto &worker : workers_) worker->start();
    }
}

void Guppy::attachSwapchain() {
    const auto& ctx = shell_->context();

    // SWAPCHAIN
    createSwapchainResources(ctx);

    // DEFAULT RENDER PASS
    RenderPass::FrameInfo frameInfo = {};
    frameInfo.extent = ctx.extent;
    frameInfo.viewCount = static_cast<uint32_t>(swapchainResources_.views.size());
    frameInfo.pViews = swapchainResources_.views.data();
    pDefaultRenderPass_->createTarget(ctx, settings_, &frameInfo);

    // IMGUI RENDER PASS
    pUIRenderPass_->createTarget(ctx, settings_, &frameInfo);
}

void Guppy::createSwapchainResources(const Shell::Context& ctx) {
    uint32_t imageCount;
    vk::assert_success(vkGetSwapchainImagesKHR(ctx.dev, ctx.swapchain, &imageCount, nullptr));

    // Get the image resources from the swapchain...
    swapchainResources_.images.resize(imageCount);
    swapchainResources_.views.resize(imageCount);
    vk::assert_success(vkGetSwapchainImagesKHR(ctx.dev, ctx.swapchain, &imageCount, swapchainResources_.images.data()));

    for (uint32_t i = 0; i < imageCount; i++) {
        helpers::createImageView(ctx.dev, swapchainResources_.images[i], 1, ctx.surfaceFormat.format,
                                 VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_VIEW_TYPE_2D, 1, swapchainResources_.views[i]);
    }
}

void Guppy::initRenderPasses() {
    const auto& ctx = shell_->context();

    RenderPass::InitInfo initInfo;
    // DEFAULT RENDER PASS
    initInfo = {};
    initInfo.clearColor = true;
    initInfo.clearDepth = true;
    initInfo.format = ctx.surfaceFormat.format;
    initInfo.depthFormat = ctx.depthFormat;
#ifdef USE_DEBUG_GUI
    initInfo.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#endif
    initInfo.samples = ctx.samples;
    initInfo.commandCount = ctx.imageCount;
    initInfo.fenceCount = ctx.imageCount;
    initInfo.semaphoreCount = ctx.imageCount;
    pDefaultRenderPass_->init(ctx, settings_, &initInfo);

    Pipeline::Handler::createPipelines(pDefaultRenderPass_);

#ifdef USE_DEBUG_GUI
    // IMGUI RENDER PASS
    initInfo = {};
    initInfo.format = ctx.surfaceFormat.format;
    initInfo.commandCount = ctx.imageCount;
    initInfo.waitDstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    pUIRenderPass_->init(ctx, settings_, &initInfo);

    shell_->initUI(pUIRenderPass_->pass);
#endif
}

void Guppy::onFrame(float framePred) {
    const auto& ctx = shell_->context();

    auto& fence = pDefaultRenderPass_->data.fences[frameIndex_];
    // wait for the last submission since we reuse frame data
    vk::assert_success(vkWaitForFences(ctx.dev, 1, &fence, true, UINT64_MAX));
    vk::assert_success(vkResetFences(ctx.dev, 1, &fence));

    const Shell::BackBuffer& back = ctx.acquiredBackBuffer;

    // **********************
    //  Pre-draw
    // **********************

    // Surface changed
    float aspect = 1.0f;
    if (ctx.extent.width && ctx.extent.height)
        aspect = static_cast<float>(ctx.extent.width) / static_cast<float>(ctx.extent.height);
    // Update the camera...
    auto pos_dir = InputHandler::getPosDir();
    auto look_dir = InputHandler::getLookDir();
    camera_.update(aspect, pos_dir, look_dir);

    Shader::Handler::updateDefaultUniform(camera_);

    // **********************
    //  Draw
    // **********************

    std::vector<SubmitResource> resources;
    SubmitResource resource;

    // SCENE
    SceneHandler::getActiveScene()->record(ctx, frameIndex_, pDefaultRenderPass_);

    resource = {};
    resource.waitSemaphores.push_back(back.acquireSemaphore);    // wait for back buffer...
    resource.waitDstStageMasks.push_back(ctx.waitDstStageMask);  // back buffer flags...
    pDefaultRenderPass_->getSubmitResource(frameIndex_, resource);
    resources.push_back(resource);

    // UI
    shell_->getUI()->draw(pUIRenderPass_, frameIndex_);

    resource = {};
    resource.waitSemaphores.push_back(
        pDefaultRenderPass_->data.semaphores[frameIndex_]);     // wait on scene... (must be earlier in submission scope)
    resource.signalSemaphores.push_back(back.renderSemaphore);  // signal back buffer...
    pUIRenderPass_->getSubmitResource(frameIndex_, resource);
    resources.push_back(resource);

    submitRenderPasses(resources, fence);

    // **********************
    //  Post-draw
    // **********************

    Shader::Handler::cleanup();
    Pipeline::Handler::cleanup(frameIndex_);
    SceneHandler::cleanupInvalidResources();

    // **********************

    frameIndex_ = (frameIndex_ + 1) % static_cast<uint8_t>(ctx.imageCount);
}

/*  This function is for updating things regardless of framerate. It is based on settings.ticks_per_second,
    and should be called that many times per second. add_game_time is weird and appears to limit the amount
    of ticks per second arbitrarily, so this in reality could do anything.
    NOTE: Things like input should not used here since this is not guaranteed to be called each time input
    is collected. This function could be called as many as
*/
void Guppy::onTick() {
    if (sim_paused_) return;

    auto& pScene = SceneHandler::getActiveScene();

    // TODO: Should this be "on_frame"? every other frame? async? ... I have no clue yet.
    LoadingResourceHandler::cleanup();
    TextureHandler::update();

    // TODO: move to SceneHandler::update or something!
    ModelHandler::update(pScene);

    Pipeline::Handler::update();

    pScene->update(settings_, shell_->context());

    // for (auto &worker : workers_) worker->update_simulation();
}

void Guppy::createScenes() {
    // default active scene
    SceneHandler::makeScene(true, true);

    MeshCreateInfo meshCreateInfo;
    ModelCreateInfo modelCreateInfo;

    // Add defaults
    if (true) {
        // GROUND PLANE
        meshCreateInfo = {};
        meshCreateInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
        meshCreateInfo.materialInfo.pTexture = TextureHandler::getTextureByPath(HARDWOOD_FLOOR_TEX_PATH);
        meshCreateInfo.materialInfo.repeat = 800.0f;
        meshCreateInfo.model = helpers::affine(glm::vec3(2000.0f), {}, -M_PI_2_FLT, CARDINAL_X);
        meshCreateInfo.selectable = false;

        auto pGroundPlane = std::make_unique<TexturePlane>(&meshCreateInfo);
        auto groundPlane_bbmm = pGroundPlane->getBoundingBoxMinMax();
        SceneHandler::getActiveScene()->moveMesh(settings_, shell_->context(), std::move(pGroundPlane));

        // ORIGIN AXES
        meshCreateInfo = {};
        meshCreateInfo.pipelineType = PIPELINE::LINE;
        meshCreateInfo.isIndexed = false;
        std::unique_ptr<LineMesh> pDefaultAxes = std::make_unique<Axes>(&meshCreateInfo, AXES_MAX_SIZE, true);
        SceneHandler::getActiveScene()->moveMesh(settings_, shell_->context(), std::move(pDefaultAxes));

        // BURNT ORANGE TORUS
        modelCreateInfo = {};
        modelCreateInfo.pipelineType = PIPELINE::PBR_COLOR;
        // modelCreateInfo.pipelineType = PIPELINE_TYPE::TRI_LIST_COLOR;
        modelCreateInfo.async = true;
        modelCreateInfo.callback = [groundPlane_bbmm](Model* pModel) {
            pModel->putOnTop(groundPlane_bbmm);
            //// This is shitty, and was unforseen when I made the Model/ModelHandler
            // auto& pMesh = SceneHandler::getColorMesh(pModel->getSceneOffset(), pModel->getMeshOffset(MESH_TYPE::COLOR,
            // 0)); auto model = helpers::affine(glm::vec3(1.0f), {0.0f, 6.0f, 0.0f}) * pMesh->getData().model;
            // pMesh->addInstance(model, std::make_unique<Material>(pMesh->getMaterial()));
        };
        modelCreateInfo.materialInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::MODE_BLINN_PHONG | Material::FLAG::METAL;
        modelCreateInfo.materialInfo.ambientCoeff = {0.8f, 0.3f, 0.0f};
        modelCreateInfo.materialInfo.diffuseCoeff = {0.8f, 0.3f, 0.0f};
        modelCreateInfo.materialInfo.roughness = 0.5f;
        modelCreateInfo.model = helpers::affine(glm::vec3(0.07f));
        modelCreateInfo.modelPath = TORUS_MODEL_PATH;
        modelCreateInfo.smoothNormals = true;
        ModelHandler::makeModel(&modelCreateInfo, SceneHandler::getActiveScene());

        // ORANGE
        modelCreateInfo = {};
        modelCreateInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
        modelCreateInfo.async = true;
        modelCreateInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
        modelCreateInfo.needsTexture = true;
        modelCreateInfo.modelPath = ORANGE_MODEL_PATH;
        modelCreateInfo.model = helpers::affine(glm::vec3(1.0f), {4.0f, 0.0f, -2.0f});
        modelCreateInfo.smoothNormals = true;
        // modelCreateInfo.visualHelper = true;
        ModelHandler::makeModel(&modelCreateInfo, SceneHandler::getActiveScene());

        ////// PEAR
        //// modelCreateInfo.pipelineType = PIPELINE_TYPE::TRI_LIST_TEX;
        //// modelCreateInfo.async = true;
        //// modelCreateInfo.material = {};
        //// modelCreateInfo.modelPath = PEAR_MODEL_PATH;
        //// modelCreateInfo.model = helpers::affine();
        //// ModelHandler::makeModel(&modelCreateInfo, SceneHandler::getActiveScene(), nullptr);

        // MEDIEVAL HOUSE
        modelCreateInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
        modelCreateInfo.async = true;
        modelCreateInfo.model = helpers::affine(glm::vec3(0.0175f), {-6.5f, 0.0f, -3.5f}, M_PI_4_FLT, CARDINAL_Y);
        modelCreateInfo.modelPath = MED_H_MODEL_PATH;
        modelCreateInfo.materialInfo.pTexture = TextureHandler::getTextureByPath(MED_H_DIFF_TEX_PATH);
        modelCreateInfo.smoothNormals = false;
        ModelHandler::makeModel(&modelCreateInfo, SceneHandler::getActiveScene());

        // PIG
        modelCreateInfo = {};
        // modelCreateInfo.pipelineType = PIPELINE_TYPE::WHATEVER;
        modelCreateInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
        modelCreateInfo.async = true;
        modelCreateInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
        modelCreateInfo.materialInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::MODE_BLINN_PHONG;
        modelCreateInfo.materialInfo.ambientCoeff = {0.8f, 0.8f, 0.8f};
        modelCreateInfo.materialInfo.diffuseCoeff = {0.8f, 0.8f, 0.8f};
        modelCreateInfo.model = helpers::affine(glm::vec3(2.0f), {0.0f, 0.0f, -4.0f});
        modelCreateInfo.modelPath = PIG_MODEL_PATH;
        modelCreateInfo.smoothNormals = true;
        ModelHandler::makeModel(&modelCreateInfo, SceneHandler::getActiveScene());
    }

    // Lights
    if (false) {
        Shader::Handler::defaultLightsAction([this](auto& posLights, auto& spotLights) {
            MeshCreateInfo meshCreateInfo;
            for (auto& light : posLights) {
                meshCreateInfo = {};
                meshCreateInfo.isIndexed = false;
                meshCreateInfo.model = light.getData().model;

                std::unique_ptr<LineMesh> pHelper = std::make_unique<VisualHelper>(&meshCreateInfo, 0.5f);
                SceneHandler::getActiveScene()->moveMesh(settings_, shell_->context(), std::move(pHelper));
            }
            for (auto& light : spotLights) {
                meshCreateInfo = {};
                meshCreateInfo.isIndexed = false;
                meshCreateInfo.model = light.getData().model;

                std::unique_ptr<LineMesh> pHelper = std::make_unique<VisualHelper>(&meshCreateInfo, 0.5f);
                SceneHandler::getActiveScene()->moveMesh(settings_, shell_->context(), std::move(pHelper));
            }
        });
    }
}

void Guppy::onKey(GAME_KEY key) {
    switch (key) {
        case GAME_KEY::KEY_SHUTDOWN:
        case GAME_KEY::KEY_ESC:
            shell_->quit();
            break;
        case GAME_KEY::KEY_SPACE:
            // sim_paused_ = !sim_paused_;
            break;
        case GAME_KEY::KEY_F:
            // sim_fade_ = !sim_fade_;
            break;
        case GAME_KEY::KEY_1: {
            Shader::Handler::defaultLightsAction([](auto& posLights, auto& spotLights) {
                if (posLights.size() > 1) {
                    auto& light = posLights[1];
                    light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_X * -2.0f)));
                }
            });
        } break;
        case GAME_KEY::KEY_2: {
            Shader::Handler::defaultLightsAction([](auto& posLights, auto& spotLights) {
                if (posLights.size() > 1) {
                    auto& light = posLights[1];
                    light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_X * 2.0f)));
                }
            });
        } break;
        case GAME_KEY::KEY_3: {
            Shader::Handler::defaultLightsAction([](auto& posLights, auto& spotLights) {
                if (posLights.size() > 1) {
                    auto& light = posLights[1];
                    light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_Z * 2.0f)));
                }
            });
        } break;
        case GAME_KEY::KEY_4: {
            Shader::Handler::defaultLightsAction([](auto& posLights, auto& spotLights) {
                if (posLights.size() > 1) {
                    auto& light = posLights[1];
                    light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_Z * -2.0f)));
                }
            });
        } break;
        case GAME_KEY::KEY_5: {
            Shader::Handler::defaultLightsAction([](auto& posLights, auto& spotLights) {
                if (posLights.size() > 1) {
                    auto& light = posLights[1];
                    light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_Y * 2.0f)));
                }
            });
        } break;
        case GAME_KEY::KEY_6: {
            Shader::Handler::defaultLightsAction([](auto& posLights, auto& spotLights) {
                if (posLights.size() > 1) {
                    auto& light = posLights[1];
                    light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_Y * -2.0f)));
                }
            });
        } break;
        case GAME_KEY::KEY_7: {
            Shader::Handler::defaultUniformAction([](auto& defUBO) {
                // Cycle through fog types
                if (defUBO.shaderData.flags & Uniform::Default::FLAG::FOG_EXP) {
                    defUBO.shaderData.flags ^= Uniform::Default::FLAG::FOG_EXP;
                    defUBO.shaderData.flags = Uniform::Default::FLAG::FOG_EXP2;
                } else if (defUBO.shaderData.flags & Uniform::Default::FLAG::FOG_EXP2) {
                    defUBO.shaderData.flags ^= Uniform::Default::FLAG::FOG_EXP2;
                    defUBO.shaderData.flags = Uniform::Default::FLAG::FOG_LINEAR;
                } else {
                    defUBO.shaderData.flags ^= Uniform::Default::FLAG::FOG_LINEAR;
                    defUBO.shaderData.flags = Uniform::Default::FLAG::FOG_EXP;
                }
                // defUBO_.shaderData.fog.maxDistance += 10.0f;
            });
        } break;
        case GAME_KEY::KEY_8: {
            Shader::Handler::defaultUniformAction(
                [](auto& defUBO) { defUBO.shaderData.flags ^= Uniform::Default::FLAG::TOON_SHADE; });
        } break;
        default:
            break;
    }
}

void Guppy::onMouse(const MouseInput& input) {
    if (input.moving) {
        // const Shell::Context& ctx = shell_->context();
        // auto ray = camera_.getRay({input.xPos, input.yPos}, ctx.extent);
        // SceneHandler::select(ctx.dev, ray);
    }
    if (input.primary) {
        const Shell::Context& ctx = shell_->context();
        auto ray = camera_.getRay({input.xPos, input.yPos}, ctx.extent);
        SceneHandler::select(ctx.dev, ray);
    }
}

void Guppy::detachShell() {
    const auto& ctx = shell_->context();

    // TODO: kill futures !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    if (multithread_) {
        // for (auto &worker : workers_) worker->stop();
    }

    SceneHandler::destroy();
    CmdBufHandler::destroy();
    Pipeline::Handler::destroy();
    Shader::Handler::destroy();
    TextureHandler::destroy();

    // RENDER PASS
    if (pDefaultRenderPass_ != nullptr) pDefaultRenderPass_->destroy(ctx.dev);
    if (pUIRenderPass_ != nullptr) pUIRenderPass_->destroy(ctx.dev);

    LoadingResourceHandler::cleanup();

    Game::detachShell();
}

void Guppy::detachSwapchain() {
    const auto& ctx = shell_->context();

    // RENDER PASSES
    pDefaultRenderPass_->destroyTargetResources(ctx.dev);
    pUIRenderPass_->destroyTargetResources(ctx.dev);

    // SWAPCHAIN
    destroySwapchainResources(ctx);

    CmdBufHandler::resetCmdBuffers();
}

void Guppy::destroySwapchainResources(const Shell::Context& ctx) {
    for (auto& view : swapchainResources_.views) vkDestroyImageView(ctx.dev, view, nullptr);
    swapchainResources_.views.clear();
    // Images are destoryed by vkDestroySwapchainKHR
    swapchainResources_.images.clear();
}

void Guppy::submitRenderPasses(const std::vector<SubmitResource>& resources, VkFence fence) {
    std::vector<VkSubmitInfo> infos;
    infos.reserve(resources.size());

    for (const auto& resource : resources) {
        assert(resource.waitSemaphores.size() == resource.waitDstStageMasks.size());

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = static_cast<uint32_t>(resource.waitSemaphores.size());
        submitInfo.pWaitSemaphores = resource.waitSemaphores.data();
        submitInfo.pWaitDstStageMask = resource.waitDstStageMasks.data();
        submitInfo.commandBufferCount = static_cast<uint32_t>(resource.commandBuffers.size());
        submitInfo.pCommandBuffers = resource.commandBuffers.data();
        submitInfo.signalSemaphoreCount = static_cast<uint32_t>(resource.signalSemaphores.size());
        submitInfo.pSignalSemaphores = resource.signalSemaphores.data();
        infos.push_back(submitInfo);
    }

    vk::assert_success(
        vkQueueSubmit(CmdBufHandler::graphics_queue(), static_cast<uint32_t>(infos.size()), infos.data(), fence));
}
