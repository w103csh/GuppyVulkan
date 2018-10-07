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
        int initial_width;
        int initial_height;
        int queue_count;
        int back_buffer_count;
        int ticks_per_second;
        bool vsync;
        bool animate;

        bool validate;
        bool validate_verbose;

        bool no_tick;
        bool no_render;
        bool no_present;

        // *
        bool try_sampler_anisotropy; // TODO: Not sure what this does
        bool try_sample_rate_shading;
        bool enable_sample_shading;
        bool include_color;
        bool include_depth;
        bool enable_double_clicks;
    };
    const Settings &settings() const { return settings_; }

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

    virtual void on_frame(float frame_pred) {}

   protected:
    Game(const std::string &name, const std::vector<std::string> &args) : settings_(), shell_(nullptr) {
        settings_.name = name;
        settings_.initial_width = 1280;
        settings_.initial_height = 1024;
        settings_.queue_count = 1;
        settings_.back_buffer_count = 3;
        settings_.ticks_per_second = 30;
        settings_.vsync = true;
        settings_.animate = true;

        settings_.validate = false;
        settings_.validate_verbose = false;

        settings_.no_tick = false;
        settings_.no_render = false;
        settings_.no_present = false;

        // *
        settings_.try_sampler_anisotropy = true;
        settings_.try_sample_rate_shading = true;
        settings_.include_color = true;
        settings_.include_depth = true;
        settings_.enable_double_clicks = false;

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
            }
        }
    }
};

#endif  // GAME_H
