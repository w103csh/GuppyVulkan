/*
 * Modifications copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
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
    LRESULT result = 0;
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
        case WM_CLOSE:
        case WM_DESTROY: {
            quit();
        } break;
        default: {
            // TRANSLATE INPUT
            if (translateKey(uMsg, wParam)) break;
            if (translateMouse(uMsg, wParam, lParam)) break;
            // Forward the information if nothing was used.
            result = DefWindowProc(hwnd_, uMsg, wParam, lParam);
        }
    }

    return result;
}

bool ShellWin32::translateKey(UINT uMsg, WPARAM wParam) {
    bool readInput = false;

    GAME_KEY key = GAME_KEY::UNKNOWN;
    switch (wParam) {
        case VK_ESCAPE:
            key = GAME_KEY::ESC;
            break;
        case VK_UP:
            key = GAME_KEY::UP;
            break;
        case VK_DOWN:
            key = GAME_KEY::DOWN;
            break;
        case VK_LEFT:
            key = GAME_KEY::LEFT;
            break;
        case VK_RIGHT:
            key = GAME_KEY::RIGHT;
            break;
        case VK_SPACE:
            key = GAME_KEY::SPACE;
            break;
        case VK_TAB:
            key = GAME_KEY::TAB;
            break;
        case 'F':
        case 'f':
            key = GAME_KEY::F;
            break;
        case 'W':
        case 'w':
            key = GAME_KEY::W;
            break;
        case 'A':
        case 'a':
            key = GAME_KEY::A;
            break;
        case 'S':
        case 's':
            key = GAME_KEY::S;
            break;
        case 'D':
        case 'd':
            key = GAME_KEY::D;
            break;
        case 'E':
        case 'e':
            key = GAME_KEY::E;
            break;
        case 'Q':
        case 'q':
            key = GAME_KEY::Q;
            break;
        case 'O':
            key = GAME_KEY::O;
            break;
        case 'P':
            key = GAME_KEY::P;
            break;
        case 'K':
            key = GAME_KEY::K;
            break;
        case 'L':
            key = GAME_KEY::L;
            break;
        case 'I':
            key = GAME_KEY::I;
            break;
        case 'T':
            key = GAME_KEY::T;
            break;
        // BRACKET/BRACE KEYS
        case '[':
            key = GAME_KEY::LEFT_BRACKET;
            break;
        case ']':
            key = GAME_KEY::RIGHT_BRACKET;
            break;
        // NUMBER KEYS
        case '1':
            key = GAME_KEY::TOP_1;
            break;
        case '2':
            key = GAME_KEY::TOP_2;
            break;
        case '3':
            key = GAME_KEY::TOP_3;
            break;
        case '4':
            key = GAME_KEY::TOP_4;
            break;
        case '5':
            key = GAME_KEY::TOP_5;
            break;
        case '6':
            key = GAME_KEY::TOP_6;
            break;
        case '7':
            key = GAME_KEY::TOP_7;
            break;
        case '8':
            key = GAME_KEY::TOP_8;
            break;
        case '9':
            key = GAME_KEY::TOP_9;
            break;
        case '0':
            key = GAME_KEY::TOP_0;
            break;
        case '-':
            key = GAME_KEY::MINUS;
            break;
        case '=':
            key = GAME_KEY::EQUALS;
            break;
        case VK_BACK:
            key = GAME_KEY::BACKSPACE;
            break;
        // FUNCTION KEYS
        case VK_F1:
            key = GAME_KEY::F1;
            break;
            // case VK_F2:
            //    key = GAME_KEY::F2;
            // break;
        case VK_F3:
            key = GAME_KEY::F3;
            break;
            // case VK_F4:
            //    key = GAME_KEY::F4;
            //    break;
        case VK_F5:
            key = GAME_KEY::F5;
            break;
        case VK_F6:
            key = GAME_KEY::F6;
            break;
        case VK_F7:
            key = GAME_KEY::F7;
            break;
            // case VK_F8:
            //    key = GAME_KEY::F8;
            //    break;
        case VK_F9:
            key = GAME_KEY::F9;
            break;
        case VK_F10:
            key = GAME_KEY::F10;
            break;
        case VK_F11:
            key = GAME_KEY::F11;
            break;
        case VK_F12:
            key = GAME_KEY::F12;
            break;
            // case MOD_ALT:
            //    key = GAME_KEY::CTRL;
            //    break;
            // case MOD_CONTROL:
            //    key = GAME_KEY::CTRL;
            //    break;
            // case MOD_SHIFT:
            //    key = GAME_KEY::CTRL;
            //    break;
        default:;
    }

    if (key != GAME_KEY::UNKNOWN) {
        readInput = true;
        switch (uMsg) {
            case WM_KEYDOWN: {
                inputHandler().keyboardMgr().keyDown(key);
            } break;
            case WM_KEYUP: {
                inputHandler().keyboardMgr().keyUp(key);
            } break;
            default:;
        }
    }

    return readInput;
}

bool ShellWin32::translateMouse(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    bool readInput = false;

    auto& info = inputHandler().mouseMgr().info;
    switch (uMsg) {
        case WM_MOUSEWHEEL: {
            readInput = true;
            info.wheel = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam));
        } break;
        case WM_MOUSEMOVE: {
            readInput = true;
            info.location.x = static_cast<float>(GET_X_LPARAM(lParam));
            info.location.y = static_cast<float>(GET_Y_LPARAM(lParam));
            switch (wParam) {
                case MK_RBUTTON:
                case MK_LBUTTON:
                case MK_CONTROL:
                case MK_MBUTTON:
                case MK_SHIFT:
                case MK_XBUTTON1:
                case MK_XBUTTON2:
                    break;
            }
        } break;
        case WM_MOUSELEAVE: {
            readInput = true;
            inputHandler().mouseMgr().mouseLeave();
        } break;
        // UP
        case WM_LBUTTONUP: {
            readInput = true;
            info.left = false;
        } break;
        case WM_RBUTTONUP: {
            readInput = true;
            info.right = false;
        } break;
        case WM_MBUTTONUP:
        case WM_XBUTTONUP:
            break;
        // DOWN
        case WM_LBUTTONDOWN: {
            readInput = true;
            info.left = true;
        } break;
        case WM_RBUTTONDOWN: {
            readInput = true;
            info.right = true;
        } break;
        case WM_MBUTTONDOWN:
        case WM_XBUTTONDOWN:
            break;
        // DOUBLE CLICK
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_RBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
            break;
    }

    return readInput;
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

            {  // Update timer related values.
                double now = timer.get();
                elapsedTime_ = now - currentTime_;
                currentTime_ = now;
                normalizedElapsedTime_ = elapsedTime_ / NORMALIZED_ELAPSED_TIME_FACTOR;
            }

            update();
            addGameTime();

            presentBackBuffer();
        }

        if (limitFramerate) {
            // TODO: this is crude and inaccurate.
            DWORD Hz = static_cast<DWORD>(1000 / framesPerSecondLimit);  // 30Hz
            if (settings_.enableDirectoryListener) asyncAlert(Hz);
        } else {
            if (settings_.enableDirectoryListener) asyncAlert(0);
        }

        inputHandler().reset();
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