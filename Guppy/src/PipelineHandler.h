/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef PIPELINE_HANDLER_H
#define PIPELINE_HANDLER_H

#include <map>
#include <set>
#include <string>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "ConstantsAll.h"
#include "Game.h"
#include "Material.h"
#include "Mesh.h"
#include "Pipeline.h"
#include "Vertex.h"

namespace Pipeline {

class Handler : public Game::Handler {
    friend class Graphics;

   public:
    Handler(Game *pGame);

    void init() override;
    void tick() override;
    void destroy() override;

    // CACHE
    constexpr const vk::PipelineCache &getPipelineCache() const { return cache_; }

    // PUSH CONSTANT
    std::vector<vk::PushConstantRange> getPushConstantRanges(const PIPELINE &pipelineType,
                                                             const std::vector<PUSH_CONSTANT> &pushConstantTypes) const;

    // PIPELINES
    void initPipelines();
    void createPipelines(const pipelinePassSet &set);
    void createPipeline(const std::string &&name, vk::GraphicsPipelineCreateInfo &createInfo, vk::Pipeline &pipeline);
    void createPipeline(const std::string &&name, vk::ComputePipelineCreateInfo &createInfo, vk::Pipeline &pipeline);
    constexpr const auto &getPipelineBindDataMap() const { return pipelineBindDataMap_; }
    bool checkVertexPipelineMap(VERTEX key, PIPELINE value) const;

    // DESCRIPTOR SET
    Uniform::offsetsMap makeUniformOffsetsMap();
    void makeShaderInfoMap(Shader::infoMap &map);

    inline const std::unique_ptr<Pipeline::Base> &getPipeline(const PIPELINE &pipelineType) const {
        return pPipelines_.at(pipelineType);
    }
    constexpr const std::map<PIPELINE, std::unique_ptr<Pipeline::Base>> &getPipelines() const { return pPipelines_; }

    // SHADER
    void getShaderStages(const std::set<PIPELINE> &pipelineTypes, vk::ShaderStageFlags &stages);

    // CLEAN UP
    void needsUpdate(const std::vector<SHADER> types);
    void cleanup();

   private:
    void reset() override;

    // CACHE
    vk::PipelineCache cache_;  // TODO: what is this for???

    // PUSH CONSTANT
    uint32_t maxPushConstantsSize_;

    // PIPELINES
    void createPipelineCache(vk::PipelineCache &cache);
    std::map<PIPELINE, std::unique_ptr<Pipeline::Base>> pPipelines_;
    pipelineBindDataMap pipelineBindDataMap_;

    // CLEAN UP
    std::set<PIPELINE> needsUpdateSet_;
    std::vector<std::pair<int, vk::Pipeline>> oldPipelines_;
};

}  // namespace Pipeline

#endif  // !PIPELINE_HANDLER_H
