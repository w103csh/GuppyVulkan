#ifndef LDG_RESOURCE_HANDLER_H
#define LDG_RESOURCE_HANDLER_H

#include <vector>
#include <vulkan/vulkan.h>

#include "Helpers.h"
#include "MyShell.h"
#include "Singleton.h"

struct LoadingResources {
    LoadingResources() : shouldWait(false), graphicsCmd(VK_NULL_HANDLE), transferCmd(VK_NULL_HANDLE), semaphore(VK_NULL_HANDLE){};
    bool shouldWait;
    VkCommandBuffer graphicsCmd, transferCmd;
    std::vector<BufferResource> stgResources;
    std::vector<VkFence> fences;
    VkSemaphore semaphore;
    bool cleanup(const VkDevice &dev);
};

class LoadingResourceHandler : Singleton {
   public:
    static void init(const MyShell::Context &ctx);

    static std::unique_ptr<LoadingResources> createLoadingResources();
    static void loadSubmit(std::unique_ptr<LoadingResources> pLdgRes);
    static void cleanupResources();

   private:
    LoadingResourceHandler(){};   // Prevent construction
    ~LoadingResourceHandler(){};  // Prevent construction

    static LoadingResourceHandler inst_;

    void reset() override{};

    MyShell::Context ctx_;  // TODO: shared_ptr

    std::vector<std::unique_ptr<LoadingResources>> ldgResources_;
};

#endif  // !LDG_RESOURCE_HANDLER_H
