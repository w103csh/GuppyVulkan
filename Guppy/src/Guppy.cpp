
#include <algorithm>
#include <functional>

#include "Axes.h"
#include "Box.h"
#include "Constants.h"
#include "Game.h"
#include "Guppy.h"
#include "Mesh.h"
#include "Model.h"
#include "Plane.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "DescriptorHandler.h"
#include "InputHandler.h"
#include "LoadingHandler.h"
#include "MaterialHandler.h"
#include "MeshHandler.h"
#include "ModelHandler.h"
#include "PipelineHandler.h"
#include "SceneHandler.h"
#include "ShaderHandler.h"
#include "TextureHandler.h"
#include "UIHandler.h"
#include "UniformHandler.h"

#ifdef USE_DEBUG_UI
#include "ImGuiHandler.h"
#endif

Guppy::Guppy(const std::vector<std::string>& args)
    : Game("Guppy", args,
           // HANDLERS
           {std::make_unique<Command::Handler>(this), std::make_unique<Descriptor::Handler>(this),
            std::make_unique<Loading::Handler>(this), std::make_unique<Material::Handler>(this),
            std::make_unique<Mesh::Handler>(this), std::make_unique<Model::Handler>(this),
            std::make_unique<Pipeline::Handler>(this), std::make_unique<Scene::Handler>(this),
            std::make_unique<Shader::Handler>(this), std::make_unique<Texture::Handler>(this),
#ifdef USE_DEBUG_UI
            std::make_unique<UI::ImGuiHandler>(this),
#else
            std::make_unique<UI::Handler>(this),
#endif
            std::make_unique<Uniform::Handler>(this)}),
      // TODO: use these or get rid of them...
      multithread_(true),
      use_push_constants_(false),
      sim_paused_(false),
      sim_fade_(false),
      // sim_(5000),
      frameIndex_(0),
      pDefaultRenderPass_(std::make_unique<RenderPass::Default>()) {
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
    const auto& ctx = shell().context();

    // if (use_push_constants_ && sizeof(ShaderParamBlock) > physical_dev_props_.limits.maxPushConstantsSize) {
    //    shell().log(Shell::LOG_WARN, "cannot enable push constants");
    //    use_push_constants_ = false;
    //}

    /*
        TODO: the "Handlers" should be constructed by Guppy's constructor. You would
        just have to ensure that "Shell", "Settings", and "Context" don't have
        their address changed, which might be the case already...
    */

    // HANDLERS
    handlers_.pCommand->init();
    handlers_.pLoading->init();
    handlers_.pMaterial->init();
    handlers_.pTexture->init();
    handlers_.pUniform->init();
    handlers_.pShader->init();
    handlers_.pDescriptor->init();
    handlers_.pPipeline->init();
    initRenderPasses();
    handlers_.pPipeline->createPipelines(pDefaultRenderPass_);
    handlers_.pUI->init();
    handlers_.pModel->init();
    handlers_.pScene->init();

    createScenes();

    // SHELL LISTENERS
    if (settings().enable_directory_listener) {
        watchDirectory(Shader::BASE_DIRNAME,
                       std::bind(&Shader::Handler::recompileShader, handlers_.pShader.get(), std::placeholders::_1));
    }

    if (multithread_) {
        // for (auto &worker : workers_) worker->start();
    }
}

void Guppy::attachSwapchain() {
    const auto& ctx = shell().context();
    // SWAPCHAIN
    createSwapchainResources();
    // RENDER PASS
    updateRenderPasses();
    // EXTENT
    float aspect = 1.0f;
    if (ctx.extent.width && ctx.extent.height)
        aspect = static_cast<float>(ctx.extent.width) / static_cast<float>(ctx.extent.height);
    handlers_.pUniform->getMainCamera().setAspect(aspect);
}

void Guppy::createSwapchainResources() {
    const auto& ctx = shell().context();

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
    const auto& ctx = shell().context();

    RenderPass::InitInfo initInfo;
    // DEFAULT RENDER PASS
    initInfo = {};
    initInfo.clearColor = true;
    initInfo.clearDepth = true;
    initInfo.format = ctx.surfaceFormat.format;
    initInfo.depthFormat = ctx.depthFormat;
#ifdef USE_DEBUG_UI
    initInfo.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
#endif
    initInfo.samples = ctx.samples;
    initInfo.commandCount = ctx.imageCount;
    initInfo.fenceCount = ctx.imageCount;
    initInfo.semaphoreCount = ctx.imageCount;
    pDefaultRenderPass_->init(ctx, settings(), &initInfo);
}

