/*
 * Modifications copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 * -------------------------------
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
#include <utility>

#include <tchar.h>
#include <windowsx.h>

#include <Common/Helpers.h>

#include "ConstantsAll.h"
#include "Game.h"
#include "ShellWin32.h"
// HANLDERS
#include "InputHandler.h"
#include "SoundHandler.h"
#include "SoundFModHandler.h"

VOID WINAPI FileIOCompletionRoutine(DWORD, DWORD, LPOVERLAPPED);

namespace {
class Win32Timer {
   public:
    Win32Timer() : freq_(), start_() {
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

ShellWin32::ShellWin32(Game& game)
    : Shell(game,
            Shell::Handlers{
                std::make_unique<Input::Handler>(this),
#ifdef USE_FMOD
                std::make_unique<Sound::FModHandler>(this),
#elif
                std::make_unique<Sound::Handler>(this),
#endif
            }),
      hinstance_(nullptr),
      hwnd_(nullptr),
      hmodule_(nullptr),
      minimized_(false) {
}

ShellWin32::~ShellWin32() {
    cleanup();
    FreeLibrary(hmodule_);
}

void ShellWin32::setPlatformSpecificExtensions() { addInstanceEnabledExtensionName(VK_KHR_WIN32_SURFACE_EXTENSION_NAME); }

void ShellWin32::createWindow() {
    const std::string class_name(settings_.name + "WindowClass");

    hinstance_ = GetModuleHandle(nullptr);

    WNDCLASSEX win_class = {};
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = window_proc;
    win_class.hInstance = hinstance_;
    win_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    win_class.lpszClassName = class_name.c_str();
    if (settings_.enableDoubleClicks) win_class.style = CS_DBLCLKS;
    RegisterClassEx(&win_class);

    const DWORD win_style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_OVERLAPPEDWINDOW;

    RECT win_rect = {0, 0, settings_.initialWidth, settings_.initialHeight};
    AdjustWindowRect(&win_rect, win_style, false);

    hwnd_ = CreateWindowEx(WS_EX_APPWINDOW, class_name.c_str(), settings_.name.c_str(), win_style, 0, 0,
                           win_rect.right - win_rect.left, win_rect.bottom - win_rect.top, nullptr, nullptr, hinstance_,
                           nullptr);

    SetForegroundWindow(hwnd_);
    SetWindowLongPtr(hwnd_, GWLP_USERDATA, (LONG_PTR)this);
}

PFN_vkGetInstanceProcAddr ShellWin32::load() {
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

vk::Bool32 ShellWin32::canPresent(vk::PhysicalDevice phy, uint32_t queueFamily) {
    return phy.getWin32PresentationSupportKHR(queueFamily);
}

vk::SurfaceKHR ShellWin32::createSurface(vk::Instance instance) {
    vk::Win32SurfaceCreateInfoKHR surfaceInfo = {};
    surfaceInfo.hinstance = hinstance_;
    surfaceInfo.hwnd = hwnd_;

    return instance.createWin32SurfaceKHR(surfaceInfo, context().pAllocator);
}

LRESULT ShellWin32::handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
                    resizeSwapchain(w, h);
                } break;
            }
        } break;
        case WM_CLOSE: {
            game_.onKey(GAME_KEY::KEY_SHUTDOWN);
        } break;
            // MOUSE INPUT
        case WM_MOUSEWHEEL:
        case WM_MOUSEMOVE: {
            getMouseModifier(wParam, lParam);
        } break;
        case WM_MOUSELEAVE: {
            handlers_.pInput->mouseLeave();
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
            auto key = getKey(wParam, INPUT_ACTION::UP);
        } break;
        case WM_KEYDOWN: {
            auto key = getKey(wParam, INPUT_ACTION::DOWN);
            onKey(key);
        } break;
            // GENERAL
        case WM_DESTROY: {
            quit();
        } break;
        default: {
            return DefWindowProc(hwnd_, uMsg, wParam, lParam);
        } break;
    }

    return 0;
}

GAME_KEY ShellWin32::getKey(WPARAM wParam, INPUT_ACTION type) {
    GAME_KEY key;
    switch (wParam) {
        case VK_ESCAPE:
            key = GAME_KEY::KEY_ESC;
            break;
        case VK_UP:
            key = GAME_KEY::KEY_UP;
            break;
        case VK_DOWN:
            key = GAME_KEY::KEY_DOWN;
            break;
        case VK_LEFT:
            key = GAME_KEY::KEY_LEFT;
            break;
        case VK_RIGHT:
            key = GAME_KEY::KEY_RIGHT;
            break;
        case VK_SPACE:
            key = GAME_KEY::KEY_SPACE;
            break;
        case VK_TAB:
            key = GAME_KEY::KEY_TAB;
            break;
        case 'F':
        case 'f':
            key = GAME_KEY::KEY_F;
            break;
        case 'W':
        case 'w':
            key = GAME_KEY::KEY_W;
            break;
            break;
        case 'A':
        case 'a':
            key = GAME_KEY::KEY_A;
            break;
        case 'S':
        case 's':
            key = GAME_KEY::KEY_S;
            break;
        case 'D':
        case 'd':
            key = GAME_KEY::KEY_D;
            break;
        case 'E':
        case 'e':
            key = GAME_KEY::KEY_E;
            break;
        case 'Q':
        case 'q':
            key = GAME_KEY::KEY_Q;
        case 'O':
            key = GAME_KEY::KEY_O;
        case 'P':
            key = GAME_KEY::KEY_P;
            break;
        case 'K':
            key = GAME_KEY::KEY_K;
            break;
        case 'L':
            key = GAME_KEY::KEY_L;
            break;
        case 'I':
            key = GAME_KEY::KEY_I;
            break;
        // BRACKET/BRACE KEYS
        case '[':
            key = GAME_KEY::KEY_LEFT_BRACKET;
            break;
        case ']':
            key = GAME_KEY::KEY_RIGHT_BRACKET;
            break;
        // NUMBER KEYS
        case '1':
            key = GAME_KEY::KEY_1;
            break;
        case '2':
            key = GAME_KEY::KEY_2;
            break;
        case '3':
            key = GAME_KEY::KEY_3;
            break;
        case '4':
            key = GAME_KEY::KEY_4;
            break;
        case '5':
            key = GAME_KEY::KEY_5;
            break;
        case '6':
            key = GAME_KEY::KEY_6;
            break;
        case '7':
            key = GAME_KEY::KEY_7;
            break;
        case '8':
            key = GAME_KEY::KEY_8;
            break;
        case '9':
            key = GAME_KEY::KEY_9;
            break;
        case '0':
            key = GAME_KEY::KEY_0;
            break;
        case '-':
            key = GAME_KEY::KEY_MINUS;
            break;
        case '=':
            key = GAME_KEY::KEY_EQUALS;
            break;
        case VK_BACK:
            key = GAME_KEY::KEY_BACKSPACE;
            break;
        // FUNCTION KEYS
        case VK_F1:
            key = GAME_KEY::KEY_F1;
            break;
            // case VK_F2:
            //    key = GAME_KEY::KEY_F2;
            // break;
        case VK_F3:
            key = GAME_KEY::KEY_F3;
            break;
            // case VK_F4:
            //    key = GAME_KEY::KEY_F4;
            //    break;
        case VK_F5:
            key = GAME_KEY::KEY_F5;
            break;
        case VK_F6:
            key = GAME_KEY::KEY_F6;
            break;
        case VK_F7:
            key = GAME_KEY::KEY_F7;
            break;
            // case VK_F8:
            //    key = GAME_KEY::KEY_F8;
            //    break;
        case VK_F9:
            key = GAME_KEY::KEY_F9;
            break;
        case VK_F10:
            key = GAME_KEY::KEY_F10;
            break;
        case VK_F11:
            key = GAME_KEY::KEY_F11;
            break;
        case VK_F12:
            key = GAME_KEY::KEY_F12;
            break;
            // case MOD_ALT:
            //    key = GAME_KEY::KEY_CTRL;
            //    break;
            // case MOD_CONTROL:
            //    key = GAME_KEY::KEY_CTRL;
            //    break;
            // case MOD_SHIFT:
            //    key = GAME_KEY::KEY_CTRL;
            //    break;
        default:
            key = GAME_KEY::KEY_UNKNOWN;
            break;
    }
    handlers_.pInput->updateKeyInput(key, type);
    return key;
}

void ShellWin32::getMouse(UINT uMsg, LPARAM lParam) {
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
            // InputHandler::inst().addMouseInput(xPos, yPos);
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

void ShellWin32::getMouseModifier(WPARAM wParam, LPARAM lParam) {
    float xPos = static_cast<float>(GET_X_LPARAM(lParam));
    float yPos = static_cast<float>(GET_Y_LPARAM(lParam));
    float zDelta = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam));
    bool isLooking = false;
    switch (wParam) {
        case MK_RBUTTON: {
            isLooking = true;
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
    handlers_.pInput->updateMousePosition(xPos, yPos, zDelta, isLooking);
}

void ShellWin32::quit() const { PostQuitMessage(0); }

void ShellWin32::run() {
    setPlatformSpecificExtensions();
    init();

    createWindow();

    createContext();
    resizeSwapchain(settings_.initialWidth, settings_.initialHeight, false);

    Win32Timer timer;
    currentTime_ = timer.get();

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

        if (settings_.enableDirectoryListener) checkDirectories();

        if (quit) {
            break;
        } else if (minimized_) {
            // TODO: somehow pause...
            // std::unique_lock<std::mutex> lock(mtx_);
            // while (minimized_) pause_.wait(lock);
        } else {
            acquireBackBuffer();

            double now = timer.get();
            elapsedTime_ = now - currentTime_;
            currentTime_ = now;

            update(elapsedTime_);
            addGameTime(static_cast<float>(elapsedTime_));

            presentBackBuffer();
        }

        if (limitFramerate) {
            // TODO: this is crude and inaccurate.
            DWORD Hz = static_cast<DWORD>(1000 / framesPerSecondLimit);  // 30Hz
            if (settings_.enableDirectoryListener) asyncAlert(Hz);
        } else {
            if (settings_.enableDirectoryListener) asyncAlert(0);
        }

        handlers_.pInput->clear();
    }

    // Free any directory listening handles
    for (auto& dirInst : dirInsts_) {
        CloseHandle(dirInst.hDir);
    }

    destroyContext();
    DestroyWindow(hwnd_);
}

void ShellWin32::asyncAlert(uint64_t milliseconds) {
    // Watch directory FileIOCompletionRoutine requires alertable thread wait.
    SleepEx(static_cast<DWORD>(milliseconds), TRUE);
}

void ShellWin32::watchDirectory(const std::string& directory, std::function<void(std::string)> callback) {
    if (settings_.enableDirectoryListener) {
        // build absolute path
        auto fullPath = GetWorkingDirectory() + ROOT_PATH + directory;
        dirInsts_.push_back({});
        // Completion routine gets called twice for some reason so I use
        // this flag instead of spending another night on this...
        dirInsts_.back().firstComp = true;
        dirInsts_.back().callback = callback;
        // Create directory listener handle
        dirInsts_.back().hDir =
            CreateFile(fullPath.c_str(), FILE_LIST_DIRECTORY, FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
                       OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
        // hEvent is not used by completion function so store the listener object for use.
        dirInsts_.back().oOverlap.hEvent = &dirInsts_.back();
    }
}

std::string ShellWin32::GetWorkingDirectory() {
    char result[_MAX_PATH];
    auto fileName = std::string(result, GetModuleFileName(NULL, result, MAX_PATH));
    std::replace(fileName.begin(), fileName.end(), '\\', '/');
    return helpers::getFilePath(fileName) + "../";
}

void ShellWin32::checkDirectories() {
    for (auto& dirInst : dirInsts_) {
        BOOL result = ReadDirectoryChangesW(dirInst.hDir, dirInst.lpBuffer, sizeof(dirInst.lpBuffer), FALSE,
                                            FILE_NOTIFY_CHANGE_LAST_WRITE, dirInst.lpBytesReturned, &dirInst.oOverlap,
                                            &FileIOCompletionRoutine);
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
        // TODO: Handle this cast properly.
        std::string fileName;
        for (auto wc : ws) fileName.push_back(static_cast<char>(wc));
        // Invoke callback with the name of the file that was modified.
        lpDirInst->callback(fileName);
    }
}

std::string ShellWin32::GetLastErrorAsString() {
    // Get the error message, if any.
    DWORD errorMessageID = ::GetLastError();
    if (errorMessageID == 0) return std::string();  // No error message has been recorded

    LPSTR messageBuffer = nullptr;
    size_t size =
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                       errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

    std::string message(messageBuffer, size);

    // Free the buffer.
    LocalFree(messageBuffer);

    return message;
}