/*
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

class Shell;

// clang-format off
namespace Command       { class Handler; }
namespace Descriptor    { class Handler; }
namespace Loading       { class Handler; }
namespace Material      { class Handler; }
namespace Mesh          { class Handler; }
namespace Model         { class Handler; }
namespace Pipeline      { class Handler; }
namespace Scene         { class Handler; }
namespace Shader        { class Handler; }
namespace Texture       { class Handler; }
namespace UI            { class Handler; }
namespace Uniform       { class Handler; }
// clang-format on

enum class GAME_KEY {
    // virtual keys
    KEY_SHUTDOWN,
    // physical keys
    KEY_UNKNOWN,
    KEY_ESC,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_SPACE,
    KEY_TAB,
    KEY_F,
    KEY_W,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_E,
    KEY_Q,
    // number keys
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_0,
    KEY_MINUS,
    KEY_EQUAL,
    // function keys
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    // modifiers
    KEY_ALT,
    KEY_CTRL,
    KEY_SHFT,
};

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
        std::string name;
        int initial_width = 1920;
        int initial_height = 1080;
        int queue_count = 1;
        int back_buffer_count = 3;
        int ticks_per_second = 30;
        bool vsync = true;
        bool animate = true;

        bool validate = false;
        bool validate_verbose = false;

        bool no_tick = false;
        bool no_render = false;
        bool no_present = false;

        // *
        bool try_sampler_anisotropy = true;  // TODO: Not sure what this does
        bool try_sample_rate_shading = true;
        bool enable_sample_shading = true;
        bool include_color = true;
        bool include_depth = true;
        bool enable_double_clicks = false;
        bool enable_debug_markers = false;
        bool enable_directory_listener = true;
        bool assert_on_recompile_shader = false;
    };

    // SETTINGS
    inline const Settings &settings() const { return settings_; }

    // SHELL
    virtual void attachShell(Shell &shell) { shell_ = &shell; }
    virtual void detachShell() { shell_ = nullptr; }
    inline const Shell &shell() const { return std::ref(*shell_); }

    // SWAPCHAIN
    virtual void attachSwapchain() = 0;
    virtual void detachSwapchain() = 0;

    // LIFECYCLE
    virtual void onTick() {}
    virtual void onFrame(float framePred) {}
    virtual void onKey(GAME_KEY key) {}               // TODO: bad design
    virtual void onMouse(const MouseInput &input){};  // TODO: bad design & misleading name

    // HANDLER
    struct Handlers {
        std::unique_ptr<Command::Handler> pCommand = nullptr;
        std::unique_ptr<Descriptor::Handler> pDescriptor = nullptr;
        std::unique_ptr<Loading::Handler> pLoading = nullptr;
        std::unique_ptr<Material::Handler> pMaterial = nullptr;
        std::unique_ptr<Mesh::Handler> pMesh = nullptr;
        std::unique_ptr<Model::Handler> pModel = nullptr;
        std::unique_ptr<Pipeline::Handler> pPipeline = nullptr;
        std::unique_ptr<Scene::Handler> pScene = nullptr;
        std::unique_ptr<Shader::Handler> pShader = nullptr;
        std::unique_ptr<Texture::Handler> pTexture = nullptr;
        std::unique_ptr<UI::Handler> pUI = nullptr;
        std::unique_ptr<Uniform::Handler> pUniform = nullptr;
    };

    class Handler {
       public:
        Handler(const Handler &) = delete;             // Prevent construction by copying
        Handler &operator=(const Handler &) = delete;  // Prevent assignment

        virtual void init() = 0;
        virtual void destroy() { reset(); };

        inline const Settings &settings() const { return pGame_->settings(); }
        inline const Shell &shell() const { return pGame_->shell(); }

        inline Command::Handler &commandHandler() const { return std::ref(*pGame_->handlers_.pCommand); }
        inline Descriptor::Handler &descriptorHandler() const { return std::ref(*pGame_->handlers_.pDescriptor); }
        inline Loading::Handler &loadingHandler() const { return std::ref(*pGame_->handlers_.pLoading); }
        inline Material::Handler &materialHandler() const { return std::ref(*pGame_->handlers_.pMaterial); }
        inline Mesh::Handler &meshHandler() const { return std::ref(*pGame_->handlers_.pMesh); }
        inline Model::Handler &modelHandler() const { return std::ref(*pGame_->handlers_.pModel); }
        inline Pipeline::Handler &pipelineHandler() const { return std::ref(*pGame_->handlers_.pPipeline); }
        inline Scene::Handler &sceneHandler() const { return std::ref(*pGame_->handlers_.pScene); }
        inline Shader::Handler &shaderHandler() const { return std::ref(*pGame_->handlers_.pShader); }
        inline Texture::Handler &textureHandler() const { return std::ref(*pGame_->handlers_.pTexture); }
        inline UI::Handler &uiHandler() const { return std::ref(*pGame_->handlers_.pUI); }
        inline Uniform::Handler &uniformHandler() const { return std::ref(*pGame_->handlers_.pUniform); }

       protected:
        Handler(Game *pGame) : pGame_(pGame) {}

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
                settings_.initial_width = std::stoi(*it);
            } else if (*it == "-h") {
                ++it;
                settings_.initial_height = std::stoi(*it);
            } else if ((*it == "-v") || (*it == "--validate")) {
                settings_.validate = true;
            } else if (*it == "-vv") {
                settings_.validate = true;
                settings_.validate_verbose = true;
            } else if (*it == "-nt") {
                settings_.no_tick = true;
            } else if (*it == "-nr") {
                settings_.no_render = true;
            } else if (*it == "-np") {
                settings_.no_present = true;
            } else if (*it == "-dbgm") {
                settings_.enable_debug_markers = true;
            }
        }
    }
};

#endif  // GAME_H
