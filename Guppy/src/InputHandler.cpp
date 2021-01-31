/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "InputHandler.h"

#include <algorithm>
#include <string>

#include "Constants.h"
#ifdef USE_XINPUT
#include "XInputHelper.h"
#define DEBUG_XINPUT 0
#endif  // USE_XINPUT

namespace {
constexpr float MOVE_SENSITIVITY = 0.25f;
// These could be UI settings...
constexpr float LOOK_SENSITIVITY = 0.05f;
constexpr float MOUSE_LOOK_SENSITIVITY = LOOK_SENSITIVITY * 12.5f;
}  // namespace

bool Input::KeyboardManager::update(const uint8_t player) {
    const auto normFactor = handler().shell().getNormalizedElaspedTime<float>();
    auto& playerInfo = handler().info_.players[player];

    bool hasInput = false;

    prepareKeys();

    playerInfo.moveDir = {};
    for (auto& key : keyDown_) {  // keyDown_ holds movement keys only after processInput.
        switch (key) {
            // FORWARD
            case GAME_KEY::UP:
            case GAME_KEY::W: {
                hasInput = true;
                playerInfo.moveDir.z--;  // right-handed
            } break;
            // BACK
            case GAME_KEY::DOWN:
            case GAME_KEY::S: {
                hasInput = true;
                playerInfo.moveDir.z++;  // right-handed
            } break;
            // RIGHT
            case GAME_KEY::RIGHT:
            case GAME_KEY::D: {
                hasInput = true;
                playerInfo.moveDir.x++;
            } break;
            // LEFT
            case GAME_KEY::LEFT:
            case GAME_KEY::A: {
                hasInput = true;
                playerInfo.moveDir.x--;
            } break;
            // UP
            case GAME_KEY::E: {
                hasInput = true;
                playerInfo.moveDir.y++;
            } break;
            // DOWN
            case GAME_KEY::Q: {
                hasInput = true;
                playerInfo.moveDir.y--;
            } break;
            default:
                assert(false);
        }
    }

    playerInfo.moveDir *= MOVE_SENSITIVITY * normFactor;
    playerInfo.pKeys = &keys_;

    // Leave currentKeys_ alone. The shell handles currentKeys_ cleanup. If there were ever more than one "players" this
    // wouldn't work.
    static_assert(NUM_PLAYERS == 1, "Keyboard input needs to be re-examined.");

    return hasInput;
}

void Input::KeyboardManager::prepareKeys() {
    // keys_ should exclusively hold keys that were released prior to this frame update. This means that keyboard input only
    // registers on key release (the keys_ vector has the only keys exposed to the Game). Movement keys are special cased
    // because they can be held down. Other keys could be special cased if necessary but I was lazy at the end of the first
    // input refactor, and didn't want to deal with it.
    keys_.clear();
    for (const auto& keyDown : keyUp_) keys_.push_back(keyDown);
    for (auto it = keyDown_.begin(); it != keyDown_.end();) {
        if (isMoveKey(*it) && (std::find(keyUp_.begin(), keyUp_.end(), *it) == keyUp_.end())) {
            it++;
        } else {
            it = keyDown_.erase(it);
        }
    }
    keyUp_.clear();
}

bool Input::MouseManager::update(const uint8_t player) {
    const auto normFactor = handler().shell().getNormalizedElaspedTime<float>();
    auto& playerInfo = handler().info_.players[player];

    bool hasInput = false;

    if (info.right) {
        hasInput = true;
        const auto& nssTransform = handler().shell().context().normalizedScreenSpace;
        playerInfo.lookDir = nssTransform * glm::vec3((info.location - previousLocation_), 0.0f);
        playerInfo.lookDir *= MOUSE_LOOK_SENSITIVITY;
        switch (lookType_) {
            case LOOK_TYPE::GRIP_CLICK:
                playerInfo.lookDir *= -1.0f;
                break;
            default:;
        }
    }
    // Use mouse wheel to move position.
    // Note: I am just going to ignore how much scrolling is happening because this will most likely only be debugging input.
    if (info.wheel) {
        hasInput = true;
        playerInfo.moveDir.z = (info.wheel > 0.0f) ? -1.0f : 1.0f;  // right-handed
        playerInfo.moveDir.z *= MOVE_SENSITIVITY * normFactor;
    }

    playerInfo.pMouse = &info;

    previousLocation_ = info.location;
    info.wheel = 0.0f;

    return hasInput;
}

