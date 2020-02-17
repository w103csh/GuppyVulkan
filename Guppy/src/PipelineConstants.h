/*
 * Copyright (C) 2020 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef PIPELINE_CONSTANTS_H
#define PIPELINE_CONSTANTS_H

#include <glm/glm.hpp>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "DescriptorConstants.h"

enum class DESCRIPTOR_SET;
enum class PUSH_CONSTANT;
enum class PASS : uint32_t;
enum class SHADER;
enum class VERTEX;

namespace Pipeline {

constexpr std::string_view LOCAL_SIZE_MACRO_ID_PREFIX = "_LS_";

// clang-format off
struct IsGraphics {
    template <typename T>
    bool operator()(const T&) const { return false; }
    bool operator()(const GRAPHICS&) const { return true; }
};
struct GetGraphics {
    template <typename T>
    GRAPHICS operator()(const T&) const { return GRAPHICS::ALL_ENUM; }
    GRAPHICS operator()(const GRAPHICS& type) const { return type; }
};
struct IsCompute {
    template <typename T>
    bool operator()(const T&) const { return false; }
    bool operator()(const COMPUTE&) const { return true; }
};
struct GetCompute {
    template <typename T>
    COMPUTE operator()(const T&) const { return COMPUTE::ALL_ENUM; }
    COMPUTE operator()(const COMPUTE& type) const { return type; }
};
struct IsAll {
    template <typename T>
    bool operator()(const T&) const { return false; }
    bool operator()(const GRAPHICS& type) const { return type == GRAPHICS::ALL_ENUM; }
    bool operator()(const COMPUTE&  type) const { return type == COMPUTE::ALL_ENUM; }
};
// clang-format on

extern const std::vector<PIPELINE> ALL;
extern const std::map<VERTEX, std::set<PIPELINE>> VERTEX_MAP;
extern const std::set<PIPELINE> MESHLESS;

struct BindData {
    const PIPELINE type;
    const vk::PipelineBindPoint bindPoint;
    const vk::PipelineLayout layout;
    vk::Pipeline pipeline;
    vk::ShaderStageFlags pushConstantStages;
    const std::vector<PUSH_CONSTANT> pushConstantTypes;
    bool usesAdjacency;
};

// Map of pipeline/pass to bind data shared pointers
using pipelineBindDataMapKey = std::pair<PIPELINE, PASS>;
using pipelineBindDataMapKeyValue = std::pair<const pipelineBindDataMapKey, const std::shared_ptr<Pipeline::BindData>>;
using pipelineBindDataMap = std::map<pipelineBindDataMapKey, const std::shared_ptr<Pipeline::BindData>>;
using pipelineBindDataList = insertion_ordered_unique_keyvalue_list<PIPELINE, std::shared_ptr<Pipeline::BindData>>;

// Map of pass types to descriptor set text replace tuples
using shaderTextReplaceInfoMap = std::map<std::set<PASS>, Descriptor::Set::textReplaceTuples>;

struct CreateInfo {
    PIPELINE type;
    std::string name = "";
    std::set<SHADER> shaderTypes;
    std::vector<DESCRIPTOR_SET> descriptorSets;
    Descriptor::OffsetsMap uniformOffsets;
    std::vector<PUSH_CONSTANT> pushConstantTypes;
    glm::uvec3 localSize{1, 1, 1};  // compute only
};

namespace Default {
struct PushConstant {
    glm::mat4 model;
};
extern const Pipeline::CreateInfo TRI_LIST_COLOR_CREATE_INFO;
extern const Pipeline::CreateInfo LINE_CREATE_INFO;
extern const Pipeline::CreateInfo POINT_CREATE_INFO;
extern const Pipeline::CreateInfo TRI_LIST_TEX_CREATE_INFO;
extern const Pipeline::CreateInfo CUBE_CREATE_INFO;
}  // namespace Default

namespace BP {
extern const Pipeline::CreateInfo TEX_CULL_NONE_CREATE_INFO;
}  // namespace BP

}  // namespace Pipeline

#endif  // !PIPELINE_CONSTANTS_H
