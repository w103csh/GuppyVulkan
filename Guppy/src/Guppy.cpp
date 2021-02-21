/*
 * Modifications copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 * Changed file name from "Hologram.cpp"
 * -------------------------------
 * Copyright (C) 2016 Google, Inc.
 * Copyright (C) 2017 The Brenwill Workshop Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <algorithm>
#include <functional>
#include <queue>

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
// REMOVE BELOW???
#include "InputHandler.h"
#include "SoundHandler.h"

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
    paused_(false),
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
    auto it = std::find_first_of(RenderPass::ALL.begin(), RenderPass::ALL.end(), Compute::ALL.begin(), Compute::ALL.end());
    assert(it == RenderPass::ALL.end());

    // init_workers();
}

Guppy::~Guppy() {}

void Guppy::attachShell() {
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
    if (settings().enableDirectoryListener) {
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
}

/* This function is for updating things regardless of framerate. It is based on settings.ticks_per_second,
 *  and should be called that many times per second.
 * NOTE: add_game_time is weird and appears to limit the amount of ticks per second arbitrarily, so this
 *  in reality could do anything...
 * NOTE: Things like input should not be used here since this is not guaranteed to be called each time input
 *  is collected. This function could be called as many as... The rest of this comment disappeared.
 */
void Guppy::tick() {
    if (sim_paused_) return;
    if (paused_) return;

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

void Guppy::frame() {
    handlers_.pPass->acquireBackBuffer();
    handlers_.pUI->frame();

    processInput();

    if (paused_) return;

    // PRE-DRAW
    handlers_.pUniform->frame();  // Camera updates happen here... this seems bad.
    handlers_.pScene->frame();
    handlers_.pParticle->frame();
    // DRAW
    handlers_.pPass->recordPasses();
    // POST-DRAW
    handlers_.pShader->cleanup();
    handlers_.pPipeline->cleanup();
    handlers_.pScene->cleanup();

    // **********************
    handlers_.pPass->updateFrameIndex();
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
}

void Guppy::processInput() {
    const auto& inputInfo = shell().inputHandler().getInfo();
    const auto& playerInfo = inputInfo.players[0];

    bool cycleCameras = false;

    // KEYBOARD
    if (playerInfo.pKeys) {
        for (const auto& key : *playerInfo.pKeys) {
            switch (key) {
                case GAME_KEY::MINUS:
                case GAME_KEY::EQUALS:
                case GAME_KEY::BACKSPACE:
                case GAME_KEY::ESC:
                    // Used by the shell
                    break;
                case GAME_KEY::SPACE:
                    // sim_paused_ = !sim_paused_;
                    // paused_ = !paused_;
                    break;
                case GAME_KEY::TAB: {
                    handlers_.pUniform->cycleCamera();
                } break;
                case GAME_KEY::F: {
                } break;
                case GAME_KEY::TOP_1: {
                    handlers_.pUniform->moveToDebugCamera();
                } break;
                case GAME_KEY::TOP_2: {
                } break;
                case GAME_KEY::TOP_3: {
                    // handlers_.pUniform->createVisualHelpers();
                } break;
                case GAME_KEY::TOP_4: {
                    handlers_.pParticle->toggleDraw();
                } break;
                case GAME_KEY::TOP_5: {
                    handlers_.pParticle->togglePause();
                } break;
                case GAME_KEY::TOP_6: {
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
                case GAME_KEY::TOP_7: {
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
                    // particleHandler().startFountain(0);
                    // particleHandler().startFountain(1);
                    // particleHandler().startFountain(2);
                    // particleHandler().startFountain(3);
                    // particleHandler().startFountain(4);
                    // particleHandler().startFountain(5);
                } break;
                case GAME_KEY::TOP_8: {
                    auto& light = handlers_.pUniform->getDefPosLight();
                    light.setFlags(helpers::incrementByteFlag(light.getFlags(), Light::FLAG::TEST_1, Light::TEST_ALL));
                    handlers_.pUniform->update(light);
                } break;
                case GAME_KEY::LEFT_BRACKET: {
                    auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(0);
                    tessData.decSegs();
                    handlers_.pUniform->update(tessData);
                } break;
                case GAME_KEY::RIGHT_BRACKET: {
                    auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(0);
                    tessData.incSegs();
                    handlers_.pUniform->update(tessData);
                } break;
                case GAME_KEY::O: {
                    auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(1);
                    tessData.decInnerLevelTriangle();
                    handlers_.pUniform->update(tessData);
                } break;
                case GAME_KEY::P: {
                    auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(1);
                    tessData.incInnerLevelTriangle();
                    handlers_.pUniform->update(tessData);
                } break;
                case GAME_KEY::K: {
                    auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(1);
                    tessData.decOuterLevelTriangle();
                    handlers_.pUniform->update(tessData);
                } break;
                case GAME_KEY::L: {
                    auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(1);
                    tessData.incOuterLevelTriangle();
                    handlers_.pUniform->update(tessData);
                } break;
                case GAME_KEY::I: {
                    auto& tessData = handlers_.pUniform->uniTessDefMgr().getTypedItem(1);
                    tessData.noTessTriangle();
                    handlers_.pUniform->update(tessData);
                } break;
                default:
                    break;
            }
        }
    }

    // MOUSE
    if (true && playerInfo.pMouse) {
        if (playerInfo.pMouse->left) {
            auto ray = handlers_.pUniform->getMainCamera().getRay(playerInfo.pMouse->location, shell().context().extent);
            handlers_.pScene->getActiveScene()->select(ray);
        }
    }

    // BUTTONS
    if (false && (playerInfo.buttons & GAME_BUTTON::A)) {
        static std::queue<glm::vec3> accels(
            {{-20.0f, -10.0f, 2.0f}, {20.0f, -10.0f, -2.0f}, {2.0f, -10.0f, 20.0f}, {-2.0f, -10.0f, -20.0f}});
        static bool flip = true;

        auto pCloth = std::static_pointer_cast<UniformDynamic::Particle::Cloth::Base>(
            handlers_.pParticle->prtclClthMgr.pItems.front());
        if (flip) {
            auto a = accels.front();
            pCloth->setAcceleration(a);
            accels.pop();
            accels.push(a);
        } else {
            pCloth->setAcceleration();
        }
        flip = !flip;
    }
    if (true && (playerInfo.buttons & GAME_BUTTON::BACK)) {
        handlers_.pUniform->cycleCamera();
    }
}
