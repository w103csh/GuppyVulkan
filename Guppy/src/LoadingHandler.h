#ifndef LDG_RESOURCE_HANDLER_H
#define LDG_RESOURCE_HANDLER_H

#include <vector>
#include <vulkan/vulkan.h>

#include "Game.h"
#include "Helpers.h"

namespace Loading {

struct Resources {
    Resources()
        :  //
          shouldWait(false),
          graphicsCmd(VK_NULL_HANDLE),
          transferCmd(VK_NULL_HANDLE),
          semaphore(VK_NULL_HANDLE){};
    bool shouldWait;
    VkCommandBuffer graphicsCmd, transferCmd;
    std::vector<BufferResource> stgResources;
    std::vector<VkFence> fences;
    VkSemaphore semaphore;
};

class Handler : public Game::Handler {
   public:
    Handler(Game *pGame);

    void init() override;
    inline void destroy() override { cleanup(); };

    std::unique_ptr<Loading::Resources> createLoadingResources() const;
    void loadSubmit(std::unique_ptr<Loading::Resources> pLdgRes);
    void cleanup();

   private:
    void reset() override{};
    bool destroyResource(Loading::Resources &resource) const;

    std::vector<std::unique_ptr<Loading::Resources>> ldgResources_;
};

}  // namespace Loading

#endif  // !LDG_RESOURCE_HANDLER_H
