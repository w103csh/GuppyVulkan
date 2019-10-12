
#include "Game.h"
#include "InputHandler.h"
#include "ShellGLFW.h"

namespace {
const bool NOTIFY_ON_KEY_UP = true;
bool IS_LOOKING = false;
bool MOUSE_PRIMARY = false;
bool MOUSE_SECONDARY = false;
}  // namespace

void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);  // TODO: shell logging
}

void glfw_resize_callback(GLFWwindow* window, int w, int h) {
    auto pShell = reinterpret_cast<Shell*>(glfwGetWindowUserPointer(window));
    pShell->resizeSwapchain(w, h);
}

void glfw_cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    auto pShell = reinterpret_cast<Shell*>(glfwGetWindowUserPointer(window));
    pShell->inputHandler().updateMousePosition(static_cast<float>(xpos), static_cast<float>(ypos), 0.0, IS_LOOKING,
                                               !ImGui::GetIO().WantCaptureMouse, MOUSE_PRIMARY, MOUSE_SECONDARY);
}

void glfw_mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureMouse && action == GLFW_PRESS) {
        if (button == GLFW_MOUSE_BUTTON_1) {
            MOUSE_PRIMARY = true;
        }
        if (button == GLFW_MOUSE_BUTTON_2) {
            MOUSE_SECONDARY = true;
            IS_LOOKING = true;
        }
    } else if (action == GLFW_RELEASE) {
        if (button == GLFW_MOUSE_BUTTON_1) {
            MOUSE_PRIMARY = false;
        }
        if (button == GLFW_MOUSE_BUTTON_2) {
            MOUSE_SECONDARY = false;
            IS_LOOKING = false;
        }
    }
}