void Input::ControllerManager::poll() {
#ifdef USE_XINPUT
    XInput::PollControllers(handler().shell().getCurrentTime<float>());
#endif
}

bool Input::ControllerManager::update(const uint8_t player) {
    bool hasInput = false;
#ifdef USE_XINPUT
    hasInput = updateXInput(player);
#endif
    return hasInput;
}

#ifdef USE_XINPUT
bool Input::ControllerManager::updateXInput(const uint8_t player) {
    constexpr float XINPUT_LOOK_SENSITIVITY = LOOK_SENSITIVITY;

    bool hasInput = false;

    uint8_t xInputPlayer = XInput::BAD_PLAYER;
    switch (player) {
        case 0:
            xInputPlayer = XInput::PLAYER_0;
        default:
            assert("Input::ControllerManager::updateXInput can only handle player 0.");
    }

    if (xInputPlayer != XInput::BAD_PLAYER) {
        const auto& cntlr = XInput::CNTLRS[xInputPlayer];

        if (XInput::CNTLRS[XInput::PLAYER_0].hasChanges() || !hadZeroState_) {
            const auto normFactor = handler().shell().getNormalizedElaspedTime<float>();
            auto& playerInfo = handler().info_.players[player];

            hasInput = true;
            hadZeroState_ = false;

#if DEBUG_XINPUT
            std::string msg =
                "THUMB left: " + helpers::toString(cntlr.thumbLNorm) + " " + std::to_string(glm::length(cntlr.thumbLNorm)) +
                " right: " + helpers::toString(cntlr.thumbRNorm) + " " + std::to_string(glm::length(cntlr.thumbRNorm));
            handler().shell().log(Shell::LogPriority::LOG_DEBUG, msg.c_str());
#endif  // DEBUG_XINPUT

            if (cntlr.getCurrState().Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) {
                playerInfo.moveDir.y++;
            }
            if (cntlr.getCurrState().Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) {
                playerInfo.moveDir.y--;
            }

            playerInfo.moveDir.x = cntlr.thumbLNorm.x;
            playerInfo.moveDir.z = -cntlr.thumbLNorm.y;  // right-handed
            playerInfo.moveDir *= MOVE_SENSITIVITY * normFactor;

            playerInfo.lookDir = cntlr.thumbRNorm;
            playerInfo.lookDir *= XINPUT_LOOK_SENSITIVITY * normFactor;

            if (cntlr.hasChanges() && cntlr.getPrevState().Gamepad.wButtons)
                playerInfo.buttons = cntlr.getCurrState().Gamepad.wButtons;
        }

        /* This accounts for an XInput scenario where controller input is held constant. When this happens, no packet is sent
         * so hasChanges() will return false, but we still need to account for the held constant input (e.g., holding right
         * on a thumb stick very still). I was trying to implement a similar pattern for overwriting only new incoming input
         * across all devices during this refactor, but I was not willing to put in the effort to make it work right yet. It
         * should be much closer now, but this workaround should not be necessary if a good solution to the problem is found.
         *
         * I guess I should also mention that this only matters because I want the game to just pick up whichever input is
         * currently turned on and being used, which makes things more complex.
         */
        hadZeroState_ = glm::all(glm::equal(cntlr.thumbLNorm, glm::vec2())) &&  //
                        glm::all(glm::equal(cntlr.thumbRNorm, glm::vec2())) &&  //
                        (cntlr.getCurrState().Gamepad.wButtons == 0);
    }

    return hasInput;
}
#endif  // USE_XINPUT

Input::Handler::Handler(Shell* pShell)
    : Shell::Handler(pShell),  //
      keyboardMgr_(*this),
      mouseMgr_(*this),
      cntlrMgr_(*this),
      info_() {}

// After this function all the state we need to know from the input should be set/calculated, and ready for per frame
// decisions.
void Input::Handler::update() {
    cntlrMgr().poll();

    // There is only one "player" currently.
    if (!cntlrMgr().update(0)) {
        keyboardMgr().update(0);
        mouseMgr().update(0);
    }
}