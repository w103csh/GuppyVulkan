#ifndef PIPELINE_HANDLER_H
#define PIPELINE_HANDLER_H

#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

#include "Constants.h"
#include "Game.h"
#include "Material.h"
#include "Mesh.h"
#include "Pipeline.h"
#include "Vertex.h"

namespace Pipeline {

class Handler : public Game::Handler {
    friend class Base;

   public:
    Handler(Game *pGame);

    void init() override;
    inline void destroy() override {
        reset();
        cleanup();
    }

    // CACHE
    inline const VkPipelineCache &getPipelineCache() { return cache_; }

    // PUSH CONSTANT
    std::vector<VkPushConstantRange> getPushConstantRanges(const PIPELINE &pipelineType,
                                                           const std::vector<PUSH_CONSTANT> &pushConstantTypes) const;

    // PIPELINES
    const std::vector<Pipeline::Reference> createPipelines(
        const std::multiset<std::pair<PIPELINE, RenderPass::offset>> &set);
    void createPipeline(const std::string &&name, VkGraphicsPipelineCreateInfo &createInfo, VkPipeline &pipeline);

    // DESCRIPTOR SET
    VkShaderStageFlags getDescriptorSetStages(const DESCRIPTOR_SET &setType);
    void initShaderInfoMap(Shader::shaderInfoMap &map);

    inline const std::unique_ptr<Pipeline::Base> &getPipeline(const PIPELINE &pipelineType) const {
        // Make sure the pipeline that is being accessed is available.
        assert(static_cast<int>(pipelineType) > static_cast<int>(PIPELINE::DONT_CARE) &&
               static_cast<int>(pipelineType) < pPipelines_.size());
        return pPipelines_[static_cast<uint64_t>(pipelineType)];
    }
    inline const std::vector<std::unique_ptr<Pipeline::Base>> &getPipelines() const { return pPipelines_; }

    // CLEAN UP
    void needsUpdate(const std::vector<SHADER> types);
    void cleanup(int frameIndex = -1);
    void update();

   private:
    void reset() override;

    // CACHE
    VkPipelineCache cache_;  // TODO: what is this for???

    // PUSH CONSTANT
    uint32_t maxPushConstantsSize_;

    // PIPELINES
    void createPipelineCache(VkPipelineCache &cache);
    std::vector<std::unique_ptr<Pipeline::Base>> pPipelines_;
    pipelineMap pipelineMap_;

    // CLEAN UP
    std::set<PIPELINE> needsUpdateSet_;
    std::vector<std::pair<int, VkPipeline>> oldPipelines_;
};

}  // namespace Pipeline

#endif  // !PIPELINE_HANDLER_H
