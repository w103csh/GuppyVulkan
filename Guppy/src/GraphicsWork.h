/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef GRAPHICS_WORK_H
#define GRAPHICS_WORK_H

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "ConstantsAll.h"
#include "Handlee.h"

// clang-format off
namespace Descriptor { class Base; }
namespace Material   { class Base; }
namespace Pass       { class Handler; }
// clang-format on

namespace GraphicsWork {

struct CreateInfo {
    std::string name = "";
    std::set<PIPELINE> pipelineTypes;
    std::vector<Descriptor::Base*> pDynDescs;
};

class Base : public Handlee<Pass::Handler> {
   public:
    const std::string NAME;
    const std::set<PIPELINE> PIPELINE_TYPES;

    Base(Pass::Handler& handler, const CreateInfo* pCreateInfo);

    void onInit() { init(); }
    void onDestroy();
    void onTick();

    constexpr auto getDraw() const { return draw_; }
    constexpr void toggleDraw() { draw_ = !draw_; }
    bool shouldDraw(const PIPELINE type) const {  //
        return draw_ && (status_ == STATUS::READY) && PIPELINE{drawMode} == type;
    }
    GRAPHICS drawMode;

    virtual void record(const PASS passType, const std::shared_ptr<Pipeline::BindData>& pPipelineBindData,
                        const vk::CommandBuffer& cmd) = 0;

    const Descriptor::Set::BindData& getDescriptorSetBindData(const PIPELINE type);

   protected:
    virtual void init() {}
    virtual void destroy() {}
    virtual void tick() {}
    virtual void frame() {}
    virtual void load(std::unique_ptr<LoadingResource>& pLdgRes) {}

    FlagBits status_;
    bool draw_;

    std::vector<Descriptor::Base*> pDynDescs_;
    std::map<PIPELINE, Descriptor::Set::bindDataMap> pplnDescSetBindDataMap_;
};
}  // namespace GraphicsWork

#endif  //! GRAPHICS_WORK_H