/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "ComputeWork.h"

#include <Common/Helpers.h>

#include "ComputeWorkManager.h"
// HANDLERS
#include "DescriptorHandler.h"
#include "CommandHandler.h"
#include "PassHandler.h"

namespace ComputeWork {

Base::Base(Pass::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo)  //
    : Handlee(handler),
      NAME(pCreateInfo->name),
      OFFSET(offset),
      TYPE(pCreateInfo->type),
      status_(STATUS::UNKNOWN),
      pipelineData_{} {
    for (const auto& pipelineType : pCreateInfo->pipelineTypes) {
        // If changed from unique list (set) then a lot of work needs to be done, so I am
        // adding some validation here.
        assert(pipelineBindDataList_.getOffset(pipelineType) == -1);
        pipelineBindDataList_.insert(pipelineType, nullptr);
    }
}

void Base::onRecord() {
    record();
    // There should be at least one command buffer.
    assert(resources.submit.commandBuffers.size());
}

void Base::setDescSetBindData() {
    assert(descSetBindDataMaps_.empty() && pipelineBindDataList_.getKeys().size());
    for (const auto& pipelineType : pipelineBindDataList_.getKeys()) {
        descSetBindDataMaps_.emplace_back();
        handler().descriptorHandler().getBindData(pipelineType, descSetBindDataMaps_.back(),
                                                  getDynamicDataItems(pipelineType));
    }
}

const Descriptor::Set::BindData& Base::getDescSetBindData(const PASS& passType, const uint32_t index) const {
    for (const auto& [passTypes, bindData] : descSetBindDataMaps_[index]) {
        if (passTypes.find(passType) != passTypes.end()) return bindData;
    }
    return descSetBindDataMaps_[index].at(Uniform::PASS_ALL_SET);
}

void Base::setBindData(const PIPELINE& pipelineType, const std::shared_ptr<Pipeline::BindData>& pBindData) {
    pipelineBindDataList_.insert(pipelineType, pBindData);
}

void Base::createCommandBuffers(const uint32_t n) {
    assert(resources.cmds.empty());
    resources.cmds.assign(n, nullptr);
    handler().commandHandler().createCmdBuffers(QUEUE::COMPUTE, resources.cmds.data(), vk::CommandBufferLevel::ePrimary,
                                                resources.cmds.size());
}

void Base::createSemaphores(const uint32_t n) {
    const auto& ctx = handler().shell().context();
    assert(resources.semaphores.empty());
    for (uint32_t i = 0; i < n; i++) {
        resources.semaphores.push_back(ctx.dev.createSemaphore({}, ctx.pAllocator));
    }
}

void Base::createFences(const uint32_t n) {
    const auto& ctx = handler().shell().context();
    assert(resources.fences.empty());
    for (uint32_t i = 0; i < n; i++) {
        resources.fences.push_back(ctx.dev.createFence({vk::FenceCreateFlagBits::eSignaled}, ctx.pAllocator));
    }
}

void Base::destroy() {
    const auto& ctx = handler().shell().context();
    // CMDS
    ctx.dev.freeCommandBuffers(handler().commandHandler().computeCmdPool(), resources.cmds);
    resources.cmds.clear();
    // SEMAPHORES
    for (const auto& s : resources.semaphores) ctx.dev.destroy(s, ctx.pAllocator);
    resources.semaphores.clear();
    // FENCES
    auto result = ctx.dev.waitForFences(resources.fences, VK_TRUE, UINT64_MAX);
    assert(result == vk::Result::eSuccess);
    for (const auto& f : resources.fences) ctx.dev.destroy(f, ctx.pAllocator);
    resources.fences.clear();
    // MISC.
    descSetBindDataMaps_.clear();
    pipelineData_ = {};
    pipelineBindDataList_.clear();
}

}  // namespace ComputeWork