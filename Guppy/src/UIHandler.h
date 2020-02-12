/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef UI_HANDLER_H
#define UI_HANDLER_H

#include <memory>
#include <vulkan/vulkan.hpp>

#include "Game.h"
#include "RenderPass.h"

namespace UI {

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame) : Game::Handler(pGame) {}

    // Default behaviour is to do nothing.
    virtual void init() override {}
    virtual void reset() override {}
    virtual void destroy() override {}
    virtual void draw(const vk::CommandBuffer& cmd, const uint8_t frameIndex) {}
};

}  // namespace UI

#endif  // !UI_HANDLER_H
