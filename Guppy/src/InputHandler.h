/*
 * Copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef INPUT_HANDLER_H
#define INPUT_HANDLER_H

#include <glm/glm.hpp>
#include <set>

#include "Shell.h"

namespace Input {

class Handler : public Shell::Handler {
   public:
    Handler(Shell* pShell);

    void init() override {}
    void destroy() override {}

    constexpr const glm::vec3& getPosDir() const { return posDir_; }
    constexpr const glm::vec3& getLookDir() const { return lookDir_; }

    inline void updateKeyInput(GAME_KEY key, INPUT_ACTION type) {
        switch (type) {
            case INPUT_ACTION::UP:
                currKeyInput_.erase(key);
                break;
            case INPUT_ACTION::DOWN:
                currKeyInput_.insert(key);
                break;
            default:;
        }
    }

    inline void updateMousePosition(float xPos, float yPos, float zDelta, bool isLooking, bool move = false,
                                    bool primary = false, bool secondary = false) {
        currMouseInput_.xPos = xPos;
        currMouseInput_.yPos = yPos;
        currMouseInput_.zDelta = zDelta;
        currMouseInput_.moving = move;
        currMouseInput_.primary = primary;
        currMouseInput_.secondary = secondary;
        isLooking_ = isLooking;
    }

    constexpr void mouseLeave() { hasFocus_ = true; }
    void update(const double elapsed) override;
    void updateInput(float elapsed);
    void clear() { reset(); }

    // TODO: all this stuff is garbage
    constexpr const MouseInput& getMouseInput() const { return currMouseInput_; }

   private:
    void reset();

    void updateKeyInput();
    void updateMouseInput();

    std::set<GAME_KEY> currKeyInput_;
    glm::vec3 posDir_;

    bool isLooking_;
    bool hasFocus_;
    MouseInput currMouseInput_;
    MouseInput prevMouseInput_;
    glm::vec3 lookDir_;
};

}  // namespace Input

#endif  // !INPUT_HANDLER_H