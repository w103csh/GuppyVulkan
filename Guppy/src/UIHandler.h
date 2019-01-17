#ifndef UI_HANDLER_H
#define UI_HANDLER_H

#include <vulkan/vulkan.h>

#include "RenderPass.h"
#include "Singleton.h"
#include "Shell.h"

// This is an empty shell that hopefully just does nothing until I figure out what to do.
class DefaultUI : public UI {
   public:
    DefaultUI() : pRenderPass_(nullptr){};

    std::unique_ptr<RenderPass::Base>& getRenderPass() override { return pRenderPass_; }
    void draw(VkCommandBuffer cmd, uint8_t frameIndex) override{};
    void reset() override{};

   private:
    std::unique_ptr<RenderPass::Base> pRenderPass_;
};

class UIHandler : public Singleton {
   public:
    static void init(Shell* sh, const Game::Settings& settings, std::shared_ptr<UI> pUI = nullptr);
    static inline void destroy() { inst_.reset(); };

    static void draw(VkCommandBuffer cmd, uint8_t frameIndex) { inst_.pUI_->draw(cmd, frameIndex); };

   private:
    UIHandler(){};   // Prevent construction
    ~UIHandler(){};  // Prevent construction
    static UIHandler inst_;
    void reset() override { inst_.pUI_->reset(); };

    Shell* sh_;                // TODO: shared_ptr
    Shell::Context ctx_;       // TODO: shared_ptr
    Game::Settings settings_;  // TODO: shared_ptr

    std::shared_ptr<UI> pUI_;
};

#endif  // !UI_HANDLER_H
