/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Game.h"

#include "Helpers.h"
#include "Types.h"
// HANDLERS
#include "CommandHandler.h"
#include "ComputeHandler.h"
#include "DescriptorHandler.h"
#include "LoadingHandler.h"
#include "MaterialHandler.h"
#include "MeshHandler.h"
#include "ModelHandler.h"
#include "ParticleHandler.h"
#include "PipelineHandler.h"
#include "RenderPassHandler.h"
#include "SceneHandler.h"
#include "ShaderHandler.h"
#include "TextureHandler.h"
#include "UIHandler.h"
#include "UniformHandler.h"

Game::~Game() = default;

Game::Game(const std::string& name, const std::vector<std::string>& args, Handlers&& handlers)
    : handlers_(std::move(handlers)), settings_(), shell_(nullptr) {
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
      validate(false),
      validate_verbose(false),
      no_tick(false),
      no_render(false),
      try_sampler_anisotropy(true),  // TODO: Not sure what this does
      try_sample_rate_shading(true),
      try_compute_shading(true),
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
      try_tessellation_shading(false),
      try_geometry_shading(false),
#else
      try_tessellation_shading(true),
      try_geometry_shading(true),
#endif
      try_wireframe_shading(true),
      try_debug_markers(false),
      try_independent_blend(true),
      try_image_cube_array(true),
      enable_sample_shading(true),
      enable_double_clicks(false),
      enable_directory_listener(true),
      assert_on_recompile_shader(false) {
}