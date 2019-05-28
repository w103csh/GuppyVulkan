#ifndef UI_HANDLER_H
#define UI_HANDLER_H

#include <memory>
#include <vulkan/vulkan.h>

#include "Game.h"
#include "RenderPass.h"

namespace UI {

class Handler : public Game::Handler {
   public:
    UI::Handler::Handler(Game* pGame) : Game::Handler(pGame) {}

    // Default behaviour is to do nothing.
    virtual void init() override {}
    virtual void reset() override {}
    virtual void destroy() override {}
    virtual void update() {}
    virtual void draw(const VkCommandBuffer& cmd, const uint8_t& frameIndex) {}
};

}  // namespace UI

#endif  // !UI_HANDLER_H