void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    INPUT_ACTION type;
    switch (action) {
        case GLFW_PRESS:
            type = INPUT_ACTION::DOWN;
            break;
        case GLFW_RELEASE:
            type = INPUT_ACTION::UP;
            break;
        default:
            type = INPUT_ACTION::UNKNOWN;
    }

    ImGuiIO& io = ImGui::GetIO();
    if (!io.WantCaptureKeyboard || type != INPUT_ACTION::DOWN) {
        GAME_KEY gKey;
        switch (key) {
            case GLFW_KEY_ESCAPE:
                gKey = GAME_KEY::KEY_ESC;
                break;
            case GLFW_KEY_UP:
                gKey = GAME_KEY::KEY_UP;
                break;
            case GLFW_KEY_DOWN:
                gKey = GAME_KEY::KEY_DOWN;
                break;
            case GLFW_KEY_LEFT:
                gKey = GAME_KEY::KEY_LEFT;
                break;
            case GLFW_KEY_RIGHT:
                gKey = GAME_KEY::KEY_RIGHT;
                break;
            case GLFW_KEY_SPACE:
                gKey = GAME_KEY::KEY_SPACE;
                break;
            case GLFW_KEY_TAB:
                gKey = GAME_KEY::KEY_TAB;
                break;
            case GLFW_KEY_F:

                gKey = GAME_KEY::KEY_F;
                break;
            case GLFW_KEY_W:
                gKey = GAME_KEY::KEY_W;
                break;
            case GLFW_KEY_A:
                gKey = GAME_KEY::KEY_A;
                break;
            case GLFW_KEY_S:
                gKey = GAME_KEY::KEY_S;
                break;
            case GLFW_KEY_D:
                gKey = GAME_KEY::KEY_D;
                break;
            case GLFW_KEY_E:
                gKey = GAME_KEY::KEY_E;
                break;
            case GLFW_KEY_Q:
                gKey = GAME_KEY::KEY_Q;
                break;
            case GLFW_KEY_O:
                gKey = GAME_KEY::KEY_O;
                break;
            case GLFW_KEY_P:
                gKey = GAME_KEY::KEY_P;
                break;
            case GLFW_KEY_K:
                gKey = GAME_KEY::KEY_K;
                break;
            case GLFW_KEY_L:
                gKey = GAME_KEY::KEY_L;
                break;
            case GLFW_KEY_I:
                gKey = GAME_KEY::KEY_I;
                break;
            // BRACKET/BRACE KEYS
            case GLFW_KEY_LEFT_BRACKET:
                gKey = GAME_KEY::KEY_LEFT_BRACKET;
                break;
            case GLFW_KEY_RIGHT_BRACKET:
                gKey = GAME_KEY::KEY_RIGHT_BRACKET;
                break;
            // NUMBER KEYS
            case GLFW_KEY_1:
                gKey = GAME_KEY::KEY_1;
                break;
            case GLFW_KEY_2:
                gKey = GAME_KEY::KEY_2;
                break;
            case GLFW_KEY_3:
                gKey = GAME_KEY::KEY_3;
                break;
            case GLFW_KEY_4:
                gKey = GAME_KEY::KEY_4;
                break;
            case GLFW_KEY_5:
                gKey = GAME_KEY::KEY_5;
                break;
            case GLFW_KEY_6:
                gKey = GAME_KEY::KEY_6;
                break;
            case GLFW_KEY_7:
                gKey = GAME_KEY::KEY_7;
                break;
            case GLFW_KEY_8:
                gKey = GAME_KEY::KEY_8;
                break;
            case GLFW_KEY_9:
                gKey = GAME_KEY::KEY_9;
                break;
            case GLFW_KEY_0:
                gKey = GAME_KEY::KEY_0;
                break;
            case GLFW_KEY_MINUS:
                gKey = GAME_KEY::KEY_MINUS;
                break;
            case GLFW_KEY_EQUAL:
                gKey = GAME_KEY::KEY_EQUALS;
                break;
            case GLFW_KEY_BACKSPACE:
                gKey = GAME_KEY::KEY_BACKSPACE;
                break;
            // FUNCTION KEYS
            case GLFW_KEY_F1:
                gKey = GAME_KEY::KEY_F1;
                break;
            case GLFW_KEY_F2:
                gKey = GAME_KEY::KEY_F2;
                break;
            case GLFW_KEY_F3:
                gKey = GAME_KEY::KEY_F3;
                break;
            case GLFW_KEY_F4:
                gKey = GAME_KEY::KEY_F4;
                break;
            case GLFW_KEY_F5:
                gKey = GAME_KEY::KEY_F5;
                break;
            case GLFW_KEY_F6:
                gKey = GAME_KEY::KEY_F6;
                break;
            case GLFW_KEY_F7:
                gKey = GAME_KEY::KEY_F7;
                break;
            case GLFW_KEY_F8:
                gKey = GAME_KEY::KEY_F8;
                break;
            case GLFW_KEY_F9:
                gKey = GAME_KEY::KEY_F9;
                break;
            case GLFW_KEY_F10:
                gKey = GAME_KEY::KEY_F10;
                break;
            case GLFW_KEY_F11:
                gKey = GAME_KEY::KEY_F11;
                break;
            case GLFW_KEY_F12:
                gKey = GAME_KEY::KEY_F12;
                break;
            // case MOD_ALT:
            //    gKey = GAME_KEY::KEY_CTRL;
            //    break;
            // case MOD_CONTROL:
            //    gKey = GAME_KEY::KEY_CTRL;
            //    break;
            // case MOD_SHIFT:
            //    gKey = GAME_KEY::KEY_CTRL;
            //    break;
            default:
                gKey = GAME_KEY::KEY_UNKNOWN;
                break;
        }

        auto pShell = reinterpret_cast<Shell*>(glfwGetWindowUserPointer(window));
        pShell->inputHandler().updateKeyInput(gKey, type);

        if (NOTIFY_ON_KEY_UP && type == INPUT_ACTION::UP) {
            pShell->onKey(gKey);
        }
    }
}
