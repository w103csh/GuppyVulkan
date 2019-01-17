
#include "UIHandler.h"

UIHandler UIHandler::inst_;

void UIHandler::init(Shell* sh, const Game::Settings& settings, std::shared_ptr<UI> pUI) {
    inst_.pUI_ = pUI == nullptr ? std::make_shared<DefaultUI>() : pUI;

    inst_.reset();
    inst_.sh_ = sh;
    inst_.ctx_ = sh->context();
    inst_.settings_ = settings;
}
