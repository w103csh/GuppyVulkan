
#include "Game.h"

#include "CommandHandler.h"
#include "ComputeHandler.h"
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "MaterialHandler.h"
#include "MeshHandler.h"
#include "ModelHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"
#include "SceneHandler.h"
#include "ShaderHandler.h"
#include "TextureHandler.h"
#include "UIHandler.h"
#include "UniformHandler.h"

Game::~Game() = default;

Game::Game(const std::string& name, const std::vector<std::string>& args, Handlers&& handlers)
    : handlers_(std::forward<Handlers>(handlers)), settings_(), shell_(nullptr) {
    settings_.name = name;
    parse_args(args);
}

void Game::watchDirectory(const std::string& directory, std::function<void(std::string)> callback) {
    shell_->watchDirectory(directory, callback);
}

Game::Settings::Settings()
    : name(""),
      initial_width(1920),
      initial_height(1080),
      queue_count(1),
      back_buffer_count(3),
      ticks_per_second(30),
      vsync(true),
      animate(true),
      validate(true),
      validate_verbose(false),
      no_tick(false),
      no_render(false),
      try_sampler_anisotropy(true),  // TODO: Not sure what this does
      try_sample_rate_shading(true),
      try_compute_shading(true),
      try_tessellation_shading(true),
      try_geometry_shading(false),
      try_wireframe_shading(false),
      enable_sample_shading(true),
      enable_double_clicks(false),
      enable_debug_markers(false),
      enable_directory_listener(true),
      assert_on_recompile_shader(false) {}
