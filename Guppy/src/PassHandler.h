/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef PASS_HANDLER_H
#define PASS_HANDLER_H

#include <memory>
#include <set>

#include "ConstantsAll.h"
#include "Game.h"
#include "Handlee.h"

// clang-format off
namespace RenderPass  { class Manager; }
namespace ComputeWork { class Manager; }
// clang-format on

namespace Pass {

class Handler;

class Manager : public Handlee<Handler> {
   public:
    Manager(Handler& handler);
    virtual ~Manager() = default;

    virtual void init() = 0;
    virtual void frame() = 0;
    virtual void reset() = 0;
    virtual void destroy() = 0;
};

class Handler : public Game::Handler {
   public:
    Handler(Game* pGame);

    void init() override;
    void tick() override;
    void frame() override;
    void destroy() override;

    // NOTE: this is not in order!!!
    void getActivePassTypes(std::set<PASS>& types, const PIPELINE& pipelineTypeIn = GRAPHICS::ALL_ENUM);
    void addPipelineTypes(const PASS passType, std::set<PIPELINE>& pipelineTypes) const;
    bool comparePipelineData(const PASS type, const PASS testType) const;

    void addPipelinePassPairs(pipelinePassSet& set);
    void updateBindData(const pipelinePassSet& set);

    auto& renderPassMgr() { return (*pRenderPassMgr_); }
    auto& compWorkMgr() { return (*pCompWorkMgr_); }

   private:
    void reset() override;

    std::unique_ptr<RenderPass::Manager> pRenderPassMgr_;
    std::unique_ptr<ComputeWork::Manager> pCompWorkMgr_;
};

}  // namespace Pass

#endif  // !PASS_HANDLER_H
