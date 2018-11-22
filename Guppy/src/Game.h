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

#ifndef GAME_H
#define GAME_H

#include <memory>
#include <string>
#include <vector>

class MyShell;

class Game {
   public:
    Game(const Game &game) = delete;
    Game &operator=(const Game &game) = delete;
    virtual ~Game() {}

    struct Settings {
        std::string name;
        int initial_width = 1280;
        int initial_height = 1024;
        int queue_count = 1;
        int back_buffer_count = 3;
        int ticks_per_second = 30;
        bool vsync = true;
        bool animate = true;

        bool validate = false;
        bool validate_verbose = false;

        bool no_tick = false;
        bool no_render = false;
        bool no_present = false;

        // *
        bool try_sampler_anisotropy = true;  // TODO: Not sure what this does
        bool try_sample_rate_shading = true;
        bool enable_sample_shading = true;
        bool include_color = true;
        bool include_depth = true;
        bool enable_double_clicks = false;
        bool enable_debug_markers = false;
        bool enable_directory_listener = true;
        bool assert_on_recompile_shader = false;
    };
    const Settings &settings() const { return settings_; }
    MyShell &shell() const { return (*shell_); }  // TODO: maybe don't do this??? (Used for listening to shader changes)

    virtual void attach_shell(MyShell &shell) { shell_ = &shell; }
    virtual void detach_shell() { shell_ = nullptr; }

    virtual void attach_swapchain() {}
    virtual void detach_swapchain() {}

    enum class KEY {
        // virtual keys
        KEY_SHUTDOWN,
        // physical keys
        KEY_UNKNOWN,
        KEY_ESC,
        KEY_UP,
        KEY_DOWN,
        KEY_LEFT,
        KEY_RIGHT,
        KEY_SPACE,
        KEY_TAB,
        KEY_F,
        KEY_W,
        KEY_A,
        KEY_S,
        KEY_D,
        KEY_E,
        KEY_Q,
        // number keys
        KEY_1,
        KEY_2,
        KEY_3,
        KEY_4,
        KEY_5,
        KEY_6,
        KEY_7,
        KEY_8,
        KEY_9,
        KEY_0,
        KEY_PLUS,
        KEY_MINUS,
        // function keys
        KEY_F1,
        KEY_F2,
        KEY_F3,
        KEY_F4,
        KEY_F5,
        KEY_F6,
        KEY_F7,
        KEY_F8,
        KEY_F9,
        KEY_F10,
        KEY_F11,
        KEY_F12,
        // modifiers
        KEY_ALT,
        KEY_CTRL,
        KEY_SHFT,
    };

    enum class MOUSE {
        LEFT,
        RIGHT,
        MIDDLE,
        X,
    };

    virtual void on_key(KEY key) {}
    virtual void on_tick() {}

    virtual void on_frame(float framePred) {}

   protected:
    Game(const std::string &name, const std::vector<std::string> &args) : shell_(nullptr), settings_() {
        settings_.name = name;
        parse_args(args);
    }

    Settings settings_;
    MyShell *shell_;

   private:
    void parse_args(const std::vector<std::string> &args) {
        for (auto it = args.begin(); it != args.end(); ++it) {
            if (*it == "-b") {
                settings_.vsync = false;
            } else if (*it == "-w") {
                ++it;
                settings_.initial_width = std::stoi(*it);
            } else if (*it == "-h") {
                ++it;
                settings_.initial_height = std::stoi(*it);
            } else if ((*it == "-v") || (*it == "--validate")) {
                settings_.validate = true;
            } else if (*it == "-vv") {
                settings_.validate = true;
                settings_.validate_verbose = true;
            } else if (*it == "-nt") {
                settings_.no_tick = true;
            } else if (*it == "-nr") {
                settings_.no_render = true;
            } else if (*it == "-np") {
                settings_.no_present = true;
            } else if (*it == "-dbgm") {
                settings_.enable_debug_markers = true;
            }
        }
    }
};

#endif  // GAME_H
