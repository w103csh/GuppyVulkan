/*
 * This came from LunarG Hologram sample, but has been radically altered at this point.
 */

#ifndef GAME_H
#define GAME_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "Enum.h"

class Shell;

// clang-format off
namespace Command       { class Handler; }
namespace Compute       { class Handler; }
namespace Descriptor    { class Handler; }
namespace Loading       { class Handler; }
namespace Material      { class Handler; }
namespace Mesh          { class Handler; }
namespace Model         { class Handler; }
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
        int initial_width;
        int initial_height;
        int queue_count;
        int back_buffer_count;
        int ticks_per_second;
        bool vsync;
        bool animate;

        bool validate;
        bool validate_verbose;

        bool no_tick;
        bool no_render;

        // *
        bool try_sampler_anisotropy;  // TODO: Not sure what this does
        bool try_sample_rate_shading;
        bool try_compute_shading;
        bool enable_sample_shading;
        bool enable_double_clicks;
        bool enable_debug_markers;
        bool enable_directory_listener;
        bool assert_on_recompile_shader;
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
    virtual void onKey(GAME_KEY key) {}               // TODO: bad design
    virtual void onMouse(const MouseInput &input){};  // TODO: bad design & misleading name

    // HANDLER
    struct Handlers {
        std::unique_ptr<Command::Handler> pCommand;
        std::unique_ptr<Compute::Handler> pCompute;
        std::unique_ptr<Descriptor::Handler> pDescriptor;
        std::unique_ptr<Loading::Handler> pLoading;
        std::unique_ptr<Material::Handler> pMaterial;
        std::unique_ptr<Mesh::Handler> pMesh;
        std::unique_ptr<Model::Handler> pModel;
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
            } else if (*it == "-dbgm") {
                settings_.enable_debug_markers = true;
            }
        }
    }
};

#endif  // GAME_H
