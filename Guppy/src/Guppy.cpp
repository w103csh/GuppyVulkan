
#include <algorithm>
#include <functional>

#include "ConstantsAll.h"
#include "Game.h"
#include "Guppy.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "ComputeHandler.h"
#include "DescriptorHandler.h"
#include "InputHandler.h"  // (shell)
#include "LoadingHandler.h"
#include "MaterialHandler.h"
#include "MeshHandler.h"
#include "ModelHandler.h"
#include "ParticleHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"
#include "SceneHandler.h"
#include "ShaderHandler.h"
#include "SoundHandler.h"  // (shell)
#include "TextureHandler.h"
#include "UIHandler.h"
#include "UniformHandler.h"

#ifdef USE_DEBUG_UI
#include "UIImGuiHandler.h"
#endif

// clang-format off
Guppy::Guppy(const std::vector<std::string>& args)
    : Game("Guppy", args, Game::Handlers{
        std::make_unique<Command::Handler>(this),
        std::make_unique<Compute::Handler>(this),
        std::make_unique<Descriptor::Handler>(this),
        std::make_unique<Loading::Handler>(this),
        std::make_unique<Material::Handler>(this),
        std::make_unique<Mesh::Handler>(this),
        std::make_unique<Model::Handler>(this),
        std::make_unique<Particle::Handler>(this),
        std::make_unique<Pipeline::Handler>(this),
        std::make_unique<RenderPass::Handler>(this),
        std::make_unique<Scene::Handler>(this),
        std::make_unique<Shader::Handler>(this),
        std::make_unique<Texture::Handler>(this),
#ifdef USE_DEBUG_UI
        std::make_unique<UI::ImGuiHandler>(this),
#else
        std::make_unique<UI::Handler>(this),
#endif
        std::make_unique<Uniform::Handler>(this)
    }),
    // TODO: use these or get rid of them...
    multithread_(true),
    use_push_constants_(false),
    sim_paused_(false)
    // sim_fade_(false),
    // sim_(5000),
// clang-format on
{
    // ARGS
    for (auto it = args.begin(); it != args.end(); ++it) {
        if (*it == "-s")
            multithread_ = false;
        else if (*it == "-p")
            use_push_constants_ = true;
    }

    // VALIDATION
    helpers::validatePassTypeStructures();

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
    handlers_.pMesh->init();
    handlers_.pParticle->init();
    handlers_.pCompute->init();
    handlers_.pPipeline->init();
    handlers_.pShader->init();
    handlers_.pPass->init();
    handlers_.pPipeline->initPipelines();
    handlers_.pUI->init();
    handlers_.pModel->init();
    handlers_.pScene->init();

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

    handlers_.pUniform->attachSwapchain();
    handlers_.pTexture->attachSwapchain();
    handlers_.pCompute->attachSwapchain();
    handlers_.pPass->attachSwapchain();

    // ASPECT
    float aspect = 1.0f;
    if (ctx.extent.width && ctx.extent.height)
        aspect = static_cast<float>(ctx.extent.width) / static_cast<float>(ctx.extent.height);
    handlers_.pUniform->getMainCamera().setAspect(aspect);

    // Trying to get rid of RenderDoc warnings...
    // VkFenceCreateInfo fenceInfo = {};
    // fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // VkFence fence;
    // vk::assert_success(vkCreateFence(shell().context().dev, &fenceInfo, nullptr, &fence));
    // VkSubmitInfo submitInfo = {};
    // submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    // submitInfo.commandBufferCount = 1;
    // submitInfo.pCommandBuffers = &handlers_.pCommand->graphicsCmd();
    // handlers_.pCommand->endCmd(handlers_.pCommand->graphicsCmd());
    // vk::assert_success(vkQueueSubmit(handlers_.pCommand->graphicsQueue(), 1, &submitInfo, fence));
    // vk::assert_success(vkWaitForFences(ctx.dev, 1, &fence, VK_TRUE, UINT64_MAX));
    // vkDestroyFence(ctx.dev, fence, nullptr);
    // vkResetCommandBuffer(handlers_.pCommand->graphicsCmd(), 0);
}

/* This function is for updating things regardless of framerate. It is based on settings.ticks_per_second,
 *  and should be called that many times per second.
 * NOTE: add_game_time is weird and appears to limit the amount of ticks per second arbitrarily, so this
 *  in reality could do anything...
 * NOTE: Things like input should not be used here since this is not guaranteed to be called each time input
 *  is collected. This function could be called as many as... The rest of this comment disappeared.
 */
void Guppy::onTick() {
    if (sim_paused_) return;

    // TODO: Should this be "on_frame"? every other frame? async? ... I have no clue yet.
    handlers_.pLoading->tick();
    handlers_.pTexture->tick();
    handlers_.pModel->tick();
    handlers_.pPipeline->tick();
    handlers_.pMesh->tick();
    handlers_.pParticle->tick();
    handlers_.pCompute->tick();

    // for (auto &worker : workers_) worker->update_simulation();
}

