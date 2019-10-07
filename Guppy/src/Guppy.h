
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
    void onKey(GAME_KEY key) override;
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