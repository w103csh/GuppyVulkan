#ifndef UI_HANDLER_H
#define UI_HANDLER_H

#include <vulkan/vulkan.h>

#include "Singleton.h"

class Shell;

class UI {
   public:
    virtual void draw(VkCommandBuffer cmd, uint8_t frameIndex) = 0;
    virtual void reset() = 0;
};

class DefaultUI : public UI {
    void draw(VkCommandBuffer cmd, uint8_t frameIndex) override{};
    void reset() override{};
};

class UIHandler : Singleton {
   public:
    static void init(Shell* sh, const Game::Settings& settings, std::unique_ptr<UI> ui = std::make_unique<DefaultUI>());
    static inline void destroy() { inst_.reset(); }

    static void draw(VkCommandBuffer cmd, uint8_t frameIndex) { inst_.ui_->draw(cmd, frameIndex); };

   private:
    UIHandler();     // Prevent construction
    ~UIHandler(){};  // Prevent construction
    static UIHandler inst_;
    void reset() override { inst_.ui_->reset(); };

    Shell* sh_;
    Shell::Context ctx_;       // TODO: shared_ptr
    Game::Settings settings_;  // TODO: shared_ptr

    std::unique_ptr<UI> ui_;
};

#endif  // !UI_HANDLER_H
