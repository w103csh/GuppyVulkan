
#include "UIHandler.h"
#include "Shell.h"

UIHandler UIHandler::inst_;

UIHandler::UIHandler() {}

void UIHandler::init(Shell* sh, const Game::Settings& settings, std::unique_ptr<UI> ui) {
    inst_.ui_ = std::move(ui);

    inst_.reset();
    inst_.sh_ = sh;
    inst_.ctx_ = sh->context();
    inst_.settings_ = settings;
}