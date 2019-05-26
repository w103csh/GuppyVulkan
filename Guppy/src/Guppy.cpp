
#include <algorithm>
#include <functional>

#include "Axes.h"
#include "Box.h"
#include "Constants.h"
#include "Game.h"
#include "Geometry.h"
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
           Game::Handlers{std::make_unique<Command::Handler>(this), std::make_unique<Descriptor::Handler>(this),
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
    // const auto& ctx = shell().context();

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
    handlers_.pDescriptor->init();
    handlers_.pPipeline->init();
    handlers_.pShader->init();
    initRenderPasses();
    handlers_.pPipeline->createPipelines(pDefaultRenderPass_);
    handlers_.pUI->init();
    handlers_.pMesh->init();
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
    bool suppress = false;

    handlers_.pScene->makeScene(true, (!suppress || false));

    // Create info structs
    Mesh::CreateInfo meshInfo;
    Model::CreateInfo modelInfo;
    AxesCreateInfo axesInfo;
    Instance::Default::CreateInfo defInstInfo;
    Material::Default::CreateInfo defMatInfo;
    Material::PBR::CreateInfo pbrMatInfo;

    BoundingBoxMinMax groundPlane_bbmm;
    uint32_t count = 4;

    // ORIGIN AXES
    if (!suppress || true) {
        axesInfo = {};
        axesInfo.lineSize = 500.f;
        axesInfo.showNegative = true;
        defInstInfo = {};
        defMatInfo = {};
        defMatInfo.shininess = 23.123f;
        handlers_.pMesh->makeLineMesh<Axes>(&axesInfo, &defMatInfo, &defInstInfo);
    }

    // GROUND PLANE (TEXTURE)
    if (!suppress || false) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
        meshInfo.selectable = false;
        defInstInfo = {};
        defInstInfo.data.push_back({helpers::affine(glm::vec3{500.0f}, {}, -M_PI_2_FLT, CARDINAL_X)});
        defMatInfo = {};
        defMatInfo.pTexture = handlers_.pTexture->getTextureByName(Texture::HARDWOOD_CREATE_INFO.name);
        defMatInfo.repeat = 800.0f;
        defMatInfo.specularCoeff *= 0.5f;
        defMatInfo.shininess = Material::SHININESS::EGGSHELL;
        auto& pGroundPlane = handlers_.pMesh->makeTextureMesh<Plane::Texture>(&meshInfo, &defMatInfo, &defInstInfo);
        groundPlane_bbmm = pGroundPlane->getBoundingBoxMinMax();
    }

    // GROUND PLANE (COLOR)
    if (!suppress && false) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
        meshInfo.selectable = false;
        defInstInfo = {};
        defInstInfo.data.push_back({helpers::affine(glm::vec3{2000.0f}, {}, -M_PI_2_FLT, CARDINAL_X)});
        defMatInfo = {};
        defMatInfo.shininess = Material::SHININESS::EGGSHELL;
        defMatInfo.color = {0.5f, 0.5f, 0.5f};
        auto& pGroundPlane = handlers_.pMesh->makeColorMesh<Plane::Color>(&meshInfo, &defMatInfo, &defInstInfo);
        groundPlane_bbmm = pGroundPlane->getBoundingBoxMinMax();
    }

    // SINGLE PLANE
    if (!suppress && false) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
        meshInfo.selectable = false;
        defInstInfo = {};
        defInstInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{0.5f, 0.0f, 0.5f}, -M_PI_2_FLT, CARDINAL_X)});
        defMatInfo = {};
        defMatInfo.shininess = Material::SHININESS::EGGSHELL;
        defMatInfo.color = {0.5f, 0.5f, 0.5f};
        handlers_.pMesh->makeColorMesh<Plane::Color>(&meshInfo, &defMatInfo, &defInstInfo);
    }

    // SKYBOX
    if (!suppress || true) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::CUBE;
        meshInfo.selectable = false;
        meshInfo.geometryCreateInfo.invert = true;
        defInstInfo = {};
        defInstInfo.data.push_back({helpers::affine(glm::vec3{10.0f})});
        defMatInfo = {};
        defMatInfo.flags |= Material::FLAG::SKYBOX;
        auto& skybox = handlers_.pMesh->makeColorMesh<Box::Color>(&meshInfo, &defMatInfo, &defInstInfo);
    }

    // BOX (TEXTURE)
    if (!suppress || false) {
        meshInfo = {};
        // meshInfo.pipelineType = PIPELINE::PARALLAX_SIMPLE;
        meshInfo.pipelineType = PIPELINE::PARALLAX_STEEP;
        // meshInfo.pipelineType = PIPELINE::PBR_TEX;
        defInstInfo = {};
        defInstInfo.data.push_back(
            {helpers::affine(glm::vec3{1.0f}, glm::vec3{0.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
        defMatInfo = {};
        defMatInfo.pTexture = handlers_.pTexture->getTextureByName(Texture::MYBRICK_CREATE_INFO.name);
        defMatInfo.shininess = Material::SHININESS::EGGSHELL;
        auto& boxTexture1 = handlers_.pMesh->makeTextureMesh<Box::Texture>(&meshInfo, &defMatInfo, &defInstInfo);
        boxTexture1->putOnTop(groundPlane_bbmm);
        handlers_.pMesh->updateMesh(boxTexture1);
        handlers_.pMesh->makeTangentSpaceVisualHelper(boxTexture1.get());
    }
    // BOX (PBR_TEX)
    if (!suppress || false) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::PBR_TEX;
        // meshInfo.model = helpers::affine(glm::vec3{1.0f}, glm::vec3{1.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f,
        // 0.0f, 1.0f});
        defInstInfo = {};
        defInstInfo.data.push_back(
            {helpers::affine(glm::vec3{1.0f}, glm::vec3{1.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
        pbrMatInfo = {};
        pbrMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::METAL;
        pbrMatInfo.color = {1.0f, 1.0f, 0.0f};
        pbrMatInfo.roughness = 0.86f; //0.43f;
        pbrMatInfo.pTexture = handlers_.pTexture->getTextureByName(Texture::VULKAN_CREATE_INFO.name);
        auto& boxTexture2 = handlers_.pMesh->makeTextureMesh<Box::Texture>(&meshInfo, &pbrMatInfo, &defInstInfo);
        boxTexture2->putOnTop(groundPlane_bbmm);
        handlers_.pMesh->updateMesh(boxTexture2);
        handlers_.pMesh->makeTangentSpaceVisualHelper(boxTexture2.get());
    }
    // BOX (PBR_COLOR)
    if (!suppress || false) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::PBR_COLOR;
        // meshInfo.model = helpers::affine(glm::vec3{1.0f}, glm::vec3{2.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f,
        // 0.0f, 1.0f});
        defInstInfo = {};
        defInstInfo.data.push_back(
            {helpers::affine(glm::vec3{1.0f}, glm::vec3{2.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
        pbrMatInfo = {};
        pbrMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR;
        pbrMatInfo.color = {1.0f, 1.0f, 0.0f};
        auto& boxPbrColor = handlers_.pMesh->makeColorMesh<Box::Color>(&meshInfo, &pbrMatInfo, &defInstInfo);
        boxPbrColor->putOnTop(groundPlane_bbmm);
        handlers_.pMesh->updateMesh(boxPbrColor);
    }
    // BOX (TRI_LIST_COLOR)
    if (!suppress || false) {
        meshInfo = {};
        meshInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
        // meshInfo.model = helpers::affine(glm::vec3{1.0f}, glm::vec3{5.0f, 0.0f, 3.5f}, M_PI_2_FLT, glm::vec3{-1.0f,
        // 0.0f, 1.0f});
        // meshInfo.model =
        //    helpers::affine(glm::vec3{1.0f}, glm::vec3{-1.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f});
        defInstInfo = {};
        defInstInfo.data.push_back(
            {helpers::affine(glm::vec3{1.0f}, glm::vec3{-1.0f, 0.0f, -3.5f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
        defMatInfo = {};
        defMatInfo.color = {1.0f, 1.0f, 0.0f};
        auto& boxDefColor1 = handlers_.pMesh->makeColorMesh<Box::Color>(&meshInfo, &defMatInfo, &defInstInfo);
        boxDefColor1->putOnTop(groundPlane_bbmm);
        handlers_.pMesh->updateMesh(boxDefColor1);
    }
    // BOX (CUBE)
    if (!suppress || false) {
        meshInfo = {};
        defInstInfo = {};
        defInstInfo.data.push_back(
            {helpers::affine(glm::vec3{1.0f}, glm::vec3{0.0f, 7.0f, 0.0f}, M_PI_2_FLT, glm::vec3{1.0f, 0.0f, 1.0f})});
        defMatInfo = {};
        // defMatInfo.color = {0x2E / 255.0f, 0x40 / 255.0f, 0x53 / 255.0f};
        defMatInfo.color = COLOR_RED;
        if (false) {  // reflect defaults
            defMatInfo.flags |= Material::FLAG::REFLECT;
            defMatInfo.eta = 0.94f;
            defMatInfo.reflectionFactor = 0.85f;
        } else {  // reflect defaults
            defMatInfo.flags |= Material::FLAG::REFLECT | Material::FLAG::REFRACT;
            defMatInfo.eta = 0.94f;
            defMatInfo.reflectionFactor = 0.1f;
        }
        meshInfo.pipelineType = PIPELINE::CUBE;
        defMatInfo.pTexture = handlers_.pTexture->getTextureByName(Texture::SKYBOX_CREATE_INFO.name);
        handlers_.pMesh->makeColorMesh<Box::Color>(&meshInfo, &defMatInfo, &defInstInfo);
        ////
        // defInstInfo = {};
        // defInstInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{0.0f, 1.0f, 0.0f}, M_PI_2_FLT,
        // CARDINAL_Z)}); handlers_.pMesh->makeColorMesh<Plane::Color>(&meshInfo, &defMatInfo, &defInstInfo);
        ////
        // meshInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
        // defMatInfo = {};
        // defMatInfo.color = {0x2E / 255.0f, 0x40 / 255.0f, 0x53 / 255.0f};
        // defInstInfo = {};
        // defInstInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{1.0f, 0.0f, 0.0f})});
        // handlers_.pMesh->makeColorMesh<Plane::Color>(&meshInfo, &defMatInfo, &defInstInfo);
        ////
        // meshInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
        // defMatInfo = {};
        // defMatInfo.pTexture = handlers_.pTexture->getTextureByName(Texture::STATUE_CREATE_INFO.name);
        // defInstInfo = {};
        // defInstInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, glm::vec3{2.0f, 0.0f, 0.0f})});
        // handlers_.pMesh->makeTextureMesh<Plane::Texture>(&meshInfo, &defMatInfo, &defInstInfo);
    }

    // BURNT ORANGE TORUS
    if (!suppress || false) {
        // MODEL
        modelInfo = {};
        modelInfo.async = false;
        modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
        modelInfo.modelPath = TORUS_MODEL_PATH;
        modelInfo.smoothNormals = true;
        defInstInfo = {};
        defInstInfo.data.push_back({helpers::affine(glm::vec3{0.07f})});
        // MATERIAL
        if (false) {
            modelInfo.pipelineType = PIPELINE::PBR_COLOR;
            pbrMatInfo = {};
            pbrMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::METAL;
            pbrMatInfo.color = {0.8f, 0.3f, 0.0f};
            pbrMatInfo.roughness = 0.9f;
            handlers_.pModel->makeColorModel(&modelInfo, &pbrMatInfo, &defInstInfo);
        } else if (true) {
            modelInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
            defMatInfo = {};
            defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR;
            defMatInfo.ambientCoeff = {0.8f, 0.3f, 0.0f};
            defMatInfo.color = {0.8f, 0.3f, 0.0f};
            handlers_.pModel->makeColorModel(&modelInfo, &defMatInfo, &defInstInfo);
        } else {
            modelInfo.pipelineType = PIPELINE::CUBE;
            defMatInfo = {};
            pbrMatInfo.color = {0.8f, 0.3f, 0.0f};
            defMatInfo.flags |= Material::FLAG::REFLECT;
            defMatInfo.reflectionFactor = 0.85f;
            defMatInfo.pTexture = handlers_.pTexture->getTextureByName(Texture::SKYBOX_CREATE_INFO.name);
            handlers_.pModel->makeColorModel(&modelInfo, &defMatInfo, &defInstInfo);
        }
    }

    count = 100;
    // GRASS
    if (!suppress || false) {
        // MODEL
        modelInfo = {};
        modelInfo.pipelineType = PIPELINE::BP_TEX_CULL_NONE;
        modelInfo.async = false;
        // modelInfo.callback = [groundPlane_bbmm](auto pModel) {};
        modelInfo.visualHelper = false;
        modelInfo.modelPath = GRASS_LP_MODEL_PATH;
        modelInfo.smoothNormals = false;
        defInstInfo = {};
        defInstInfo.update = false;
        std::srand(static_cast<unsigned>(time(0)));
        float x, z;
        float f1 = 0.03f;
        // float f1 = 0.2f;
        float f2 = f1 * 5.0f;
        float low = f2 * 0.05f;
        float high = f2 * 3.0f;
        defInstInfo.data.reserve(static_cast<size_t>(count) * static_cast<size_t>(count) * 4);
        for (uint32_t i = 0; i < count; i++) {
            x = static_cast<float>(i) * f2;
            // x += low + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (high - low)));
            for (uint32_t j = 0; j < count; j++) {
                z = static_cast<float>(j) * f2;
                // z += low + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / (high - low)));
                auto scale = glm::vec3{f1};
                auto translate1 = glm::vec3{x, 0.0f, z};
                auto translate2 = glm::vec3{x, 0.0f, -z};
                defInstInfo.data.push_back({helpers::affine(scale, translate1)});
                if (i == 0 && j == 0) continue;
                defInstInfo.data.push_back({helpers::affine(scale, -translate1)});
                if (i == 0) continue;
                defInstInfo.data.push_back({helpers::affine(scale, translate2)});
                defInstInfo.data.push_back({helpers::affine(scale, -translate2)});
            }
        }
        // MATERIAL
        defMatInfo = {};
        handlers_.pModel->makeTextureModel(&modelInfo, &defMatInfo, &defInstInfo);
    }

    count = 5;
    // ORANGE
    if (!suppress || false) {
        // MODEL
        modelInfo = {};
        modelInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
        modelInfo.async = false;
        modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
        modelInfo.visualHelper = true;
        modelInfo.modelPath = ORANGE_MODEL_PATH;
        modelInfo.smoothNormals = true;
        defInstInfo = {};
        defInstInfo.update = false;
        defInstInfo.data.reserve(static_cast<size_t>(count) * static_cast<size_t>(count));
        for (uint32_t i = 0; i < count; i++) {
            float x = 4.0f + (static_cast<float>(i) * 2.0f);
            for (uint32_t j = 0; j < count; j++) {
                float z = -2.0f - (static_cast<float>(j) * 2.0f);
                defInstInfo.data.push_back({helpers::affine(glm::vec3{1.0f}, {x, 0.0f, z})});
            }
        }
        // MATERIAL
        defMatInfo = {};
        handlers_.pModel->makeTextureModel(&modelInfo, &defMatInfo, &defInstInfo);
    }

    // PEAR
    if (!suppress || false) {
        //// modelCreateInfo.pipelineType = PIPELINE_TYPE::TRI_LIST_TEX;
        //// modelCreateInfo.async = true;
        //// modelCreateInfo.material = {};
        //// modelCreateInfo.modelPath = PEAR_MODEL_PATH;
        //// modelCreateInfo.model = helpers::affine();
        //// ModelHandler::makeModel(&modelCreateInfo, SceneHandler::getActiveScene(), nullptr);
    }

    count = 5;
    // MEDIEVAL HOUSE (TRI_COLOR_TEX)
    if (!suppress || true) {
        modelInfo = {};
        modelInfo.pipelineType = PIPELINE::TRI_LIST_TEX;
        modelInfo.async = false;
        modelInfo.modelPath = MED_H_MODEL_PATH;
        modelInfo.smoothNormals = false;
        modelInfo.visualHelper = true;
        defInstInfo = {};
        defInstInfo.data.reserve(static_cast<size_t>(count) * static_cast<size_t>(count));
        for (uint32_t i = 0; i < count; i++) {
            float x = -6.5f - (static_cast<float>(i) * 6.5f);
            for (uint32_t j = 0; j < count; j++) {
                float z = -(static_cast<float>(j) * 6.5f);
                defInstInfo.data.push_back({helpers::affine(glm::vec3{0.0175f}, {x, 0.0f, z}, M_PI_4_FLT, CARDINAL_Y)});
            }
        }
        // MATERIAL
        defMatInfo = {};
        defMatInfo.pTexture = handlers_.pTexture->getTextureByName(Texture::MEDIEVAL_HOUSE_CREATE_INFO.name);
        handlers_.pModel->makeTextureModel(&modelInfo, &defMatInfo, &defInstInfo);
    }
    // MEDIEVAL HOUSE (PBR_TEX)
    if (!suppress || false) {
        modelInfo = {};
        modelInfo.pipelineType = PIPELINE::PBR_TEX;
        modelInfo.async = true;
        modelInfo.modelPath = MED_H_MODEL_PATH;
        modelInfo.smoothNormals = false;
        modelInfo.visualHelper = false;
        defInstInfo = {};
        defInstInfo.data.reserve(static_cast<size_t>(count) * static_cast<size_t>(count));
        for (uint32_t i = 0; i < count; i++) {
            float x = -6.5f - (static_cast<float>(i) * 6.5f);
            for (uint32_t j = 0; j < count; j++) {
                float z = (static_cast<float>(j) * 6.5f);
                defInstInfo.data.push_back({helpers::affine(glm::vec3{0.0175f}, {x, 0.0f, z}, M_PI_4_FLT, CARDINAL_Y)});
            }
        }
        defInstInfo.data.push_back({helpers::affine(glm::vec3{0.0175f}, {-6.5f, 0.0f, -6.5f}, M_PI_4_FLT, CARDINAL_Y)});
        // MATERIAL
        pbrMatInfo = {};
        pbrMatInfo.pTexture = handlers_.pTexture->getTextureByName(Texture::MEDIEVAL_HOUSE_CREATE_INFO.name);
        handlers_.pModel->makeTextureModel(&modelInfo, &pbrMatInfo, &defInstInfo);
    }

    // PIG
    if (!suppress || false) {
        // MODEL
        modelInfo = {};
        modelInfo.pipelineType = PIPELINE::TRI_LIST_COLOR;
        modelInfo.async = true;
        modelInfo.callback = [groundPlane_bbmm](auto pModel) { pModel->putOnTop(groundPlane_bbmm); };
        modelInfo.modelPath = PIG_MODEL_PATH;
        modelInfo.smoothNormals = true;
        defInstInfo = {};
        defInstInfo.data.push_back({helpers::affine(glm::vec3{2.0f}, {0.0f, 0.0f, -4.0f})});
        // MATERIAL
        defMatInfo = {};
        defMatInfo.flags = Material::FLAG::PER_MATERIAL_COLOR | Material::FLAG::MODE_BLINN_PHONG;
        defMatInfo.ambientCoeff = {0.8f, 0.8f, 0.8f};
        defMatInfo.color = {0.8f, 0.8f, 0.8f};
        handlers_.pModel->makeColorModel(&modelInfo, &defMatInfo, &defInstInfo);
    }
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
            auto& light = handlers_.pUniform->getDefPosLight();
            light.setFlags(helpers::incrementByteFlag(light.getFlags(), Light::FLAG::TEST_1, Light::TEST_ALL));
            handlers_.pUniform->update(light);
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
    // Images are destroyed by vkDestroySwapchainKHR
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
