
#include <glm/glm.hpp>
#include <sstream>

#include "InputHandler.h"

const bool MY_DEBUG = true;

// TDOD: these are probably platform specific
constexpr float X_AXIS_MOVEMENT = 0.01f;
constexpr float Y_AXIS_MOVEMENT = 0.01f;
constexpr float Z_AXIS_MOVEMENT = 0.01f;

InputHandler::InputHandler(MyShell* sh) : sh_(sh) { assert(sh != nullptr); }

void InputHandler::updateKeyInput() {
    std::stringstream ss;
    reset();

    // if (action == GLFW_PRESS || action == GLFW_REPEAT)
    //{
    //    std::cout << "pressed: " << key << std::endl;

    //    // TODO: I'm sure there is faster math here somehow.

    //    if (key == GLFWDirectionKeyMap.UP)
    //    {
    //        pos_dir_.y += Y_AXIS_MOVEMENT;
    //    }
    //    else if (key == GLFWDirectionKeyMap.DOWN)
    //    {
    //        pos_dir_.y += Y_AXIS_MOVEMENT * -1;
    //    }
    //    else if (key == GLFWDirectionKeyMap.FORWARD)
    //    {
    //        pos_dir_.z += Z_AXIS_MOVEMENT;
    //    }
    //    else if (key == GLFWDirectionKeyMap.BACK)
    //    {
    //        pos_dir_.z += Z_AXIS_MOVEMENT * -1;
    //    }
    //    else if (key == GLFWDirectionKeyMap.RIGHT)
    //    {
    //        pos_dir_.x += X_AXIS_MOVEMENT;
    //    }
    //    else if (key == GLFWDirectionKeyMap.LEFT)
    //    {
    //        pos_dir_.x += X_AXIS_MOVEMENT * -1;
    //    }
    //}

    for (auto& key : curr_key_input_) {
        switch (key) {
                // FORWARD
            case Game::KEY_UP:
            case Game::KEY_W: {
                if (MY_DEBUG) ss << " FORWARD ";
                pos_dir_.z += Z_AXIS_MOVEMENT * -1;
            } break;
                // BACK
            case Game::KEY_DOWN:
            case Game::KEY_S: {
                if (MY_DEBUG) ss << " BACK ";
                pos_dir_.z += Z_AXIS_MOVEMENT;
            } break;
                // RIGHT
            case Game::KEY_RIGHT:
            case Game::KEY_D: {
                if (MY_DEBUG) ss << " RIGHT ";
                pos_dir_.x += X_AXIS_MOVEMENT * -1;
            } break;
                // LEFT
            case Game::KEY_LEFT:
            case Game::KEY_A: {
                if (MY_DEBUG) ss << " LEFT ";
                pos_dir_.x += X_AXIS_MOVEMENT;
            } break;
                // UP
            case Game::KEY_E: {
                if (MY_DEBUG) ss << " UP ";
                pos_dir_.y += Y_AXIS_MOVEMENT * -1;
            } break;
                // DOWN
            case Game::KEY_Q: {
                if (MY_DEBUG) ss << " DOWN ";
                pos_dir_.y += Y_AXIS_MOVEMENT;
            } break;
        }
    }

    auto msg = ss.str();
    if (MY_DEBUG && msg.size() > 0) {
        ss << "\ninput direction: (" << pos_dir_.x << ", " << pos_dir_.y << ", " << pos_dir_.z << ")" << std::endl;
        sh_->log(MyShell::LOG_INFO, ss.str().c_str());
    }
}

void InputHandler::reset() {
    look_dir_ = {};
    pos_dir_ = {};
}