void Guppy::updateRenderPasses() {
    const auto& ctx = shell().context();

    // DEFAULT
    RenderPass::FrameInfo frameInfo = {};
    frameInfo.extent = ctx.extent;
    frameInfo.viewCount = static_cast<uint32_t>(swapchainResources_.views.size());
    frameInfo.pViews = swapchainResources_.views.data();
    pDefaultRenderPass_->createTarget(ctx, settings(), std::ref(*handlers_.pCommand), &frameInfo);

    // UI
    handlers_.pUI->updateRenderPass(&frameInfo);
}

void Guppy::createScenes() {
    const auto& ctx = shell().context();

    handlers_.pScene->makeScene(true, true);

    // Create info structs
    Mesh::CreateInfo meshInfo;
    Model::CreateInfo modelInfo;
    AxesCreateInfo axesInfo;
    Material::Default::CreateInfo defMatInfo;
    Material::PBR::CreateInfo pbrMatInfo;

    // ORIGIN AXES
    axesInfo = {};
    axesInfo.lineSize = AXES_MAX_SIZE;
    axesInfo.showNegative = true;
    defMatInfo = {};
    defMatInfo.shininess = 23.123f;
    // pbrMatInfo = {};
    auto& pOriginAxes = handlers_.pMesh->makeLineMesh<Axes>(&axesInfo, &defMatInfo);

    // GROUND PLANE (TEXTURE)
    meshInfo = {};
    meshInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
    meshInfo.model = helpers::affine(glm::vec3{2000.0f}, {}, -M_PI_2_FLT, CARDINAL_X);
    meshInfo.selectable = false;
    defMatInfo = {};
    defMatInfo.pTexture = handlers_.pTexture->getTextureByPath(HARDWOOD_FLOOR_TEX_PATH);
    defMatInfo.repeat = 800.0f;
    defMatInfo.shininess = Material::SHININESS::EGGSHELL;
    auto& pGroundPlane = handlers_.pMesh->makeTextureMesh<Plane::Texture>(&meshInfo, &defMatInfo);
    auto groundPlane_bbmm = pGroundPlane->getBoundingBoxMinMax();

    //// GROUND PLANE (COLOR)
    // meshInfo = {};
    // meshInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
    // meshInfo.model = helpers::affine(glm::vec3{2000.0f}, {}, -M_PI_2_FLT, CARDINAL_X);
    // meshInfo.selectable = false;
    // defMatInfo = {};
    // defMatInfo.shininess = Material::SHININESS::EGGSHELL;
    // defMatInfo.diffuseCoeff = {0.5f, 0.5f, 0.5f};
    // auto& pGroundPlane = handlers_.pMesh->makeColorMesh<Plane::Color>(&meshInfo, &defMatInfo);
    // auto groundPlane_bbmm = pGroundPlane->getBoundingBoxMinMax();

    // BOX (TEXTURE)
    meshInfo = {};
    meshInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
    meshInfo.model = helpers::affine(glm::vec3{1.0f}, glm::vec3{0.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f});
    defMatInfo = {};
    defMatInfo.pTexture = handlers_.pTexture->getTextureByPath(VULKAN_TEX_PATH);
    defMatInfo.shininess = Material::SHININESS::EGGSHELL;
    auto& boxTexture = handlers_.pMesh->makeTextureMesh<Box::Texture>(&meshInfo, &defMatInfo);
    boxTexture->putOnTop(groundPlane_bbmm);
    handlers_.pMesh->makeTangentSpaceVisualHelper(boxTexture.get());

    // BOX (COLOR)
    meshInfo = {};
    meshInfo.pipelineType = PIPELINE::PBR_COLOR;
    meshInfo.model = helpers::affine(glm::vec3{1.0f}, glm::vec3{6.0f, 0.0f, 3.5f}, M_PI_2_FLT, glm::vec3{-1.0f, 0.0f, 1.0f});
    pbrMatInfo = {};
    pbrMatInfo.flags = Material::FLAG::METAL;
    pbrMatInfo.diffuseCoeff = {1.0f, 1.0f, 0.0f};
    auto& boxColor = handlers_.pMesh->makeColorMesh<Box::Color>(&meshInfo, &pbrMatInfo);
    boxColor->putOnTop(groundPlane_bbmm);

    return;

    // BURNT ORANGE TORUS
    // MODEL
    modelInfo = {};
    // modelCreateInfo.pipelineType = PIPELINE::PBR_COLOR;
    modelInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
    modelInfo.async = true;
    modelInfo.callback = [groundPlane_bbmm](auto pModel) {
        pModel->putOnTop(groundPlane_bbmm);
        //// This is shitty, and was unforseen when I made the Model/ModelHandler
        // auto& pMesh = SceneHandler::getColorMesh(pModel->getSceneOffset(),
        // pModel->getMeshOffset(MESH_TYPE::COLOR,
        // 0)); auto model = helpers::affine(glm::vec3{1.0f}, {0.0f, 6.0f, 0.0f}) * pMesh->getModel();
        // pMesh->addInstance(model, std::make_unique<Material>(pMesh->getMaterial()));
    };
    modelInfo.model = helpers::affine(glm::vec3(0.07f));
    modelInfo.modelPath = TORUS_MODEL_PATH;
    modelInfo.smoothNormals = true;
    // MATERIAL
    defMatInfo = {};
    defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::METAL;
    defMatInfo.ambientCoeff = {0.8f, 0.3f, 0.0f};
    defMatInfo.diffuseCoeff = {0.8f, 0.3f, 0.0f};
    // matCreateInfo.roughness = 0.5f;
    handlers_.pModel->makeColorModel(&modelInfo, &defMatInfo);

    // ORANGE
    // MODEL
    modelInfo = {};
    modelInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
    modelInfo.async = false;
    modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
    modelInfo.visualHelper = true;
    modelInfo.modelPath = ORANGE_MODEL_PATH;
    modelInfo.model = helpers::affine(glm::vec3{1.0f}, {4.0f, 0.0f, -2.0f});
    modelInfo.smoothNormals = true;
    // MATERIAL
    defMatInfo = {};
    handlers_.pModel->makeTextureModel(&modelInfo, &defMatInfo);

    //// PEAR
    // modelCreateInfo.pipelineType = PIPELINE_TYPE::TRI_LIST_TEX;
    // modelCreateInfo.async = true;
    // modelCreateInfo.material = {};
    // modelCreateInfo.modelPath = PEAR_MODEL_PATH;
    // modelCreateInfo.model = helpers::affine();
    // ModelHandler::makeModel(&modelCreateInfo, SceneHandler::getActiveScene(), nullptr);

    // MEDIEVAL HOUSE
    modelInfo = {};
    modelInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
    modelInfo.async = true;
    modelInfo.model = helpers::affine(glm::vec3{0.0175f}, {-6.5f, 0.0f, -3.5f}, M_PI_4_FLT, CARDINAL_Y);
    modelInfo.modelPath = MED_H_MODEL_PATH;
    modelInfo.smoothNormals = false;
    modelInfo.visualHelper = true;
    // MATERIAL
    defMatInfo = {};
    defMatInfo.pTexture = handlers_.pTexture->getTextureByPath(MED_H_DIFF_TEX_PATH);
    handlers_.pModel->makeTextureModel(&modelInfo, &defMatInfo);

    // PIG
    // MODEL
    modelInfo = {};
    modelInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
    modelInfo.async = true;
    modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
    modelInfo.model = helpers::affine(glm::vec3{2.0f}, {0.0f, 0.0f, -4.0f});
    modelInfo.modelPath = PIG_MODEL_PATH;
    modelInfo.smoothNormals = true;
    // MATERIAL
    defMatInfo = {};
    defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::MODE_BLINN_PHONG;
    defMatInfo.ambientCoeff = {0.8f, 0.8f, 0.8f};
    defMatInfo.diffuseCoeff = {0.8f, 0.8f, 0.8f};
    handlers_.pModel->makeColorModel(&modelInfo, &defMatInfo);
}

