/*
 * Modifications copyright (C) 2019 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 * Changed file name from "Hologram.h"
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

#ifndef GUPPY_H
#define GUPPY_H

#include <vector>

#include "Game.h"
#include "Helpers.h"
#include "RenderPass.h"

class Base;
struct MouseInput;
class Shell;

class Guppy : public Game {
   public:
    Guppy(const std::vector<std::string>& args);
    ~Guppy();

    // SHELL
    void attachShell(Shell& sh) override;
    void detachShell() override;

    // SWAPCHAIN
    void attachSwapchain() override;
    void detachSwapchain() override;

    // GAME
    void onTick() override;
    void onFrame(float framePred) override;

    // INPUT
    void onButton(const GameButtonBits buttons) override;
    void onKey(const GAME_KEY key) override;
    void onMouse(const MouseInput& input) override;

   private:
    // TODO: check Hologram for a good starting point for these...
    bool multithread_;
    bool use_push_constants_;
    bool sim_paused_;
    // bool sim_fade_;
    // Simulation sim_;
};

#endif  // !GUPPY_H