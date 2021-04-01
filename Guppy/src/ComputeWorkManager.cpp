/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "ComputeWorkManager.h"

#include <variant>

#include "ConstantsAll.h"
#include "Ocean.h"  // Included for macro OCEAN_USE_COMPUTE_QUEUE_DISPATCH
#include "OceanComputeWork.h"
#include "RenderPassManager.h"
#include "Shell.h"
// HANDLERS
#include "CommandHandler.h"
#include "PipelineHandler.h"

namespace ComputeWork {

const std::set<COMPUTE_WORK> ALL = {
#if OCEAN_USE_COMPUTE_QUEUE_DISPATCH
    COMPUTE_WORK::OCEAN,
#endif
};
const std::vector<COMPUTE_WORK> ACTIVE = {
#if OCEAN_USE_COMPUTE_QUEUE_DISPATCH
    COMPUTE_WORK::OCEAN,
#endif
};

Manager::Manager(Pass::Handler& handler) : Pass::Manager(handler) {
    for (const auto& type : ALL) {
        // clang-format off
        switch (type) {
            case COMPUTE_WORK::OCEAN: pWorkloads_.emplace_back(std::make_unique<Ocean>(handler, static_cast<index>(pWorkloads_.size()))); break;
            default: assert(false && "Unhandled COMPUTE_WORK type"); exit(EXIT_FAILURE);
        }
        // clang-format on
        assert(pWorkloads_.back()->TYPE == type);
        assert(ALL.count(pWorkloads_.back()->TYPE));
    }

    // Initialize offset structures
    for (const auto& type : ACTIVE) {
        bool found = false;
        for (const auto& pWork : pWorkloads_) {
            if (pWork->TYPE == type) {
                // Add the work from ACTIVE to the active pass/offset pairs
                activeTypeOffsetPairs_.insert({pWork->TYPE, pWork->OFFSET});
                found = true;
                break;
            }
        }
        assert(found);
    }
}

void Manager::init() {
    for (auto& [type, offset] : activeTypeOffsetPairs_) pWorkloads_[offset]->onInit();
}

void Manager::tick() {
    for (const auto& [passType, offset] : activeTypeOffsetPairs_) pWorkloads_[offset]->prepare();
}

void Manager::frame() {
    for (const auto& [passType, offset] : activeTypeOffsetPairs_) {
        auto& pWorkload = pWorkloads_[offset];
        pWorkload->record();
        submit(pWorkload->resources.submit);
    }
}

void Manager::reset() {
    for (auto& pWork : pWorkloads_) pWork->onDestroy();
    pWorkloads_.clear();
}

void Manager::getActivePassTypes(const PIPELINE& pipelineTypeIn, std::set<PASS>& types) {
    if (std::visit(Pipeline::IsCompute{}, pipelineTypeIn)) {
        for (const auto& [passType, offset] : activeTypeOffsetPairs_) {
            if (pipelineTypeIn == PIPELINE{COMPUTE::ALL_ENUM}) {
                types.insert(pWorkloads_[offset]->TYPE);
            } else {
                for (const auto& pipelineType : pWorkloads_[offset]->getPipelineTypes())
                    if (pipelineType == pipelineTypeIn) types.insert(pWorkloads_[offset]->TYPE);
            }
        }
    }
}

void Manager::addPipelinePassPairs(pipelinePassSet& set) {
    /* Create pipeline to pass offset map. This way pipeline cache stuff can potentially be used to speed up creation or
     * caching???? I am not going to worry about it other than this atm though.
     *
     * TODO: This was duplicated from elsewhere. Is this necessary?
     */
    for (const auto& [passType, offset] : activeTypeOffsetPairs_) {
        for (const auto [pipelineType, bindDataOffset] : pWorkloads_[offset]->getPipelineBindDataList().getKeyOffsetMap()) {
            set.insert({pipelineType, pWorkloads_[offset]->TYPE});
        }
    }
}

void Manager::submit(const SubmitResource& resource) {
    vk::SubmitInfo info = {};
    info.waitSemaphoreCount = static_cast<uint32_t>(resource.waitSemaphores.size());
    info.pWaitSemaphores = resource.waitSemaphores.data();
    info.pWaitDstStageMask = &resource.waitDstStageMask;
    info.commandBufferCount = static_cast<uint32_t>(resource.commandBuffers.size());
    info.pCommandBuffers = resource.commandBuffers.data();
    info.signalSemaphoreCount = static_cast<uint32_t>(resource.signalSemaphores.size());
    info.pSignalSemaphores = resource.signalSemaphores.data();

    handler().commandHandler().computeQueue().submit({info}, resource.fence);
}

}  // namespace ComputeWork