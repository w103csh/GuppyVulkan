/*
 * Modifications copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
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

#include <Common/Helpers.h>

#include "Game.h"
#include "RenderPass.h"

class Shell;

class Guppy : public Game {
   public:
    Guppy(const std::vector<std::string>& args);
    ~Guppy();

   private:
    // LIFECYCLE
    void attachShell() override;
    void attachSwapchain() override;
    void tick() override;
    void frame() override;
    void detachSwapchain() override;
    void detachShell() override;

    // INPUT
    void processInput();

    bool paused_;

    // TODO: check Hologram for a good starting point for these...
    bool multithread_;
    bool use_push_constants_;
    bool sim_paused_;
    // bool sim_fade_;
    // Simulation sim_;
};

#endif  // !GUPPY_H