/*  This function is for updating things regardless of framerate. It is based on settings.ticks_per_second,
    and should be called that many times per second. add_game_time is weird and appears to limit the amount
    of ticks per second arbitrarily, so this in reality could do anything.
    NOTE: Things like input should not used here since this is not guaranteed to be called each time input
    is collected. This function could be called as many as
*/
void Guppy::onTick() {
    if (sim_paused_) return;

    auto& pScene = handlers_.pScene->getActiveScene();

    // TODO: Should this be "on_frame"? every other frame? async? ... I have no clue yet.
    handlers_.pLoading->cleanup();
    handlers_.pTexture->update();

    // TODO: move to SceneHandler::update or something!
    handlers_.pModel->update(pScene);

    handlers_.pPipeline->update();

    handlers_.pMesh->update();

    // for (auto &worker : workers_) worker->update_simulation();
}

void Guppy::onFrame(float framePred) {
    const auto& ctx = shell().context();

    auto& fence = pDefaultRenderPass_->data.fences[frameIndex_];
    // wait for the last submission since we reuse frame data
    vk::assert_success(vkWaitForFences(ctx.dev, 1, &fence, true, UINT64_MAX));
    vk::assert_success(vkResetFences(ctx.dev, 1, &fence));

    const Shell::BackBuffer& back = ctx.acquiredBackBuffer;

    // **********************
    //  Pre-draw
    // **********************

    handlers_.pUniform->update();

    // **********************
    //  Draw
    // **********************

    std::vector<SubmitResource> resources;
    SubmitResource resource;

    // SCENE
    handlers_.pScene->getActiveScene()->record(frameIndex_, pDefaultRenderPass_);

    resource = {};
    resource.waitSemaphores.push_back(back.acquireSemaphore);    // wait for back buffer...
    resource.waitDstStageMasks.push_back(ctx.waitDstStageMask);  // back buffer flags...
#ifndef USE_DEBUG_UI
    resource.signalSemaphores.push_back(back.renderSemaphore);  // signal back buffer...
#endif
    pDefaultRenderPass_->getSubmitResource(frameIndex_, resource);
    resources.push_back(resource);

#ifdef USE_DEBUG_UI
    handlers_.pUI->draw(frameIndex_);

    resource = {};
    resource.waitSemaphores.push_back(
        pDefaultRenderPass_->data.semaphores[frameIndex_]);     // wait on scene... (must be earlier in submission scope)
    resource.signalSemaphores.push_back(back.renderSemaphore);  // signal back buffer...
    handlers_.pUI->getPass()->getSubmitResource(frameIndex_, resource);
    resources.push_back(resource);
#endif

    submitRenderPasses(resources, fence);

    // **********************
    //  Post-draw
    // **********************

    handlers_.pShader->cleanup();
    handlers_.pPipeline->cleanup(frameIndex_);
    handlers_.pScene->cleanup();

    // **********************

    frameIndex_ = (frameIndex_ + 1) % static_cast<uint8_t>(ctx.imageCount);
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
        vkQueueSubmit(handlers_.pCommand->graphicsQueue(), static_cast<uint32_t>(infos.size()), infos.data(), fence));
}

