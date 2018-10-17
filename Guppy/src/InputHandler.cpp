
#include <glm/glm.hpp>
#include <sstream>

#include "InputHandler.h"

// TDOD: these are probably platform specific
constexpr float K_X_MOVE_FACT = 0.01f;
constexpr float K_Y_MOVE_FACT = 0.01f;
constexpr float K_Z_MOVE_FACT = 0.01f;
constexpr float M_X_LOOK_FACT = 0.01f;
constexpr float M_Y_LOOK_FACT = -0.01f;

InputHandler::InputHandler(MyShell* sh) : sh_(sh) { assert(sh != nullptr); }

void InputHandler::updateInput() {
    reset();
    updateKeyInput();
    updateMouseInput();
}

void InputHandler::updateKeyInput() {
    std::stringstream ss;

    for (auto& key : curr_key_input_) {
        switch (key) {
                // FORWARD
            case Game::KEY::KEY_UP:
            case Game::KEY::KEY_W: {
                if (MY_DEBUG) ss << " FORWARD ";
                pos_dir_.z += K_Z_MOVE_FACT;
            } break;
                // BACK
            case Game::KEY::KEY_DOWN:
            case Game::KEY::KEY_S: {
                if (MY_DEBUG) ss << " BACK ";
                pos_dir_.z += K_Z_MOVE_FACT * -1;
            } break;
                // RIGHT
            case Game::KEY::KEY_RIGHT:
            case Game::KEY::KEY_D: {
                if (MY_DEBUG) ss << " RIGHT ";
                pos_dir_.x += K_X_MOVE_FACT;
            } break;
                // LEFT
            case Game::KEY::KEY_LEFT:
            case Game::KEY::KEY_A: {
                if (MY_DEBUG) ss << " LEFT ";
                pos_dir_.x += K_X_MOVE_FACT * -1;
            } break;
                // UP
            case Game::KEY::KEY_E: {
                if (MY_DEBUG) ss << " UP ";
                pos_dir_.y += K_Y_MOVE_FACT;
            } break;
                // DOWN
            case Game::KEY::KEY_Q: {
                if (MY_DEBUG) ss << " DOWN ";
                pos_dir_.y += K_Y_MOVE_FACT * -1;
            } break;
        }
    }

    auto msg = ss.str();
    if (MY_DEBUG && msg.size() > 0) {
        ss << "\ninput direction: (" << pos_dir_.x << ", " << pos_dir_.y << ", " << pos_dir_.z << ")" << std::endl;
        sh_->log(MyShell::LOG_INFO, ss.str().c_str());
    }
}

void InputHandler::updateMouseInput() {
    std::stringstream ss;

    if (is_looking_) {
        look_dir_.x = (curr_mouse_input_.xPos - prev_mouse_input_.xPos) * M_X_LOOK_FACT;
        look_dir_.y = (curr_mouse_input_.yPos - prev_mouse_input_.yPos) * M_Y_LOOK_FACT;

        if (MY_DEBUG && (!helpers::almost_equal(look_dir_.x, 0.0f, 1) || !helpers::almost_equal(look_dir_.y, 0.0f, 1))) {
            ss << "( " << look_dir_.x << ", " << look_dir_.y << ")";
            auto str = ss.str();
            sh_->log(MyShell::LOG_INFO, str.c_str());
        }
    }

    prev_mouse_input_ = curr_mouse_input_;
    is_looking_ = false;
}

void InputHandler::reset() {
    look_dir_ = {};
    pos_dir_ = {};
}