/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef COMPUTE_WORK_MANAGER_H
#define COMPUTE_WORK_MANAGER_H

#include <memory>
#include <set>
#include <vector>
#include <vulkan/vulkan.hpp>
#include <utility>

#include "ComputeWorkConstants.h"
#include "ComputeWork.h"
#include "PassHandler.h"

namespace ComputeWork {

class Manager : public Pass::Manager {
   public:
    Manager(Pass::Handler& handler);

    void init() override;
    void tick();
    void frame() override;
    inline void destroy() override { reset(); }
    void reset() override;

    void getActivePassTypes(const PIPELINE& pipelineTypeIn, std::set<PASS>& types);

    inline const auto& getWork(const COMPUTE_WORK& type) {
        for (const auto& pWork : pWorkloads_)
            if (pWork->TYPE == type) return pWork;
        assert(false);
        exit(EXIT_FAILURE);
    }

    // PIPELINE
    void addPipelinePassPairs(pipelinePassSet& set);

   private:
    void submit(const SubmitResource& resource);

    std::vector<std::unique_ptr<Base>> pWorkloads_;
    std::set<std::pair<COMPUTE_WORK, index>> activeTypeOffsetPairs_;
};

}  // namespace ComputeWork

#endif  // !COMPUTE_WORK_MANAGER_H