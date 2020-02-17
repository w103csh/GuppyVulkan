/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "InputHandler.h"

#include <sstream>

#include "Constants.h"

/* All of this old input code needs an update (everything other than XInput). I wrote it when I
 *  barely knew C++. I am going to hold off on any major abstractions of the input until I have
 *  some kind of target for how interacting with the 3D scene is going to happen.
 */

#define DEBUG_OLD 0
#if DEBUG_OLD
#include <sstream>
#endif  // DEBUG_OLD

namespace {

// move
constexpr float K_X_MOVE_FACT = 2.0f;
constexpr float K_Y_MOVE_FACT = 2.0f;
constexpr float K_Z_MOVE_FACT = 2.0f;
// look
constexpr float M_X_LOOK_FACT = 0.3f;
constexpr float M_Y_LOOK_FACT = -0.3f;

}  // namespace

Input::Handler::Handler(Shell* pShell)
    : Shell::Handler(pShell),
      currKeyInput_(),
      posDir_(),
      isLooking_(false),
      hasFocus_(false),
      currMouseInput_{0.0f, 0.0f, 0.0f},
      prevMouseInput_{0.0f, 0.0f, 0.0f},
      lookDir_() {}

void Input::Handler::updateInput(float elapsed) {
    // reset();

    updateKeyInput();
    updateMouseInput();

    // account for time
    posDir_ *= elapsed;
    // lookDir_ *= (0.001 / elapsed);

#if DEBUG_OLD
    if (glm::any(glm::notEqual(posDir_, glm::vec3(0.0f)))) {
        std::stringstream ss;
        ss << "move (" << elapsed << "): " << helpers::makeVecString(posDir_) << std::endl;
        pShell_->log(Shell::LogPriority::LOG_INFO, ss.str().c_str());
    }
#endif  // DEBUG_OLD
}

void Input::Handler::updateKeyInput() {
#if DEBUG_OLD
    std::stringstream ss;
#endif  // DEBUG_OLD

    for (auto& key : currKeyInput_) {
        switch (key) {
                // FORWARD
            case GAME_KEY::KEY_UP:
            case GAME_KEY::KEY_W: {
#if DEBUG_OLD
                ss << " FORWARD ";
#endif  // DEBUG_OLD
                posDir_.z += K_Z_MOVE_FACT;
            } break;
                // BACK
            case GAME_KEY::KEY_DOWN:
            case GAME_KEY::KEY_S: {
#if DEBUG_OLD
                ss << " BACK ";
#endif  // DEBUG_OLD
                posDir_.z += K_Z_MOVE_FACT * -1;
            } break;
                // RIGHT
            case GAME_KEY::KEY_RIGHT:
            case GAME_KEY::KEY_D: {
#if DEBUG_OLD
                ss << " RIGHT ";
#endif  // DEBUG_OLD
                posDir_.x += K_X_MOVE_FACT;
            } break;
                // LEFT
            case GAME_KEY::KEY_LEFT:
            case GAME_KEY::KEY_A: {
#if DEBUG_OLD
                ss << " LEFT ";
#endif  // DEBUG_OLD
                posDir_.x += K_X_MOVE_FACT * -1;
            } break;
                // UP
            case GAME_KEY::KEY_E: {
#if DEBUG_OLD
                ss << " UP ";
#endif  // DEBUG_OLD
                posDir_.y += K_Y_MOVE_FACT;
            } break;
                // DOWN
            case GAME_KEY::KEY_Q: {
#if DEBUG_OLD
                ss << " DOWN ";
#endif  // DEBUG_OLD
                posDir_.y += K_Y_MOVE_FACT * -1;
            } break;
            default:;
        }
    }

#if DEBUG_OLD
    auto sMsg = ss.str();
    if (sMsg.size() > 0) {
        sMsg += "\n Y position direction: " + helpers::makeVecString(posDir_) + "\n";
        pShell_->log(Shell::LogPriority::LOG_INFO, sMsg.c_str());
    }
#endif  // DEBUG_OLD
}

void Input::Handler::updateMouseInput() {
    std::stringstream ss;

    if (isLooking_) {
        lookDir_.x = (currMouseInput_.xPos - prevMouseInput_.xPos) * M_X_LOOK_FACT;
        lookDir_.y = (currMouseInput_.yPos - prevMouseInput_.yPos) * M_Y_LOOK_FACT;

#if DEBUG_OLD
        if (!helpers::almost_equal(lookDir_.x, 0.0f, 1) || !helpers::almost_equal(lookDir_.y, 0.0f, 1)) {
            ss << "look direction: " << helpers::makeVecString(lookDir_) << std::endl;
        }
#endif  // DEBUG_OLD
    }
    if (currMouseInput_.zDelta) {
        posDir_.z += currMouseInput_.zDelta > 0 ? K_Z_MOVE_FACT * 6 : K_Z_MOVE_FACT * -6;
        currMouseInput_.zDelta = 0;  // reset zDelta here... this stuff is not great

#if DEBUG_OLD
        ss << "mouse wheel: " << helpers::makeVecString(posDir_) << std::endl;
#endif  // DEBUG_OLD
    }

#if DEBUG_OLD
    auto sMsg = ss.str();
    if (sMsg.size()) pShell_->log(Shell::LogPriority::LOG_INFO, sMsg.c_str());
#endif  // DEBUG_OLD

    prevMouseInput_ = currMouseInput_;
    isLooking_ = false;
}

void Input::Handler::reset() {
    lookDir_ = {};
    posDir_ = {};
    currMouseInput_.moving = false;
}

// NEW SHIT

#ifdef USE_XINPUT
#include "XInputHelper.h"
#define DEBUG_XINPUT 0
#if DEBUG_XINPUT
#include <sstream>
#endif  // DEBUG_XINPUT
#endif  // USE_XINPUT

void Input::Handler::update(const double elapsed) {
#ifdef USE_XINPUT
    XInput::PollControllers(pShell_->getCurrentTime<float>());
    if (XInput::PLAYER_0 != XInput::BAD_PLAYER) {
        const auto& cntlr = XInput::CNTLRS[XInput::PLAYER_0];
#if DEBUG_XINPUT
        if (XInput::CNTLRS[XInput::PLAYER_0].hasChanges()) {
            std::stringstream ss;
            ss << "THUMB left: " << helpers::makeVecString(cntlr.thumbLNorm) << " " << glm::length(cntlr.thumbLNorm)
               << " right: " << helpers::makeVecString(cntlr.thumbRNorm) << " " << glm::length(cntlr.thumbRNorm);
            pShell_->log(Shell::LogPriority::LOG_DEBUG, ss.str().c_str());
        }
#endif  // DEBUG_XINPUT
        if (cntlr.getCurrState().Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
            posDir_ += CARDINAL_Y * static_cast<float>(elapsed) * 1.5f;
        }
        if (cntlr.getCurrState().Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) {
            posDir_ += -CARDINAL_Y * static_cast<float>(elapsed) * 1.5f;
        }
        lookDir_ += glm::vec3{cntlr.thumbRNorm.x, cntlr.thumbRNorm.y, 0.0f} * static_cast<float>(elapsed) * 8.0f;
        posDir_ += glm::vec3{cntlr.thumbLNorm.x, 0.0f, cntlr.thumbLNorm.y} * static_cast<float>(elapsed) * 2.5f;
        if (cntlr.hasChanges() && cntlr.getPrevState().Gamepad.wButtons)
            pShell_->onButton(cntlr.getCurrState().Gamepad.wButtons);
    }
#endif  // USE_XINPUT
}