void Guppy::onKey(GAME_KEY key) {
    switch (key) {
        case GAME_KEY::KEY_SHUTDOWN:
        case GAME_KEY::KEY_ESC:
            shell().quit();
            break;
        case GAME_KEY::KEY_SPACE:
            // sim_paused_ = !sim_paused_;
            break;
        case GAME_KEY::KEY_F:
            // sim_fade_ = !sim_fade_;
            break;
            // case GAME_KEY::KEY_1: {
            //    Shader::Handler::defaultLightsAction([](auto& posLights, auto& spotLights) {
            //        if (posLights.size() > 1) {
            //            auto& light = posLights[1];
            //            light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_X * -2.0f)));
            //        }
            //    });
            //} break;
            // case GAME_KEY::KEY_2: {
            //    Shader::Handler::defaultLightsAction([](auto& posLights, auto& spotLights) {
            //        if (posLights.size() > 1) {
            //            auto& light = posLights[1];
            //            light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_X * 2.0f)));
            //        }
            //    });
            //} break;
            // case GAME_KEY::KEY_3: {
            //    Shader::Handler::defaultLightsAction([](auto& posLights, auto& spotLights) {
            //        if (posLights.size() > 1) {
            //            auto& light = posLights[1];
            //            light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_Z * 2.0f)));
            //        }
            //    });
            //} break;
        case GAME_KEY::KEY_4: {
            // Shader::Handler::defaultLightsAction([](auto& posLights, auto& spotLights) {
            //    if (posLights.size() > 1) {
            //        auto& light = posLights[1];
            //        light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_Z * -2.0f)));
            //    }
            //});
            handlers_.pUniform->createVisualHelpers();
        } break;
        case GAME_KEY::KEY_5: {
            // Shader::Handler::defaultLightsAction([](auto& posLights, auto& spotLights) {
            //    if (posLights.size() > 1) {
            //        auto& light = posLights[1];
            //        light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_Y * 2.0f)));
            //    }
            //});
        } break;
        case GAME_KEY::KEY_6: {
            // Shader::Handler::defaultLightsAction([](auto& posLights, auto& spotLights) {
            //    if (posLights.size() > 1) {
            //        auto& light = posLights[1];
            //        light.transform(helpers::affine(glm::vec3(1.0f), (CARDINAL_Y * -2.0f)));
            //    }
            //});
        } break;
        case GAME_KEY::KEY_7: {
            // Shader::Handler::defaultUniformAction([](auto& defUBO) {
            //    // Cycle through fog types
            //    if (defUBO.shaderData.flags & Uniform::Default::FLAG::FOG_EXP) {
            //        defUBO.shaderData.flags ^= Uniform::Default::FLAG::FOG_EXP;
            //        defUBO.shaderData.flags = Uniform::Default::FLAG::FOG_EXP2;
            //    } else if (defUBO.shaderData.flags & Uniform::Default::FLAG::FOG_EXP2) {
            //        defUBO.shaderData.flags ^= Uniform::Default::FLAG::FOG_EXP2;
            //        defUBO.shaderData.flags = Uniform::Default::FLAG::FOG_LINEAR;
            //    } else {
            //        defUBO.shaderData.flags ^= Uniform::Default::FLAG::FOG_LINEAR;
            //        defUBO.shaderData.flags = Uniform::Default::FLAG::FOG_EXP;
            //    }
            //    // defUBO_.shaderData.fog.maxDistance += 10.0f;
            //});
        } break;
        case GAME_KEY::KEY_8: {
            // Shader::Handler::defaultUniformAction(
            //    [](auto& defUBO) { defUBO.shaderData.flags ^= Uniform::Default::FLAG::TOON_SHADE; });
        } break;
        default:
            break;
    }
}

