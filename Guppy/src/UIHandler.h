#ifndef UI_HANDLER_H
#define UI_HANDLER_H

#include "Game.h"
#include "RenderPass.h"

namespace UI {

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame, std::unique_ptr<RenderPass::Base>&& pPass = nullptr);

    // Default behaviour is to do nothing.
    void init() override {}
    virtual void updateRenderPass(RenderPass::FrameInfo* pFrameInfo){};
    virtual void draw(uint8_t frameIndex){};
    void reset() override{};

    inline const std::unique_ptr<RenderPass::Base>& getPass() const { return pPass_; }

   protected:
    std::unique_ptr<RenderPass::Base> pPass_;
};

}  // namespace UI

#endif  // !UI_HANDLER_H
