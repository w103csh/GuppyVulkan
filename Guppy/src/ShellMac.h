/*
 */

#ifndef SHELL_MAC
#define SHELL_MAC

#include <CoreServices/CoreServices.h>

#include "InputHandler.h"
#include "Shell.h"
#include "ShellGLFW.h"

class ShellMac : public Shell {
   public:
    ShellMac(Game &game);
    ~ShellMac();

    void run() override;
    void quit() const override;

    // SHADER RECOMPILING
    void asyncAlert(uint64_t milliseconds) override;  // TODO: Why is the necessay??
    void checkDirectories() override {}; // TODO: Why is the necessay??
    void watchDirectory(const std::string &directory, std::function<void(std::string)> callback) override;

   protected:
    virtual void setPlatformSpecificExtensions() override;
    PFN_vkGetInstanceProcAddr load_vk() override;
    void destroy_context() override;

   private:
    VkBool32 canPresent(VkPhysicalDevice phy, uint32_t queue_family) override;

    void createWindow() override;
    // VkSurfaceKHR createSurface(VkInstance instance);

    // GAME_KEY getKey(WPARAM wParam, INPUT_ACTION type);
    // void getMouse(UINT uMsg, LPARAM lParam);
    // void getMouseModifier(WPARAM wParam, LPARAM lParam);

    FSEventStreamRef watchDirStreamRef;
};

#endif  // SHELL_MAC
