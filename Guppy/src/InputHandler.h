/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <array>
#include <glm/glm.hpp>
#include <set>

#include "Handlee.h"
#include "Shell.h"

namespace Input {

class Handler;

class ManagerBase : public Handlee<Handler> {
   public:
    ManagerBase(Handler& handler) : Handlee(handler) {}
    virtual ~ManagerBase() = default;

    virtual bool update(const uint8_t player) = 0;
};

class KeyboardManager : public ManagerBase {
   public:
    KeyboardManager(Handler& handler)
        : ManagerBase(handler),  //
          keys_(),
          keyDown_(),
          keyUp_() {}

    bool update(const uint8_t player) override;
    inline void keyDown(GAME_KEY key) { keyDown_.insert(key); }
    inline void keyUp(GAME_KEY key) { keyUp_.insert(key); }

   private:
    void prepareKeys();
    // The idea was to have GAM_KEYs map actions in the game to key/controller binds. This seems like overkill atm, so
    // GAME_KEYs are mapped to keyboard keys and then I map the actions to both keys and controller buttons. The following
    // would be completely removed in a user binding layer situation, and the platform shells would just map the input to
    // GAME_ACTIONs or something.
    constexpr bool isMoveKey(GAME_KEY key) {
        return (key == GAME_KEY::UP            //
                || key == GAME_KEY::W          //
                || key == GAME_KEY::DOWN       //
                || key == GAME_KEY::S          //
                || key == GAME_KEY::RIGHT      //
                || key == GAME_KEY::D          //
                || key == GAME_KEY::LEFT       //
                || key == GAME_KEY::A          //
                || key == GAME_KEY::E          //
                || key == GAME_KEY::Q          //
                || key == GAME_KEY::LEFT_SHFT  //
        );
    }

    std::vector<GAME_KEY> keys_;
    std::set<GAME_KEY> keyDown_;
    std::set<GAME_KEY> keyUp_;
};

struct MouseInfo {
    glm::vec2 delta = {};
    glm::vec2 location = {};
    bool right = false;
    bool left = false;
    float wheel = 0.0f;
};

class MouseManager : public ManagerBase {
   public:
    enum class LOOK_TYPE {
        FREE_CLICK,
        GRIP_CLICK,
    };

    MouseManager(Handler& handler)
        : ManagerBase(handler),  //
          info(),
          lookType_(LOOK_TYPE::FREE_CLICK),
          hasFocus_(false),
          previousLocation_() {}

    bool update(const uint8_t player) override;
    constexpr void setLookType(LOOK_TYPE type) { lookType_ = type; }
    constexpr void mouseLeave() { hasFocus_ = false; }

    MouseInfo info;

   private:
    LOOK_TYPE lookType_;
    bool hasFocus_;
    glm::vec2 previousLocation_;
};

class ControllerManager : public ManagerBase {
   public:
    ControllerManager(Handler& handler)
        : ManagerBase(handler),  //
          hadZeroState_(true),
          buttonBits_(GAME_BUTTON::NONE) {}

    bool update(const uint8_t player) override;
    void poll();
#ifdef USE_XINPUT
    bool updateXInput(const uint8_t player);
#endif

   private:
    bool hadZeroState_;
    GameButtonBits buttonBits_;
};

constexpr uint8_t NUM_PLAYERS = 1;

struct PlayerInfo {
    glm::vec3 moveDir = {};
    glm::vec2 lookDir = {};
    GameButtonBits buttons = GAME_BUTTON::NONE;
    const std::vector<GAME_KEY>* pKeys;
    const MouseInfo* pMouse;
};

struct InputInfo {
    std::array<PlayerInfo, NUM_PLAYERS> players = {};
};

class Handler : public Shell::Handler {
    friend class KeyboardManager;
    friend class MouseManager;
    friend class ControllerManager;

   public:
    Handler(Shell* pShell);

    void init() override {}
    void update() override;
    void destroy() override { reset(); }
    void reset() { info_ = {}; }

    // These should probably only be accessible from the shell...
    auto& keyboardMgr() { return keyboardMgr_; }
    auto& mouseMgr() { return mouseMgr_; }
    auto& cntlrMgr() { return cntlrMgr_; }

    const auto& getInfo() const { return info_; }

   private:
    // MANAGERS
    KeyboardManager keyboardMgr_;
    MouseManager mouseMgr_;
    ControllerManager cntlrMgr_;

    InputInfo info_;
};

}  // namespace Input

#endif  // !INPUT_HANDLER_H