void Guppy::onMouse(const MouseInput& input) {
    if (input.moving) {
        // auto ray = handlers_.pUniform->getMainCamera().getRay({input.xPos, input.yPos}, shell().context().extent);
        // handlers_.pScene->getActiveScene()->select(ray);
    }
    if (input.primary) {
        auto ray = handlers_.pUniform->getMainCamera().getRay({input.xPos, input.yPos}, shell().context().extent);
        handlers_.pScene->getActiveScene()->select(ray);
    }
}

void Guppy::detachSwapchain() {
    const auto& ctx = shell().context();

    // RENDER PASSES
    pDefaultRenderPass_->destroyTargetResources(ctx.dev, std::ref(*handlers_.pCommand));

    // UI
    handlers_.pUI->reset();

    // SWAPCHAIN
    destroySwapchainResources();

    handlers_.pCommand->resetCmdBuffers();
}

void Guppy::destroySwapchainResources() {
    const auto& ctx = shell().context();

    for (auto& view : swapchainResources_.views) vkDestroyImageView(ctx.dev, view, nullptr);
    swapchainResources_.views.clear();
    // Images are destoryed by vkDestroySwapchainKHR
    swapchainResources_.images.clear();
}

void Guppy::detachShell() {
    const auto& ctx = shell().context();

    // TODO: kill futures !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    if (multithread_) {
        // for (auto &worker : workers_) worker->stop();
    }

    handlers_.pScene->destroy();
    handlers_.pCommand->destroy();
    handlers_.pPipeline->destroy();
    handlers_.pShader->destroy();
    handlers_.pDescriptor->destroy();
    handlers_.pUniform->destroy();
    handlers_.pTexture->destroy();
    handlers_.pMaterial->destroy();
    handlers_.pMesh->destroy();
    handlers_.pScene->destroy();
    handlers_.pUI->destroy();
    // RENDER PASS
    if (pDefaultRenderPass_ != nullptr) pDefaultRenderPass_->destroy(ctx.dev);

    handlers_.pLoading->destroy();

    Game::detachShell();
}
