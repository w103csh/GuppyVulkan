/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "Game.h"
#include "InputHandler.h"
#include "ShellGLFW.h"

void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);  // TODO: shell logging
}

void glfw_resize_callback(GLFWwindow* window, int w, int h) {
    auto pShell = reinterpret_cast<Shell*>(glfwGetWindowUserPointer(window));
    pShell->resizeSwapchain(w, h);
}

void glfw_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!ImGui::GetIO().WantCaptureMouse) {
        auto pShell = reinterpret_cast<Shell*>(glfwGetWindowUserPointer(window));
        auto& info = pShell->inputHandler().mouseMgr().info;
        info.location.x = static_cast<float>(xpos);
        info.location.y = static_cast<float>(ypos);
        // pShell->log(Shell::LogPriority::LOG_INFO, helpers::toString(info.location).c_str());
    }
}

void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (!ImGui::GetIO().WantCaptureMouse) {
        auto pShell = reinterpret_cast<Shell*>(glfwGetWindowUserPointer(window));
        auto& info = pShell->inputHandler().mouseMgr().info;
        if (button == GLFW_MOUSE_BUTTON_1) {
            info.left = action != GLFW_RELEASE;
        }
        if (button == GLFW_MOUSE_BUTTON_2) {
            info.right = action != GLFW_RELEASE;
        }
    }
}

void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (!ImGui::GetIO().WantCaptureKeyboard || action != GLFW_PRESS) {
        auto pShell = reinterpret_cast<Shell*>(glfwGetWindowUserPointer(window));
        GAME_KEY gKey;
        switch (key) {
            case GLFW_KEY_ESCAPE:
                gKey = GAME_KEY::ESC;
                break;
            case GLFW_KEY_UP:
                gKey = GAME_KEY::UP;
                break;
            case GLFW_KEY_DOWN:
                gKey = GAME_KEY::DOWN;
                break;
            case GLFW_KEY_LEFT:
                gKey = GAME_KEY::LEFT;
                break;
            case GLFW_KEY_RIGHT:
                gKey = GAME_KEY::RIGHT;
                break;
            case GLFW_KEY_SPACE:
                gKey = GAME_KEY::SPACE;
                break;
            case GLFW_KEY_TAB:
                gKey = GAME_KEY::TAB;
                break;
            case GLFW_KEY_F:
                gKey = GAME_KEY::F;
                break;
            case GLFW_KEY_W:
                gKey = GAME_KEY::W;
                break;
            case GLFW_KEY_A:
                gKey = GAME_KEY::A;
                break;
            case GLFW_KEY_S:
                gKey = GAME_KEY::S;
                break;
            case GLFW_KEY_D:
                gKey = GAME_KEY::D;
                break;
            case GLFW_KEY_E:
                gKey = GAME_KEY::E;
                break;
            case GLFW_KEY_Q:
                gKey = GAME_KEY::Q;
                break;
            case GLFW_KEY_O:
                gKey = GAME_KEY::O;
                break;
            case GLFW_KEY_P:
                gKey = GAME_KEY::P;
                break;
            case GLFW_KEY_K:
                gKey = GAME_KEY::K;
                break;
            case GLFW_KEY_L:
                gKey = GAME_KEY::L;
                break;
            case GLFW_KEY_I:
                gKey = GAME_KEY::I;
                break;
            // BRACKET/BRACE KEYS
            case GLFW_KEY_LEFT_BRACKET:
                gKey = GAME_KEY::LEFT_BRACKET;
                break;
            case GLFW_KEY_RIGHT_BRACKET:
                gKey = GAME_KEY::RIGHT_BRACKET;
                break;
            // NUMBER KEYS
            case GLFW_KEY_1:
                gKey = GAME_KEY::TOP_1;
                break;
            case GLFW_KEY_2:
                gKey = GAME_KEY::TOP_2;
                break;
            case GLFW_KEY_3:
                gKey = GAME_KEY::TOP_3;
                break;
            case GLFW_KEY_4:
                gKey = GAME_KEY::TOP_4;
                break;
            case GLFW_KEY_5:
                gKey = GAME_KEY::TOP_5;
                break;
            case GLFW_KEY_6:
                gKey = GAME_KEY::TOP_6;
                break;
            case GLFW_KEY_7:
                gKey = GAME_KEY::TOP_7;
                break;
            case GLFW_KEY_8:
                gKey = GAME_KEY::TOP_8;
                break;
            case GLFW_KEY_9:
                gKey = GAME_KEY::TOP_9;
                break;
            case GLFW_KEY_0:
                gKey = GAME_KEY::TOP_0;
                break;
            case GLFW_KEY_MINUS:
                gKey = GAME_KEY::MINUS;
                break;
            case GLFW_KEY_EQUAL:
                gKey = GAME_KEY::EQUALS;
                break;
            case GLFW_KEY_BACKSPACE:
                gKey = GAME_KEY::BACKSPACE;
                break;
            // FUNCTION KEYS
            case GLFW_KEY_F1:
                gKey = GAME_KEY::F1;
                break;
            case GLFW_KEY_F2:
                gKey = GAME_KEY::F2;
                break;
            case GLFW_KEY_F3:
                gKey = GAME_KEY::F3;
                break;
            case GLFW_KEY_F4:
                gKey = GAME_KEY::F4;
                break;
            case GLFW_KEY_F5:
                gKey = GAME_KEY::F5;
                break;
            case GLFW_KEY_F6:
                gKey = GAME_KEY::F6;
                break;
            case GLFW_KEY_F7:
                gKey = GAME_KEY::F7;
                break;
            case GLFW_KEY_F8:
                gKey = GAME_KEY::F8;
                break;
            case GLFW_KEY_F9:
                gKey = GAME_KEY::F9;
                break;
            case GLFW_KEY_F10:
                gKey = GAME_KEY::F10;
                break;
            case GLFW_KEY_F11:
                gKey = GAME_KEY::F11;
                break;
            case GLFW_KEY_F12:
                gKey = GAME_KEY::F12;
                break;
            // case MOD_ALT:
            //    gKey = GAME_KEY::CTRL;
            //    break;
            // case MOD_CONTROL:
            //    gKey = GAME_KEY::CTRL;
            //    break;
            // case MOD_SHIFT:
            //    gKey = GAME_KEY::CTRL;
            //    break;
            case GLFW_KEY_COMMA:
                gKey = GAME_KEY::COMMA;
                break;
            case GLFW_KEY_PERIOD:
                gKey = GAME_KEY::PERIOD;
                break;
            case GLFW_KEY_SLASH:
                gKey = GAME_KEY::SLASH;
                break;
            case GLFW_KEY_LEFT_SHIFT:
                gKey = GAME_KEY::LEFT_SHFT;
                break;
            case GLFW_KEY_LEFT_CONTROL:
                gKey = GAME_KEY::LEFT_CTRL;
                break;
            case GLFW_KEY_LEFT_ALT:
                gKey = GAME_KEY::LEFT_ALT;
                break;
            default:
                gKey = GAME_KEY::UNKNOWN;
                break;
        }

        switch (action) {
            case GLFW_PRESS: {
                // pShell->log(Shell::LogPriority::LOG_INFO, "DOWN");
                pShell->inputHandler().keyboardMgr().keyDown(gKey);
            } break;
            case GLFW_RELEASE: {
                // pShell->log(Shell::LogPriority::LOG_INFO, "UP");
                pShell->inputHandler().keyboardMgr().keyUp(gKey);
            } break;
            case GLFW_REPEAT:
            default:
                break;
        }
    }
}
