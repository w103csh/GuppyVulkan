
#include "UIHandler.h"

#include "Shell.h"

UI::Handler::Handler(Game* pGame, std::unique_ptr<RenderPass::Base>&& pPass)  //
    : Game::Handler(pGame), pPass_(std::move(pPass)) {}
