/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifdef USE_XINPUT
#ifndef XINPUT_HELPER_H
#define XINPUT_HELPER_H

#include <array>
#include <glm/glm.hpp>
#include <XInput.h>

#include "Helpers.h"

namespace Input {
namespace XInput {

constexpr SHORT THUMB_MAX = 32767;  // SHRT_MAX
constexpr BYTE TRIGGER_MAX = 255;   // UINT8_MAX

static float POLL_CNTLR_INTRVL = 2.0f;
static float POLL_CNTLR_LAST_CHECK = -1.0f;

static float THUMB_L_DEADZONE = XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE;
static float THUMB_R_DEADZONE = XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE;
static float TRIGGER_DEADZONE = XINPUT_GAMEPAD_TRIGGER_THRESHOLD;

constexpr uint8_t BAD_PLAYER = UINT8_MAX;
static uint8_t PLAYER_0 = BAD_PLAYER;

inline glm::vec2 normalizeThumb(float LX, float LY, const float deadzone) {
    // determine how far the controller is pushed
    float magnitude = sqrt(LX * LX + LY * LY);

    // check if the controller is outside a circular dead zone
    if (magnitude > deadzone) {
        glm::vec2 v(LX / magnitude, LY / magnitude);

        // clip the magnitude at its expected maximum value
        if (magnitude > THUMB_MAX) magnitude = THUMB_MAX;

        // adjust magnitude relative to the end of the dead zone
        magnitude -= deadzone;

        // optionally normalize the magnitude with respect to its expected range
        // giving a magnitude value of 0.0 to 1.0
        float normalizedMagnitude = magnitude / (THUMB_MAX - deadzone);

        return v * normalizedMagnitude;
    } else {
        return {0.0f, 0.0f};
    }
}

constexpr float normalizeTrigger(float magnitude, const float deadzone) {
    float normalizedMagnitude = 0.0f;
    if (magnitude > deadzone) {
        magnitude -= deadzone;
        normalizedMagnitude = magnitude / (TRIGGER_MAX - deadzone);
    }
    return normalizedMagnitude;
}

struct Controller {
    bool active = false;
    uint8_t currIndex = 0;
    std::array<XINPUT_STATE, 2> states = {};
    glm::vec2 thumbLNorm{0.0f};
    glm::vec2 thumbRNorm{0.0f};
    float thumbLTrig = 0.0f;
    float thumbRTrig = 0.0f;

    constexpr auto& getCurrState() { return states[currIndex]; }
    constexpr const auto& getCurrState() const { return states[currIndex]; }
    constexpr auto& getPrevState() { return states[currIndex % 2]; }
    constexpr const auto& getPrevState() const { return states[currIndex % 2]; }
    constexpr bool hasChanges() const { return states[0].dwPacketNumber != states[1].dwPacketNumber; }

    void normalizeInput() {
        XINPUT_STATE& state = getCurrState();
        thumbLNorm = normalizeThumb(static_cast<float>(state.Gamepad.sThumbLX), static_cast<float>(state.Gamepad.sThumbLY),
                                    THUMB_L_DEADZONE);
        thumbRNorm = normalizeThumb(static_cast<float>(state.Gamepad.sThumbRX), static_cast<float>(state.Gamepad.sThumbRY),
                                    THUMB_R_DEADZONE);
        thumbLTrig = normalizeTrigger(static_cast<float>(state.Gamepad.bLeftTrigger), TRIGGER_DEADZONE);
        thumbRTrig = normalizeTrigger(static_cast<float>(state.Gamepad.bLeftTrigger), TRIGGER_DEADZONE);
    }

    void update() {
        if (!hasChanges()) return;
        normalizeInput();
        currIndex = (currIndex + 1) % 2;
    }
};

static std::array<Controller, XUSER_MAX_COUNT> CNTLRS = {};

void PollControllers(const float t) {
    DWORD dwResult;
    // Poll all possible controllers on a regular interval.
    if (helpers::checkInterval(t, POLL_CNTLR_INTRVL, POLL_CNTLR_LAST_CHECK)) {
        for (uint8_t i = 0; i < XUSER_MAX_COUNT; i++) {
            // Simply get the state of the controller from XInput.
            dwResult = XInputGetState(i, &CNTLRS[i].getCurrState());
            if (dwResult == ERROR_SUCCESS) {
                CNTLRS[i].active = true;
                if (PLAYER_0 == BAD_PLAYER) PLAYER_0 = i;
                CNTLRS[i].update();
            } else if (CNTLRS[i].active) {
                CNTLRS[i].active = false;
                if (PLAYER_0 == i) {
                    PLAYER_0 = BAD_PLAYER;
                    // Assign PLAYER_0 to any active controller. TODO: timestamp? (This code wasn't tested)
                    for (int16_t j = i - 1, k = i + 1; j >= 0 || k < XUSER_MAX_COUNT; j--, k++) {
                        if (j >= 0 && CNTLRS[j].active) {
                            PLAYER_0 = static_cast<uint8_t>(j);
                            break;
                        } else if (k <= XUSER_MAX_COUNT && CNTLRS[k].active) {
                            PLAYER_0 = static_cast<uint8_t>(j);
                            break;
                        }
                    }
                }
            }
        }
    } else if (PLAYER_0 != BAD_PLAYER) {
        dwResult = XInputGetState(PLAYER_0, &CNTLRS[PLAYER_0].getCurrState());
        if (dwResult == ERROR_SUCCESS) {
            CNTLRS[PLAYER_0].update();
        } else {
            CNTLRS[PLAYER_0].active = false;
            PLAYER_0 = BAD_PLAYER;
        }
    }
}

}  // namespace XInput
}  // namespace Input

#endif  //! XINPUT_HELPER_H
#endif  //! USE_XINPUT