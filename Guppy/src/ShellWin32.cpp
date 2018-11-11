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
#include <thread>

#include <tchar.h>
#include <windowsx.h>

#include "Constants.h"
#include "Game.h"
#include "Helpers.h"
#include "ShellWin32.h"

#define LIMIT_FRAMERATE

VOID WINAPI FileIOCompletionRoutine(DWORD, DWORD, LPOVERLAPPED);

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

ShellWin32::ShellWin32(Game& game) : MyShell(game), hwnd_(nullptr), minimized_(false) {
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
    if (settings_.enable_double_clicks) win_class.style = CS_DBLCLKS;
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

VkBool32 ShellWin32::can_present(VkPhysicalDevice phy, uint32_t queue_family) {
    return vkGetPhysicalDeviceWin32PresentationSupportKHR(phy, queue_family);
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

LRESULT ShellWin32::handle_message(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
            // WINDOW EVENTS
        case WM_SIZE: {
            switch (wParam) {
                case SIZE_MINIMIZED: {
                    minimized_ = true;
                } break;
                case SIZE_RESTORED: {
                    minimized_ = false;
                } break;
                default: {
                    UINT w = LOWORD(lParam);
                    UINT h = HIWORD(lParam);
                    resize_swapchain(w, h);
                } break;
            }
        } break;
        case WM_CLOSE: {
            game_.on_key(Game::KEY::KEY_SHUTDOWN);
        } break;
            // MOUSE INPUT
        case WM_MOUSEMOVE: {
            get_mouse_mod(wParam, lParam);
        } break;
        case WM_MOUSELEAVE: {
            InputHandler::mouseLeave();
        } break;
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK: {
            // get_mouse(Game::MOUSE::LEFT, uMsg, lParam);
        } break;
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MBUTTONDBLCLK: {
            // get_mouse(Game::MOUSE::MIDDLE, uMsg, lParam);
        } break;
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK: {
            // get_mouse(Game::MOUSE::RIGHT, uMsg, lParam);
        } break;
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_XBUTTONDBLCLK: {
            // get_mouse(Game::MOUSE::X, uMsg, lParam);
        } break;
            // KEYBOARD INPUT
        case WM_KEYUP: {
            auto key = get_key(wParam, InputHandler::INPUT_TYPE::UP);
        } break;
        case WM_KEYDOWN: {
            auto key = get_key(wParam, InputHandler::INPUT_TYPE::DOWN);
            game_.on_key(key);
        } break;
            // GENERAL
        case WM_DESTROY: {
            quit();
        } break;
        default: { return DefWindowProc(hwnd_, uMsg, wParam, lParam); } break;
    }

    return 0;
}

Game::KEY ShellWin32::get_key(WPARAM wParam, InputHandler::INPUT_TYPE type) {
    Game::KEY key;
    switch (wParam) {
        case VK_ESCAPE:
            key = Game::KEY::KEY_ESC;
            break;
        case VK_UP:
            key = Game::KEY::KEY_UP;
            break;
        case VK_DOWN:
            key = Game::KEY::KEY_DOWN;
            break;
        case VK_LEFT:
            key = Game::KEY::KEY_LEFT;
            break;
        case VK_RIGHT:
            key = Game::KEY::KEY_RIGHT;
            break;
        case VK_SPACE:
            key = Game::KEY::KEY_SPACE;
            break;
        case VK_TAB:
            key = Game::KEY::KEY_TAB;
            break;
        case 'F':
        case 'f':
            key = Game::KEY::KEY_F;
            break;
        case 'W':
        case 'w':
            key = Game::KEY::KEY_W;
            break;
            break;
        case 'A':
        case 'a':
            key = Game::KEY::KEY_A;
            break;
        case 'S':
        case 's':
            key = Game::KEY::KEY_S;
            break;
        case 'D':
        case 'd':
            key = Game::KEY::KEY_D;
            break;
        case 'E':
        case 'e':
            key = Game::KEY::KEY_E;
            break;
        case 'Q':
        case 'q':
            key = Game::KEY::KEY_Q;
            break;
        // NUMBER KEYS
        case '1':
            key = Game::KEY::KEY_1;
            break;
        case '2':
            key = Game::KEY::KEY_2;
            break;
        case '3':
            key = Game::KEY::KEY_3;
            break;
        case '4':
            key = Game::KEY::KEY_4;
            break;
        case '5':
            key = Game::KEY::KEY_5;
            break;
        case '6':
            key = Game::KEY::KEY_6;
            break;
        case '7':
            key = Game::KEY::KEY_7;
            break;
        case '8':
            key = Game::KEY::KEY_8;
            break;
        case '9':
            key = Game::KEY::KEY_9;
            break;
        case '0':
            key = Game::KEY::KEY_0;
            break;
        case '-':
            key = Game::KEY::KEY_MINUS;
            break;
        case '=':
            key = Game::KEY::KEY_PLUS;
            break;
        // FUNCTION KEYS
        case VK_F1:
            key = Game::KEY::KEY_F1;
            break;
            // case VK_F2:
            //    key = Game::KEY::KEY_F2;
            // break;
        case VK_F3:
            key = Game::KEY::KEY_F3;
            break;
            // case VK_F4:
            //    key = Game::KEY::KEY_F4;
            //    break;
        case VK_F5:
            key = Game::KEY::KEY_F5;
            break;
        case VK_F6:
            key = Game::KEY::KEY_F6;
            break;
        case VK_F7:
            key = Game::KEY::KEY_F7;
            break;
            // case VK_F8:
            //    key = Game::KEY::KEY_F8;
            //    break;
        case VK_F9:
            key = Game::KEY::KEY_F9;
            break;
        case VK_F10:
            key = Game::KEY::KEY_F10;
            break;
        case VK_F11:
            key = Game::KEY::KEY_F11;
            break;
        case VK_F12:
            key = Game::KEY::KEY_F12;
            break;
            // case MOD_ALT:
            //    key = Game::KEY::KEY_CTRL;
            //    break;
            // case MOD_CONTROL:
            //    key = Game::KEY::KEY_CTRL;
            //    break;
            // case MOD_SHIFT:
            //    key = Game::KEY::KEY_CTRL;
            //    break;
        default:
            key = Game::KEY::KEY_UNKNOWN;
            break;
    }
    InputHandler::updateKeyInput(key, type);
    return key;
}

void ShellWin32::get_mouse(Game::MOUSE mouse, UINT uMsg, LPARAM lParam) {
    switch (uMsg) {
            // UP
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
        case WM_XBUTTONUP: {
            // nothing here yet...
        } break;
            // DOWN
        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
            break;
        case WM_RBUTTONDOWN: {
            // float xPos = static_cast<float>(GET_X_LPARAM(lParam));
            // float yPos = static_cast<float>(GET_Y_LPARAM(lParam));
            // InputHandler::get().addMouseInput(xPos, yPos);
        } break;
            // DOUBLE CLICK (SETTINGS TOGGLE)
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK: {
            // nothing here yet...
        } break;
    }
}

void ShellWin32::get_mouse_mod(WPARAM wParam, LPARAM lParam) {
    float xPos = static_cast<float>(GET_X_LPARAM(lParam));
    float yPos = static_cast<float>(GET_Y_LPARAM(lParam));
    bool is_looking = false;
    switch (wParam) {
        case MK_RBUTTON: {
            is_looking = true;
        } break;
        case MK_CONTROL:
        case MK_LBUTTON:
        case MK_MBUTTON:
        case MK_SHIFT:
        case MK_XBUTTON1:
        case MK_XBUTTON2: {
            // nothing here yet...
        } break;
    }
    InputHandler::updateMousePosition(xPos, yPos, is_looking);
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

        if (settings_.enable_directory_listener) CheckDirectories();

        if (quit) {
            break;
        } else if (minimized_) {
            // TODO: somehow pause...
            // std::unique_lock<std::mutex> lock(mtx_);
            // while (minimized_) pause_.wait(lock);
        } else {
            acquire_back_buffer();

            // TODO: simplify this?
            double now = timer.get();
            double elapsed = now - current_time;
            current_time = now;

            InputHandler::updateInput(static_cast<float>(elapsed));
            add_game_time(static_cast<float>(elapsed));

            present_back_buffer();
        }

#ifdef LIMIT_FRAMERATE
        // TODO: this is crude and inaccurate.
        DWORD Hz = static_cast<DWORD>(1000 / 10);  // 30Hz
        if (settings_.enable_directory_listener) AsyncAlert(Hz);
#else
        if (settings_.enable_directory_listener) AsyncAlert(0);
#endif
    }

    // Free any directory listening handles
    for (auto& dirInst : dirInsts_) {
        CloseHandle(dirInst.hDir);
    }

    destroy_context();

    DestroyWindow(hwnd_);
}

