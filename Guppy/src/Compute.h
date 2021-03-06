/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

//#ifndef COMPUTE_H
//#define COMPUTE_H
//
//#include <list>
//#include <set>
//#include <string>
//#include <vector>
//#include <vulkan/vulkan.hpp>
//
//#include "ConstantsAll.h"
//#include "Handlee.h"
//
//// clang-format off
// namespace RenderPass { class Handler; }
//// clang-format on
//
// namespace Compute {
//
// constexpr uint8_t RESOURCE_SIZE = 10;
//
// using SubmitResource = ::SubmitResource<RESOURCE_SIZE>;
// using SubmitResources = std::array<SubmitResource, RESOURCE_SIZE>;
//
// struct CreateInfo {
//    PIPELINE pipelineType;
//    PASS passType;
//    QUEUE queueType;
//    // Render pass this will come after. Should this be a list?
//    PASS postPassType;
//    // -1 indicates that its frame buffer image size dependent
//    uint32_t groupCountX = UINT32_MAX;
//    uint32_t groupCountY = UINT32_MAX;
//    uint32_t groupCountZ = UINT32_MAX;
//    // -1 indicates that its frame buffer image count dependent
//    int syncCount = -1;
//    bool needsFence = false;
//};
//
// class Handler;
//
//// BASE
//
// class Base : public Handlee<Handler> {
//    friend class Handler;
//
//   public:
//    virtual ~Base() = default;
//
//    void init();
//    void prepare();
//
//    const auto& getBindData(const PASS& passType) const {
//        for (const auto& [passTypes, bindData] : bindDataMap_) {
//            if (passTypes.find(passType) != passTypes.end()) return bindData;
//        }
//        return bindDataMap_.at(Uniform::PASS_ALL_SET);
//    }
//
//    const bool HAS_FENCE;
//    const PIPELINE PIPELINE_TYPE;
//    const PASS PASS_TYPE;
//    const PASS POST_PASS_TYPE;
//    const QUEUE QUEUE_TYPE;
//
//    constexpr const auto& getStatus() { return status_; }
//
//    // PIPELINE
//    void setBindData(const PIPELINE& pipelineType, const std::shared_ptr<Pipeline::BindData>& pBindData);
//
//    // PASS
//    virtual void record(const uint8_t frameIndex, RenderPass::SubmitResource& submitResource);
//
//    // SYNC
//    void createSyncResources();
//    void destroySyncResources();
//
//    void attachSwapchain();
//    void detachSwapchain();
//
//    void destroy();
//
//   protected:
//    Base(Handler& handler, const CreateInfo* pCreateInfo);
//
//    virtual void bindPushConstants(vk::CommandBuffer cmd) {}
//    virtual void preDispatch(const vk::CommandBuffer& cmd, const std::shared_ptr<Pipeline::BindData>& pPplnBindData,
//                             const Descriptor::Set::BindData& descSetBindData, const uint8_t frameIndex) {}
//    virtual void postDispatch(const vk::CommandBuffer& cmd, const std::shared_ptr<Pipeline::BindData>& pPplnBindData,
//                              const Descriptor::Set::BindData& descSetBindData, const uint8_t frameIndex) {}
//
//    uint32_t groupCountX_;
//    uint32_t groupCountY_;
//    uint32_t groupCountZ_;
//
//    std::vector<vk::CommandBuffer> cmds_;
//    BarrierResource barrierResource_;
//
//    // PIPELINE
//    Pipeline::pipelineBindDataList pipelineBindDataList_;  // Is this still necessary? !!!!!
//
//    // DESCRIPTOR SET
//    Descriptor::Set::bindDataMap bindDataMap_;
//
//    // SYNC
//    std::vector<vk::Semaphore> semaphores_;
//
//   private:
//    FlagBits status_;
//
//    bool isFramebufferCountDependent_;
//    bool isFramebufferImageSizeDependent_;
//
//    // SYNC
//    vk::Fence fence_;
//};
//
// namespace PostProcess {
//
// struct PushConstant {
//    FlagBits flags;
//};
//
// extern const CreateInfo DEFAULT_CREATE_INFO;
//
// class Default : public Base {
//   public:
//    Default(Handler& handler);
//
//   protected:
//    void record(const uint8_t frameIndex, RenderPass::SubmitResource& submitResource) override;
//    void preDispatch(const vk::CommandBuffer& cmd, const std::shared_ptr<Pipeline::BindData>& pPplnBindData,
//                     const Descriptor::Set::BindData& descSetBindData, const uint8_t frameIndex) override;
//    void postDispatch(const vk::CommandBuffer& cmd, const std::shared_ptr<Pipeline::BindData>& pPplnBindData,
//                      const Descriptor::Set::BindData& descSetBindData, const uint8_t frameIndex) override;
//};
//
//}  // namespace PostProcess
//
//}  // namespace Compute
//
//#endif  // !COMPUTE_H