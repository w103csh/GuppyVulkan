/*
 * Copyright (C) 2021 Colin Hughes <colin.s.hughes@gmail.com>
 * All Rights Reserved
 */

#ifndef PASS_CONSTANTS_H
#define PASS_CONSTANTS_H

#include <set>
#include <string>
#include <utility>
#include <vulkan/vulkan.hpp>

// Why is this a multiset?
using pipelinePassSet = std::multiset<std::pair<PIPELINE, PASS>>;

namespace Pass {

struct PipelineData {
    constexpr bool operator==(const PipelineData& other) const {
        return usesDepth == other.usesDepth &&  //
               samples == other.samples;
    }
    constexpr bool operator!=(const PipelineData& other) { return !(*this == other); }
    vk::Bool32 usesDepth;
    vk::SampleCountFlagBits samples;
};

// clang-format off
struct GetTypeString {
    template <typename T> std::string operator()(const T&) const { assert(false); exit(EXIT_FAILURE); }
    std::string operator()(const RENDER_PASS& type)  const { return std::string("RENDER_PASS "  + std::to_string(static_cast<int>(type))); }
    std::string operator()(const COMPUTE_WORK& type) const { return std::string("COMPUTE_WORK " + std::to_string(static_cast<int>(type))); }
};
 struct IsRender {
    template <typename T>
    bool operator()(const T&) const { return false; }
    bool operator()(const RENDER_PASS&) const { return true; }
};
 struct GetRender {
    template <typename T>
    RENDER_PASS operator()(const T&) const { return RENDER_PASS::ALL_ENUM; }
    RENDER_PASS operator()(const RENDER_PASS& type) const { return type; }
};
 struct IsCompute {
    template <typename T>
    bool operator()(const T&) const { return false; }
    bool operator()(const COMPUTE_WORK&) const { return true; }
};
 struct GetCompute {
    template <typename T>
    COMPUTE_WORK operator()(const T&) const { return COMPUTE_WORK::ALL_ENUM; }
    COMPUTE_WORK operator()(const COMPUTE_WORK& type) const { return type; }
};
 struct IsAll {
    template <typename T>
    bool operator()(const T&) const { return false; }
    bool operator()(const RENDER_PASS&  type) const { return type == RENDER_PASS::ALL_ENUM; }
    bool operator()(const COMPUTE_WORK& type) const { return type == COMPUTE_WORK::ALL_ENUM; }
};
// clang-format on

}  // namespace Pass

#endif  // !PASS_CONSTANTS_H
