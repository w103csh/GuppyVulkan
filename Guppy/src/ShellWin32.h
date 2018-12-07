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

#ifndef SHELL_WIN32_H
#define SHELL_WIN32_H

#include <condition_variable>
#include <mutex>
#include <windows.h>

#include "InputHandler.h"
#include "MyShell.h"
#include "ShellGLFW.h"

#define BUFSIZE 8

typedef struct _DIR_INST {
    OVERLAPPED oOverlap;
    HANDLE hDir;
    FILE_NOTIFY_INFORMATION lpBuffer[BUFSIZE];
    LPDWORD lpBytesReturned;
    BOOL firstComp;
    std::function<void(std::string)> callback;
} DIR_INST, *LPDIR_INST;

class ShellWin32 : public MyShell {
   public:
    ShellWin32(Game &game);
    ~ShellWin32();

    void watchDirectory(std::string dir, std::function<void(std::string)> callback) override;
    void run() override;
    void quit() override;

   protected:
    virtual void setPlatformSpecificExtensions() override;
    PFN_vkGetInstanceProcAddr load_vk();

   private:
    VkBool32 canPresent(VkPhysicalDevice phy, uint32_t queue_family);

    void createWindow() override;
    VkSurfaceKHR createSurface(VkInstance instance);

    Game::KEY getKey(WPARAM wParam, InputHandler::INPUT_TYPE type);
    void getMouse(Game::MOUSE mouse, UINT uMsg, LPARAM lParam);
    void getMouseModifier(WPARAM wParam, LPARAM lParam);

    static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        ShellWin32 *shell = reinterpret_cast<ShellWin32 *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        // called from constructor, CreateWindowEx specifically.  But why?
        if (!shell) return DefWindowProc(hwnd, uMsg, wParam, lParam);

        return shell->handleMessage(uMsg, wParam, lParam);
    }
    LRESULT handleMessage(UINT msg, WPARAM wparam, LPARAM lparam);

    // Directory listener
    void AsyncAlert(DWORD);
    std::string GetWorkingDirectory();
    std::string GetLastErrorAsString();
    void CheckDirectories();
    std::vector<DIR_INST> dirInsts_;

    HINSTANCE hinstance_;
    HWND hwnd_;

    HMODULE hmodule_;

    bool minimized_;
    // std::mutex mtx_;
    // std::condition_variable pause_;
};

#endif  // SHELL_WIN32_H
