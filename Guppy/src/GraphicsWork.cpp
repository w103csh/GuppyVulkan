/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#include "GraphicsWork.h"

#include "Descriptor.h"
#include "Material.h"
// HANDLERS
#include "LoadingHandler.h"
#include "PassHandler.h"
#include "DescriptorHandler.h"

namespace GraphicsWork {

Base::Base(Pass::Handler& handler, const CreateInfo* pCreateInfo)
    : Handlee(handler),
      NAME(pCreateInfo->name),
      PIPELINE_TYPES(pCreateInfo->pipelineTypes),
      drawMode(GRAPHICS::ALL_ENUM),
      status_(STATUS::PENDING_BIND_DATA),
      draw_(false),
      pDynDescs_(pCreateInfo->pDynDescs) {
    assert(PIPELINE_TYPES.size());
}

void Base::onDestroy() {
    destroy();
    drawMode = GRAPHICS::ALL_ENUM;
    status_ = STATUS::UNKNOWN;
    pDynDescs_.clear();
}

void Base::onTick() {
    tick();  // Tick before? Tick after?

    if ((status_ & STATUS::PENDING_BIND_DATA)) {
        for (const auto& pipelineType : PIPELINE_TYPES) {
            auto it = pplnDescSetBindDataMap_.find(pipelineType);
            if (it == pplnDescSetBindDataMap_.end()) {
                auto insertPair = pplnDescSetBindDataMap_.insert({pipelineType, {}});
                assert(insertPair.second);
                it = insertPair.first;
            }
            handler().descriptorHandler().getBindData(pipelineType, it->second, pDynDescs_);
        }
        status_ ^= STATUS::PENDING_BIND_DATA;
    }

    if ((status_ & STATUS::PENDING_BUFFERS)) {
        auto pLdgRes = handler().loadingHandler().createLoadingResources();
        load(pLdgRes);
        handler().loadingHandler().loadSubmit(std::move(std::move(pLdgRes)));
        status_ ^= STATUS::PENDING_BUFFERS;
    }
}

const Descriptor::Set::BindData& Base::getDescriptorSetBindData(const PIPELINE type) {
    const auto& descSetBindDataMap = pplnDescSetBindDataMap_.at(type);
    // I am curious if this will ever hit, so just putting an assert for now and skipping the loop.
    assert(descSetBindDataMap.size() == 1);
    return descSetBindDataMap.begin()->second;
    // for (const auto& [passTypes, bindData] : descSetBindDataMap) {
    //    if (passTypes.find(passType) != passTypes.end()) return bindData;
    //}
    // return descSetBindDataMap.at(Uniform::PASS_ALL_SET);
}

}  // namespace GraphicsWork
