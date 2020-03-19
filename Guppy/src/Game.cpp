/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Game.h"

#include <Common/Helpers.h>
#include <Common/Types.h>

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
      initialWidth(1920),
      initialHeight(1080),
      queueCount(1),
      backBufferCount(3),
      ticksPerSecond(30),
      vsync(true),
      animate(true),
      validate(true),
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
      validateVerbose(true),
#else
      validateVerbose(false),
#endif
      noTick(false),
      noRender(false),
      trySamplerAnisotropy(true),  // TODO: Not sure what this does
      trySampleRateShading(true),
      tryComputeShading(true),
#if (defined(VK_USE_PLATFORM_IOS_MVK) || defined(VK_USE_PLATFORM_MACOS_MVK))
      tryTessellationShading(false),
      tryGeometryShading(false),
#else
      tryTessellationShading(true),
      tryGeometryShading(true),
#endif
      tryWireframeShading(true),
      tryDebugMarkers(false),
      tryIndependentBlend(true),
      tryImageCubeArray(true),
      enableSampleShading(true),
      enableDoubleClicks(false),
      enableDirectoryListener(true),
      assertOnRecompileShader(false) {
}