void ShellWin32::AsyncAlert(DWORD milliseconds) {
    // Watch directory FileIOCompletionRoutine requires alertable thread wait.
    SleepEx(milliseconds, TRUE);
}

void ShellWin32::watch_directory(std::string dir, std::function<void(std::string)> callback) {
    if (settings_.enable_directory_listener) {
        // build absolute path
        dir = GetWorkingDirectory() + ROOT_PATH + dir;
        dirInsts_.push_back({});
        // Completion routine gets called twice for some reason so I use
        // this flag instead of spending another night on this...
        dirInsts_.back().firstComp = true;
        dirInsts_.back().callback = callback;
        // Create directory listener handle
        dirInsts_.back().hDir = CreateFile(dir.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
                                           NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
        // hEvent is not used by completion function so store the listener object for use.
        dirInsts_.back().oOverlap.hEvent = &dirInsts_.back();
    }
}

std::string ShellWin32::GetWorkingDirectory() {
    char result[_MAX_PATH];
    auto fileName = std::string(result, GetModuleFileName(NULL, result, MAX_PATH));
    return helpers::getFilePath(fileName) + "..\\";
}

void ShellWin32::CheckDirectories() {
    for (auto& dirInst : dirInsts_) {
        BOOL result =
            ReadDirectoryChangesW(dirInst.hDir, dirInst.lpBuffer, sizeof(dirInst.lpBuffer), FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE,
                                  dirInst.lpBytesReturned, &dirInst.oOverlap, &FileIOCompletionRoutine);
        if (result == 0) {
            _tprintf(TEXT("\n ERROR: (%s)"), GetLastErrorAsString().c_str());
        }
    }
}

VOID WINAPI FileIOCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped) {
    LPDIR_INST lpDirInst = (LPDIR_INST)lpOverlapped->hEvent;
    lpDirInst->firstComp = !lpDirInst->firstComp;
    if (lpDirInst->firstComp) {
        // Divide file name length by 2 because chars are wide.
        auto ws = std::wstring(lpDirInst->lpBuffer->FileName, lpDirInst->lpBuffer->FileNameLength / 2);
        std::string fileName = std::string(ws.begin(), ws.end());
        // Invoke callback with the name of the file that was modified.
        lpDirInst->callback(fileName);
    }
}

std::string ShellWin32::GetLastErrorAsString() {
    // Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) return std::string();  // No error message has been recorded

    LPSTR messageBuffer = nullptr;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                                 errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);

    // Free the buffer.
    LocalFree(messageBuffer);

    return message;
}