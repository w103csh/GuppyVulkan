#ifndef LDG_RESOURCE_HANDLER_H
#define LDG_RESOURCE_HANDLER_H

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

#include "Game.h"
#include "Helpers.h"
#include "Types.h"

namespace Loading {

class Handler : public Game::Handler {
   public:
    Handler(Game *pGame);

    void init() override;
    void tick() override;
    inline void destroy() override { tick(); };

    std::unique_ptr<LoadingResource> createLoadingResources() const;
    void loadSubmit(std::unique_ptr<LoadingResource> pLdgRes);

    void getFences(std::vector<VkFence> &fences);

   private:
    void reset() override{};
    bool destroyResource(LoadingResource &resource) const;

    std::vector<std::unique_ptr<LoadingResource>> ldgResources_;
};

}  // namespace Loading

#endif  // !LDG_RESOURCE_HANDLER_H
