#ifndef COMPUTE_HANDLER_H
#define COMPUTE_HANDLER_H

#include <memory>
#include <map>
#include <set>

#include "ConstantsAll.h"
#include "Compute.h"
#include "Game.h"

namespace Compute {

extern const std::set<PASS> ALL;
extern const std::vector<PASS> ACTIVE;

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame);

    void init() override;
    inline void destroy() override { reset(); }

    void update();

    // NOTE: this is not in order!!!
    void getActivePassTypes(std::set<PASS>& types, const PIPELINE& pipelineTypeIn = PIPELINE::ALL_ENUM);
    bool isActiveAndReady(const PASS& type) { return pComputeMap_.count(type) > 0; }

    void attachSwapchain();
    void detachSwapchain();

    // PIPELINE
    void addPipelinePassPairs(pipelinePassSet& set);
    void updateBindData(const pipelinePassSet& set);

    // PASS
    bool recordPasses(const PASS& renderPassType, RenderPass::SubmitResource& submitResource);

   private:
    void reset() override;

    // SYNC
    std::vector<VkFence> passFences_;

    // SUBMIT (This is not used atm)
    void submitResources(std::vector<const SubmitResource*>& pResources);
    std::vector<VkSubmitInfo> submitInfos_;

    /* Everything in pComputeMap_ gets submitted per frame. If you need to submit compute
     *  commands at a different rate then you should make another map, so that the logic doesn't
     *  get to complicated searching for things that need to be submitted.
     */
    std::map<PASS, std::unique_ptr<Compute::Base>> pComputeMap_;
    std::map<PASS, std::unique_ptr<Compute::Base>> pComputePendingMap_;
};

}  // namespace Compute

#endif  // !COMPUTE_HANDLER_H