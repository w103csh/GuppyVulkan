/*
 * Modifications copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 * -------------------------------
 * Copyright (C) 2016 Google, Inc.
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

#ifndef GAME_H
#define GAME_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Enum.h"

struct BufferResource;
class Shell;
// clang-format off
namespace Command       { class Handler; }
namespace Compute       { class Handler; }
namespace Descriptor    { class Handler; }
namespace Loading       { class Handler; }
namespace Material      { class Handler; }
namespace Mesh          { class Handler; }
namespace Model         { class Handler; }
namespace Particle      { class Handler; }
namespace Pipeline      { class Handler; }
namespace RenderPass    { class Handler; }
namespace Scene         { class Handler; }
namespace Shader        { class Handler; }
namespace Storage       { class Handler; }
namespace Texture       { class Handler; }
namespace UI            { class Handler; }
namespace Uniform       { class Handler; }
// clang-format on

struct MouseInput {
    float xPos, yPos, zDelta;
    bool moving, primary, secondary;
};

class Game {
   public:
    Game(const Game &game) = delete;
    Game &operator=(const Game &game) = delete;
    virtual ~Game();

    struct Settings {
        Settings();
        std::string name;
        int initialWidth;
        int initialHeight;
        int queueCount;
        int backBufferCount;
        int ticksPerSecond;
        bool vsync;
        bool animate;

        bool validate;
        bool validateVerbose;

        bool noTick;
        bool noRender;

        // *
        bool trySamplerAnisotropy;  // TODO: Not sure what this does
        bool trySampleRateShading;
        bool tryComputeShading;
        bool tryTessellationShading;
        bool tryGeometryShading;
        bool tryWireframeShading;
        bool tryDebugMarkers;
        bool tryIndependentBlend;
        bool tryImageCubeArray;
        bool enableSampleShading;
        bool enableDoubleClicks;
        bool enableDirectoryListener;
        bool assertOnRecompileShader;
    };

    // SETTINGS
    constexpr const auto &settings() const { return settings_; }

    // SHELL
    virtual void attachShell(Shell &shell) { shell_ = &shell; }
    virtual void detachShell() { shell_ = nullptr; }
    constexpr const auto &shell() const { return *shell_; }

    // SWAPCHAIN
    virtual void attachSwapchain() = 0;
    virtual void detachSwapchain() = 0;

    // LIFECYCLE
    virtual void onTick() {}
    virtual void onFrame(float framePred) {}
    virtual void onButton(const GameButtonBits buttons) {}  // TODO: bad design
    virtual void onKey(const GAME_KEY key) {}               // TODO: bad design
    virtual void onMouse(const MouseInput &input) {}        // TODO: bad design & misleading name

    // HANDLER
    struct Handlers {
        std::unique_ptr<Command::Handler> pCommand;
        std::unique_ptr<Compute::Handler> pCompute;
        std::unique_ptr<Descriptor::Handler> pDescriptor;
        std::unique_ptr<Loading::Handler> pLoading;
        std::unique_ptr<Material::Handler> pMaterial;
        std::unique_ptr<Mesh::Handler> pMesh;
        std::unique_ptr<Model::Handler> pModel;
        std::unique_ptr<Particle::Handler> pParticle;
        std::unique_ptr<Pipeline::Handler> pPipeline;
        std::unique_ptr<RenderPass::Handler> pPass;
        std::unique_ptr<Scene::Handler> pScene;
        std::unique_ptr<Shader::Handler> pShader;
        std::unique_ptr<Texture::Handler> pTexture;
        std::unique_ptr<UI::Handler> pUI;
        std::unique_ptr<Uniform::Handler> pUniform;
    };

    class Handler {
       public:
        Handler(const Handler &) = delete;             // Prevent construction by copying
        Handler &operator=(const Handler &) = delete;  // Prevent assignment

        virtual void init() = 0;
        virtual void tick() {}
        virtual void frame() {}
        virtual void destroy() { reset(); };

        constexpr const auto &settings() const { return pGame_->settings(); }
        constexpr const auto &shell() const { return pGame_->shell(); }

        inline Command::Handler &commandHandler() const { return std::ref(*pGame_->handlers_.pCommand); }
        inline Compute::Handler &computeHandler() const { return std::ref(*pGame_->handlers_.pCompute); }
        inline Descriptor::Handler &descriptorHandler() const { return std::ref(*pGame_->handlers_.pDescriptor); }
        inline Loading::Handler &loadingHandler() const { return std::ref(*pGame_->handlers_.pLoading); }
        inline Material::Handler &materialHandler() const { return std::ref(*pGame_->handlers_.pMaterial); }
        inline Mesh::Handler &meshHandler() const { return std::ref(*pGame_->handlers_.pMesh); }
        inline Model::Handler &modelHandler() const { return std::ref(*pGame_->handlers_.pModel); }
        inline Particle::Handler &particleHandler() const { return std::ref(*pGame_->handlers_.pParticle); }
        inline Pipeline::Handler &pipelineHandler() const { return std::ref(*pGame_->handlers_.pPipeline); }
        inline RenderPass::Handler &passHandler() const { return std::ref(*pGame_->handlers_.pPass); }
        inline Scene::Handler &sceneHandler() const { return std::ref(*pGame_->handlers_.pScene); }
        inline Shader::Handler &shaderHandler() const { return std::ref(*pGame_->handlers_.pShader); }
        inline Texture::Handler &textureHandler() const { return std::ref(*pGame_->handlers_.pTexture); }
        inline UI::Handler &uiHandler() const { return std::ref(*pGame_->handlers_.pUI); }
        inline Uniform::Handler &uniformHandler() const { return std::ref(*pGame_->handlers_.pUniform); }

       protected:
        Handler(Game *pGame) : pGame_(pGame) {}
        virtual ~Handler() = default;

        virtual void reset() = 0;

        Game *pGame_;
    };

   protected:
    Game(const std::string &name, const std::vector<std::string> &args, Handlers &&handlers);

    // LISTENERS
    void watchDirectory(const std::string &directory, std::function<void(std::string)> callback);

    // HANDLERS
    const Handlers handlers_;

   private:
    Settings settings_;
    Shell *shell_;

    void parse_args(const std::vector<std::string> &args) {
        for (auto it = args.begin(); it != args.end(); ++it) {
            if (*it == "-b") {
                settings_.vsync = false;
            } else if (*it == "-w") {
                ++it;
                settings_.initialWidth = std::stoi(*it);
            } else if (*it == "-h") {
                ++it;
                settings_.initialHeight = std::stoi(*it);
            } else if ((*it == "-v") || (*it == "--validate")) {
                settings_.validate = true;
            } else if (*it == "-vv") {
                settings_.validate = true;
                settings_.validateVerbose = true;
            } else if (*it == "-nt") {
                settings_.noTick = true;
            } else if (*it == "-nr") {
                settings_.noRender = true;
            } else if (*it == "-dbgm") {
                settings_.tryDebugMarkers = true;
            }
        }
    }
};

#endif  // GAME_H
