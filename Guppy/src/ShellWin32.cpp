/*
 * Copyright (C) 2016 Google, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cassert>
#include <iostream>
#include <sstream>

#include "Game.h"
#include "Helpers.h"
#include "InputHandler.h"
#include "ShellWin32.h"

namespace {

class Win32Timer {
   public:
    Win32Timer() {
        LARGE_INTEGER freq;
        QueryPerformanceFrequency(&freq);
        freq_ = static_cast<double>(freq.QuadPart);

        reset();
    }

    void reset() { QueryPerformanceCounter(&start_); }

    double get() const {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);

        return static_cast<double>(now.QuadPart - start_.QuadPart) / freq_;
    }

   private:
    double freq_;
    LARGE_INTEGER start_;
};

}  // namespace

ShellWin32::ShellWin32(Game &game) : MyShell(game), hwnd_(nullptr), minimized_(false) {
    instance_extensions_.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    init_vk();
}

ShellWin32::~ShellWin32() {
    cleanup_vk();
    FreeLibrary(hmodule_);
}

void ShellWin32::create_window() {
    const std::string class_name(settings_.name + "WindowClass");

    hinstance_ = GetModuleHandle(nullptr);

    WNDCLASSEX win_class = {};
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = window_proc;
    win_class.hInstance = hinstance_;
    win_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    win_class.lpszClassName = class_name.c_str();
    RegisterClassEx(&win_class);

    const DWORD win_style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_OVERLAPPEDWINDOW;

    RECT win_rect = {0, 0, settings_.initial_width, settings_.initial_height};
    AdjustWindowRect(&win_rect, win_style, false);

    hwnd_ = CreateWindowEx(WS_EX_APPWINDOW, class_name.c_str(), settings_.name.c_str(), win_style, 0, 0,
                           win_rect.right - win_rect.left, win_rect.bottom - win_rect.top, nullptr, nullptr, hinstance_, nullptr);

    SetForegroundWindow(hwnd_);
    SetWindowLongPtr(hwnd_, GWLP_USERDATA, (LONG_PTR)this);
}

PFN_vkGetInstanceProcAddr ShellWin32::load_vk() {
    const char filename[] = "vulkan-1.dll";
    HMODULE mod;
    PFN_vkGetInstanceProcAddr get_proc = nullptr;

    mod = LoadLibrary(filename);
    if (mod) {
        get_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(mod, "vkGetInstanceProcAddr"));
    }

    if (!mod || !get_proc) {
        std::stringstream ss;
        ss << "failed to load " << filename;

        if (mod) FreeLibrary(mod);

        throw std::runtime_error(ss.str());
    }

    hmodule_ = mod;

    return get_proc;
}

bool ShellWin32::can_present(VkPhysicalDevice phy, uint32_t queue_family) {
    // return vk::GetPhysicalDeviceWin32PresentationSupportKHR(phy, queue_family) == VK_TRUE;
    return true;
}

VkSurfaceKHR ShellWin32::create_surface(VkInstance instance) {
    VkWin32SurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hinstance = hinstance_;
    surface_info.hwnd = hwnd_;

    VkSurfaceKHR surface;
    vk::assert_success(vkCreateWin32SurfaceKHR(instance, &surface_info, NULL, &surface));

    return surface;
}

LRESULT ShellWin32::handle_message(UINT msg, WPARAM wparam, LPARAM lparam) {
    switch (msg) {
        case WM_SIZE: {
            switch (wparam) {
                case SIZE_MINIMIZED: {
                    minimized_ = true;
                } break;
                case SIZE_RESTORED: {
                    minimized_ = false;
                } break;
                default: {
                    UINT w = LOWORD(lparam);
                    UINT h = HIWORD(lparam);
                    resize_swapchain(w, h);
                } break;
            }
        } break;
        case WM_KEYUP: {
            auto key = get_key(wparam);
            InputHandler::get().removeKeyInput(key);
        } break;
        case WM_KEYDOWN: {
            auto key = get_key(wparam);
            InputHandler::get().addKeyInput(key);
            game_.on_key(key);
        } break;
        case WM_CLOSE:
            game_.on_key(Game::KEY_SHUTDOWN);
            break;
        case WM_DESTROY:
            quit();
            break;
        default:
            return DefWindowProc(hwnd_, msg, wparam, lparam);
            break;
    }

    return 0;
}

Game::Key ShellWin32::get_key(WPARAM wParam) {
    Game::Key key;
    switch (wParam) {
        case VK_ESCAPE:
            key = Game::KEY_ESC;
            break;
        case VK_UP:
            key = Game::KEY_UP;
            break;
        case VK_DOWN:
            key = Game::KEY_DOWN;
            break;
        case VK_LEFT:
            key = Game::KEY_LEFT;
            break;
        case VK_RIGHT:
            key = Game::KEY_RIGHT;
            break;
        case VK_SPACE:
            key = Game::KEY_SPACE;
            break;
        case VK_TAB:
            key = Game::KEY_TAB;
            break;
        case 'F':
        case 'f':
            key = Game::KEY_F;
            break;
        case 'W':
        case 'w':
            key = Game::KEY_W;
            break;
            break;
        case 'A':
        case 'a':
            key = Game::KEY_A;
            break;
        case 'S':
        case 's':
            key = Game::KEY_S;
            break;
        case 'D':
        case 'd':
            key = Game::KEY_D;
            break;
        case 'E':
        case 'e':
            key = Game::KEY_E;
            break;
        case 'Q':
        case 'q':
            key = Game::KEY_Q;
            break;
        case '_':
        case '-':
            key = Game::KEY_PLUS;
            break;
        case '+':
        case '=':
            key = Game::KEY_F1;
            break;
        case VK_F1:
            key = Game::KEY_F1;
            break;
            // case VK_F2:
            //    key = Game::KEY_F2;
            break;
        case VK_F3:
            key = Game::KEY_F3;
            break;
            // case VK_F4:
            //    key = Game::KEY_F4;
            //    break;
        case VK_F5:
            key = Game::KEY_F5;
            break;
        case VK_F6:
            key = Game::KEY_F6;
            break;
        case VK_F7:
            key = Game::KEY_F7;
            break;
            // case VK_F8:
            //    key = Game::KEY_F8;
            //    break;
        case VK_F9:
            key = Game::KEY_F9;
            break;
        case VK_F10:
            key = Game::KEY_F10;
            break;
        case VK_F11:
            key = Game::KEY_F11;
            break;
        case VK_F12:
            key = Game::KEY_F12;
            break;
            // case MOD_ALT:
            //    key = Game::KEY_CTRL;
            //    break;
            // case MOD_CONTROL:
            //    key = Game::KEY_CTRL;
            //    break;
            // case MOD_SHIFT:
            //    key = Game::KEY_CTRL;
            //    break;
        default:
            key = Game::KEY_UNKNOWN;
            break;
    }
    return key;
}

void ShellWin32::quit() { PostQuitMessage(0); }

void ShellWin32::run() {
    create_window();

    create_context();
    resize_swapchain(settings_.initial_width, settings_.initial_height, false);

    Win32Timer timer;
    double current_time = timer.get();

    while (true) {
        bool quit = false;

        assert(settings_.animate);

        // process all messages
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                quit = true;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (quit) {
            break;
        } else if (minimized_) {
            // TODO: somehow pause...
            // std::unique_lock<std::mutex> lock(mtx_);
            // while (minimized_) pause_.wait(lock);
        } else {
            InputHandler::get().updateKeyInput();

            acquire_back_buffer();

            double t = timer.get();
            add_game_time(static_cast<float>(t - current_time));

            present_back_buffer();

            current_time = t;
        }
    }

    destroy_context();

    DestroyWindow(hwnd_);
}
