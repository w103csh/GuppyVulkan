
#include "Game.h"

#include "CommandHandler.h"
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