void Guppy::onFrame(float framePred) {
    // shell().log(Shell::LOG_DEBUG, std::to_string(shell().getCurrentTime()).c_str());
    handlers_.pPass->acquireBackBuffer();
    // PRE-DRAW
    handlers_.pUniform->frame();
    handlers_.pParticle->frame();
    handlers_.pUI->frame();
    // DRAW
    handlers_.pPass->recordPasses();
    // POST-DRAW
    // TODO: make a postDraw virtual thing.
    handlers_.pShader->cleanup();
    handlers_.pPipeline->cleanup(handlers_.pPass->getFrameIndex());
    handlers_.pScene->cleanup();
    // **********************
    handlers_.pPass->updateFrameIndex();
}

void Guppy::onKey(GAME_KEY key) {
    switch (key) {
        case GAME_KEY::KEY_MINUS:
        case GAME_KEY::KEY_EQUALS:
        case GAME_KEY::KEY_BACKSPACE:
            // Used by the shell.
            break;
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

            // The fade doesn't work right, and the result change from day to day.
            Sound::StartInfo startInfo = {};
            // startInfo.volume = Sound::START_ZERO_VOLUME;
            startInfo.volume = 0.5f;
            // Sound::EffectInfo effectInfo = {Sound::EFFECT::FADE, 5.0f};
            // if (!shell().soundHandler().start(Sound::TYPE::OCEAN_WAVES, &startInfo, &effectInfo)) {
            if (!shell().soundHandler().start(Sound::TYPE::OCEAN_WAVES, &startInfo, nullptr)) {
                // effectInfo = {Sound::EFFECT::FADE, 8.0f, 0.0f, Sound::PLAYBACK::STOP};
                // shell().soundHandler().addEffect(Sound::TYPE::OCEAN_WAVES, &effectInfo);
                // shell().soundHandler().pause(Sound::TYPE::OCEAN_WAVES);
                shell().soundHandler().stop(Sound::TYPE::OCEAN_WAVES);
            }

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
            handlers_.pParticle->startFountain(0);
            handlers_.pParticle->startFountain(1);
            handlers_.pParticle->startFountain(2);
            handlers_.pParticle->startFountain(3);
            handlers_.pParticle->startFountain(4);
            handlers_.pParticle->startFountain(5);
        } break;
        case GAME_KEY::KEY_8: {
            auto& light = handlers_.pUniform->getDefPosLight();
            light.setFlags(helpers::incrementByteFlag(light.getFlags(), Light::FLAG::TEST_1, Light::TEST_ALL));
            handlers_.pUniform->update(light);
        } break;
        case GAME_KEY::KEY_LEFT_BRACKET: {
            auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(0);
            tessData.decSegs();
            handlers_.pUniform->update(tessData);
        } break;
        case GAME_KEY::KEY_RIGHT_BRACKET: {
            auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(0);
            tessData.incSegs();
            handlers_.pUniform->update(tessData);
        } break;
        case GAME_KEY::KEY_O: {
            auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(1);
            tessData.decInnerLevelTriangle();
            handlers_.pUniform->update(tessData);
        } break;
        case GAME_KEY::KEY_P: {
            auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(1);
            tessData.incInnerLevelTriangle();
            handlers_.pUniform->update(tessData);
        } break;
        case GAME_KEY::KEY_K: {
            auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(1);
            tessData.decOuterLevelTriangle();
            handlers_.pUniform->update(tessData);
        } break;
        case GAME_KEY::KEY_L: {
            auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(1);
            tessData.incOuterLevelTriangle();
            handlers_.pUniform->update(tessData);
        } break;
        case GAME_KEY::KEY_I: {
            auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(1);
            tessData.noTessTriangle();
            handlers_.pUniform->update(tessData);
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
    handlers_.pTexture->detachSwapchain();
    handlers_.pCompute->detachSwapchain();
    handlers_.pPass->detachSwapchain();
    handlers_.pUI->reset();
    handlers_.pCommand->resetCmdBuffers();
}

void Guppy::detachShell() {
    // TODO: kill futures !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    if (multithread_) {
        // for (auto &worker : workers_) worker->stop();
    }

    handlers_.pScene->destroy();
    handlers_.pPipeline->destroy();
    handlers_.pShader->destroy();
    handlers_.pDescriptor->destroy();
    handlers_.pUniform->destroy();
    handlers_.pTexture->destroy();
    handlers_.pMaterial->destroy();
    handlers_.pMesh->destroy();
    handlers_.pParticle->destroy();
    handlers_.pCompute->destroy();
    handlers_.pScene->destroy();
    handlers_.pUI->destroy();
    handlers_.pPass->destroy();
    handlers_.pCommand->destroy();
    handlers_.pLoading->destroy();

    Game::detachShell();
}
