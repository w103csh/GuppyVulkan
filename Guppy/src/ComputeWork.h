/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef COMPUTE_WORK_H
#define COMPUTE_WORK_H

#include <list>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "ConstantsAll.h"
#include "Handlee.h"

// clang-format off
namespace Pass       { class Handler; }
namespace Descriptor { class Base; }
// clang-format on

namespace ComputeWork {

struct CreateInfo {
    COMPUTE_WORK type;
    std::string name;
    std::list<PIPELINE> pipelineTypes;
};

class Base : public Handlee<Pass::Handler> {
   public:
    const std::string NAME;
    const index OFFSET;
    const COMPUTE_WORK TYPE;

    Base(Pass::Handler& handler, const index&& offset, const CreateInfo* pCreateInfo);

    void onInit() { init(); }
    void onRecord();
    void destroy();

    virtual void prepare() { status_ = STATUS::READY; }
    virtual void record() = 0;
    virtual const std::vector<Descriptor::Base*> getDynamicDataItems(const PIPELINE pipelineType) const { return {}; }

    // DESCRIPTOR SET
    constexpr const auto& getDescSetBindDataMaps() const { return descSetBindDataMaps_; }
    void setDescSetBindData();
    const Descriptor::Set::BindData& getDescSetBindData(const PASS& passType, const uint32_t index) const;

    // PIPELINES
    void setBindData(const PIPELINE& pipelineType, const std::shared_ptr<Pipeline::BindData>& pBindData);
    constexpr const auto& getPipelineBindDataList() const { return pipelineBindDataList_; }
    constexpr const auto& getPipelineData() const { return pipelineData_; }
    inline std::set<PIPELINE> getPipelineTypes() {
        std::set<PIPELINE> types;
        for (const auto& keyValue : pipelineBindDataList_.getKeyOffsetMap()) types.insert(keyValue.first);
        return types;
    }
    inline void addPipelineTypes(std::set<PIPELINE>& pipelineTypes) const {
        for (const auto& keyValue : pipelineBindDataList_.getKeyOffsetMap()) pipelineTypes.insert(keyValue.first);
    }

    // RESOURCES
    struct Resources {
        std::vector<vk::CommandBuffer> cmds;
        std::vector<vk::Semaphore> semaphores;
        std::vector<vk::Fence> fences;
        SubmitResource submit;
    } resources;

   protected:
    virtual void init() {}

    void createCommandBuffers(const uint32_t n);
    void createSemaphores(const uint32_t n);
    void createFences(const uint32_t n);

    FlagBits status_;

   private:
    std::vector<Descriptor::Set::bindDataMap> descSetBindDataMaps_;

    // PIPELINE
    Pass::PipelineData pipelineData_;
    Pipeline::pipelineBindDataList pipelineBindDataList_;
};

}  // namespace ComputeWork

#endif  // !COMPUTE_H