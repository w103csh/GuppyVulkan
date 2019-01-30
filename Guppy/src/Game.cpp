
#include "Game.h"

#include "CommandHandler.h"
#include "LoadingHandler.h"
#include "MaterialHandler.h"
#include "ModelHandler.h"
#include "PipelineHandler.h"
#include "SceneHandler.h"
#include "ShaderHandler.h"
#include "TextureHandler.h"
#include "UIHandler.h"

Game::~Game() = default;

Game::Game(const std::string& name, const std::vector<std::string>& args,
           std::unique_ptr<Command::Handler>&& pCommandHandler,    //
           std::unique_ptr<Loading::Handler>&& pLoadingHandler,    //
           std::unique_ptr<Material::Handler>&& pMaterialHandler,  //
           std::unique_ptr<Model::Handler>&& pModelHandler,        //
           std::unique_ptr<Pipeline::Handler>&& pPipelineHandler,  //
           std::unique_ptr<Scene::Handler>&& pSceneHandler,        //
           std::unique_ptr<Shader::Handler>&& pShaderHandler,      //
           std::unique_ptr<Texture::Handler>&& pTextureHandler,    //
           std::unique_ptr<UI::Handler>&& pUIHandler               //
           )
    :  //
      shell_(nullptr),
      settings_(),
      pCommandHandler_(std::move(pCommandHandler)),
      pLoadingHandler_(std::move(pLoadingHandler)),
      pMaterialHandler_(std::move(pMaterialHandler)),
      pModelHandler_(std::move(pModelHandler)),
      pPipelineHandler_(std::move(pPipelineHandler)),
      pSceneHandler_(std::move(pSceneHandler)),
      pShaderHandler_(std::move(pShaderHandler)),
      pTextureHandler_(std::move(pTextureHandler)),
      pUIHandler_(std::move(pUIHandler))
//
{
    settings_.name = name;
    parse_args(args);
}

void Game::watchDirectory(const std::string& directory, std::function<void(std::string)> callback) {
    shell_->watchDirectory(directory, callback);
}