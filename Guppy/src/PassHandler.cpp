/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "PassHandler.h"

#include "ComputeWorkManager.h"
#include "RenderPassManager.h"
// HANDLER
#include "PipelineHandler.h"

namespace Pass {

// MANAGER
Manager::Manager(Handler& handler) : Handlee(handler) {}

// HANDLER
Handler::Handler(Game* pGame)  //
    : Game::Handler(pGame),
      pRenderPassMgr_(std::make_unique<RenderPass::Manager>(*this)),
      pCompWorkMgr_(std::make_unique<ComputeWork::Manager>(*this)) {}

void Handler::init() {
    pRenderPassMgr_->init();
    pCompWorkMgr_->init();
}

void Handler::tick() {
    pCompWorkMgr_->tick();
    // TODO: Move render pass status work here?
}

void Handler::frame() {
    pCompWorkMgr_->frame();
    pRenderPassMgr_->frame();
}

void Handler::destroy() {
    pRenderPassMgr_->destroy();
    pCompWorkMgr_->destroy();
}

void Handler::getActivePassTypes(std::set<PASS>& types, const PIPELINE& pipelineTypeIn) {
    pRenderPassMgr_->getActivePassTypes(pipelineTypeIn, types);
    pCompWorkMgr_->getActivePassTypes(pipelineTypeIn, types);
}

void Handler::addPipelineTypes(const PASS passType, std::set<PIPELINE>& pipelineTypes) const {
    if (std::visit(IsRender{}, passType)) {
        const auto type = std::visit(GetRender{}, passType);
        pRenderPassMgr_->getPass(type)->addPipelineTypes(pipelineTypes);
    } else if (std::visit(IsCompute{}, passType)) {
        const auto type = std::visit(GetCompute{}, passType);
        pCompWorkMgr_->getWork(type)->addPipelineTypes(pipelineTypes);
    } else {
        assert(false && "Unknown PASS type");
        exit(EXIT_FAILURE);
    }
}

bool Handler::comparePipelineData(const PASS type, const PASS testType) const {
    const Pass::PipelineData* pData = nullptr;
    if (std::visit(IsRender{}, type)) {
        const auto t = std::visit(GetRender{}, type);
        pData = &pRenderPassMgr_->getPass(t)->getPipelineData();
    } else if (std::visit(IsCompute{}, type)) {
        const auto t = std::visit(GetCompute{}, type);
        pData = &pCompWorkMgr_->getWork(t)->getPipelineData();
    } else {
        assert(false && "Unknown PASS type");
        exit(EXIT_FAILURE);
    }

    const Pass::PipelineData* pTestData = nullptr;
    if (std::visit(IsRender{}, testType)) {
        const auto t = std::visit(GetRender{}, testType);
        pData = &pRenderPassMgr_->getPass(t)->getPipelineData();
    } else if (std::visit(IsCompute{}, testType)) {
        const auto t = std::visit(GetCompute{}, testType);
        pData = &pCompWorkMgr_->getWork(t)->getPipelineData();
    } else {
        assert(false && "Unknown PASS type");
        exit(EXIT_FAILURE);
    }

    if (pData != nullptr || pTestData != nullptr) {
        assert(false);
        return false;
    } else {
        return ((*pData) == (*pTestData));
    }
}

void Handler::addPipelinePassPairs(pipelinePassSet& set) {
    pRenderPassMgr_->addPipelinePassPairs(set);
    pCompWorkMgr_->addPipelinePassPairs(set);
}

void Handler::updateBindData(const pipelinePassSet& set) {
    const auto& bindDataMap = pipelineHandler().getPipelineBindDataMap();
    // Update pipeline bind data for all passes.
    for (const auto& pair : set) {
        if (std::visit(Pass::IsRender{}, pair.second)) {
            const auto type = std::visit(Pass::GetRender{}, pair.second);
            pRenderPassMgr_->getPass(type)->setBindData(pair.first, bindDataMap.at(pair));
        } else if (std::visit(Pass::IsCompute{}, pair.second)) {
            const auto type = std::visit(Pass::GetCompute{}, pair.second);
            pCompWorkMgr_->getWork(type)->setBindData(pair.first, bindDataMap.at(pair));
        } else {
            assert(false && "Unknown PASS type");
            exit(EXIT_FAILURE);
        }
    }
}

void Handler::reset() {
    pRenderPassMgr_->reset();
    pCompWorkMgr_->reset();
}

}  // namespace Pass